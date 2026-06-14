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

/* Saca una ocurrencia de `pid` de la cola, compactando. Retorna 1 si estaba. */
static int remove_waiter(Sem *s, uint64_t pid) {
	uint64_t tmp[MAX_SEM_WAITERS];
	int idx = s->wait_head, m = 0, found = 0;
	for (int i = 0; i < s->wait_count; i++) {
		uint64_t p = s->waiters[idx];
		idx = (idx + 1) % MAX_SEM_WAITERS;
		if (p == pid && !found)
			found = 1;       /* saltear una sola ocurrencia */
		else
			tmp[m++] = p;
	}
	if (found) {
		for (int i = 0; i < m; i++)
			s->waiters[i] = tmp[i];
		s->wait_head = 0;
		s->wait_count = m;
	}
	return found;
}

/* Baja un refcount y libera el semáforo si llega a 0. El caller tiene irq tomado. */
static void sem_drop(int id) {
	Sem *s = &sems[id];
	if (!s->in_use)
		return;
	if (--s->refs <= 0) {
		s->in_use = 0;
		/* Despertar a cualquiera que quedara esperando (sem destruido). */
		uint64_t pid;
		while ((pid = dequeue_waiter(s)) != 0)
			unblockProcess(pid);
	}
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
	} else {
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
	}

	/* Marcar que el proceso actual tiene este semáforo abierto, para liberarlo
	 * automáticamente si el proceso muere sin cerrarlo (ver sem_release_owned). */
	PCB *cur = getCurrentProcess();
	if (cur)
		cur->sems_opened |= (1u << id);

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

	uint64_t flags = irq_save();
	if (!sems[id].in_use) {
		irq_restore(flags);
		return -1;
	}

	PCB *cur = getCurrentProcess();
	if (cur)
		cur->sems_opened &= ~(1u << id);
	sem_drop(id);

	irq_restore(flags);
	return 0;
}

/*
 * Saca a `pid` de las colas de todos los semáforos donde estuviera bloqueado
 * y deshace su sem_wait (value++), manteniendo la contabilidad consistente.
 * La llama el scheduler cuando un proceso bloqueado es matado, así su wakeup
 * no se pierde y el semáforo no queda desfasado.
 */
void sem_release_waiter(uint64_t pid) {
	uint64_t flags = irq_save();
	for (int i = 0; i < MAX_SEMS; i++) {
		Sem *s = &sems[i];
		if (!s->in_use || s->wait_count == 0)
			continue;
		if (remove_waiter(s, pid))
			s->value++;
	}
	irq_restore(flags);
}

/*
 * Cierra todos los semáforos que `mask` marca como abiertos por un proceso.
 * La llama el scheduler cuando un proceso termina (muere o sale), para devolver
 * sus referencias; el semáforo se libera recién cuando ya nadie lo tiene abierto.
 */
void sem_release_owned(uint32_t mask) {
	uint64_t flags = irq_save();
	for (int id = 0; id < MAX_SEMS; id++)
		if (mask & (1u << id))
			sem_drop(id);
	irq_restore(flags);
}