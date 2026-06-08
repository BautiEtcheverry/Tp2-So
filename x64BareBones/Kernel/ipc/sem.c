#include "sem.h"
#include "libasm.h"
#include "process.h"
#include "scheduler.h"
#include <stddef.h>

typedef struct {
	int in_use;
	char name[SEM_NAME_LEN];
	int64_t value;					   // El valor propio del sem, dependera de si es un mutex o un contador
	int refs;						   // Cuantos tienen el sem abierto
	uint64_t waiters[MAX_SEM_WAITERS]; // cola circular FIFO de PIDs
	int wait_head;
	int wait_count;
} Sem;

static Sem sems[MAX_SEMS];

/*--------------------- helpers de string ---------------------*/
static int name_eq(const char *a, const char *b) {
	for (int i = 0; i < SEM_NAME_LEN; i++) {
		if (a[i] != b[i])
			return 0;
		if (a[i] == 0)
			return 1;
	}
	return 1;
}

static void name_copy(char *dst, const char *src) {
	int i;
	for (i = 0; i < SEM_NAME_LEN - 1 && src[i]; i++)
		dst[i] = src[i];
	dst[i] = 0;
}

/*--------------------- cola FIFO de bloqueados ---------------------*/
static void enqueue_waiter(Sem *s, uint64_t pid) {
	if (s->wait_count >= MAX_SEM_WAITERS)
		return; // cola llena
	int tail = (s->wait_head + s->wait_count) % MAX_SEM_WAITERS;
	s->waiters[tail] = pid;
	s->wait_count++;
}

static uint64_t dequeue_waiter(Sem *s) {
	if (s->wait_count == 0)
		return 0;
	uint64_t pid = s->waiters[s->wait_head];
	s->wait_head = (s->wait_head + 1) % MAX_SEM_WAITERS;
	s->wait_count--;
	return pid;
}

/*--------------------- API ---------------------*/
void sem_init(void) {
	for (int i = 0; i < MAX_SEMS; i++) {
		sems[i].in_use = 0;
		sems[i].value = 0;
		sems[i].refs = 0;
		sems[i].wait_head = 0;
		sems[i].wait_count = 0;
	}
}
static int find_by_name(const char *name) {
	for (int i = 0; i < MAX_SEMS; i++)
		if (sems[i].in_use && name_eq(sems[i].name, name))
			return i;
	return -1;
}

static int find_free_slot(void) {
	for (int i = 0; i < MAX_SEMS; i++)
		if (!sems[i].in_use)
			return i;
	return -1;
}

int sem_open(const char *name, uint64_t initialValue) {
	if (name == NULL)
		return -1;

	uint64_t flags = irq_save();

	int id = find_by_name(name);
	if (id != -1) {
		sems[id].refs++;
		irq_restore(flags);
		return id;
	}

	id = find_free_slot();
	if (id == -1) {
		irq_restore(flags);
		return -1;
	}

	Sem *s = &sems[id];
	s->in_use = 1;
	s->value = (int64_t) initialValue;
	s->refs = 1;
	s->wait_head = 0;
	s->wait_count = 0;
	name_copy(s->name, name);

	irq_restore(flags);
	return id;
}

int sem_wait(int id) {
	if (id < 0 || id >= MAX_SEMS)
		return -1;

	Sem *s = &sems[id];
	uint64_t flags = irq_save();
	if (!s->in_use) {
		irq_restore(flags);
		return -1;
	}

	s->value--;
	if (s->value < 0) {
		uint64_t pid = getCurrentPID();
		enqueue_waiter(s, pid);
		blockProcess(pid);
		irq_restore(flags);

		/* Esperar hasta que sem_post nos desbloquee (state vuelve a READY). */
		PCB *self = getCurrentProcess();
		while (((volatile ProcessState) self->state) == BLOCKED)
			// Usamos (volatile ProcessState) para que el compilador sepa que ese es un valor que va ser
			// modificado por fuera de este codigo, en caso de no hacerlo el compilador agarra y guarda el
			// valor que leyo de la memoria una vez, es decir lo cachea. De esta forma en cada ciclo vuelve
			// a leer el valor de la memorial, justo lo que necesitabamos para ver si podemos seguir
			// haciendo lo nuestro.
			__asm__ volatile("hlt");
	}
	else {
		irq_restore(flags);
	}
	return 0;
}

int sem_post(int id) {
	if (id < 0 || id >= MAX_SEMS)
		return -1;

	Sem *s = &sems[id];
	uint64_t flags = irq_save();
	if (!s->in_use) {
		irq_restore(flags);
		return -1;
	}

	s->value++;
	if (s->value <= 0) {
		uint64_t pid = dequeue_waiter(s);
		if (pid != 0)
			unblockProcess(pid);
	}

	irq_restore(flags);
	return 0;
}

int sem_close(int id) {
	if (id < 0 || id >= MAX_SEMS)
		return -1;

	Sem *s = &sems[id];
	uint64_t flags = irq_save();
	if (!s->in_use) {
		irq_restore(flags);
		return -1;
	}

	if (--s->refs <= 0) {
		s->in_use = 0;
		/* Despertar a cualquiera que quedara esperando (sem destruido). */
		uint64_t pid;
		while ((pid = dequeue_waiter(s)) != 0)
			unblockProcess(pid);
	}

	irq_restore(flags);
	return 0;
}