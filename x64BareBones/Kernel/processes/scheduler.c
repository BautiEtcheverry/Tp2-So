#include "scheduler.h"
#include "process.h"
#include "pipe.h"
#include "sem.h"
#include "libasm.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Round Robin con prioridades por quantums.
 * priority 0 → MAX_QUANTUMS ticks por turno
 * priority N → max(1, MAX_QUANTUMS - N) ticks por turno
 * Proceso bloqueado o muerto = nunca scheduleable.
 */

static void wakeWaiters(uint64_t dead_pid);
static int hasWaiter(uint64_t pid);

/* Definida en syscall.c — drena el kill diferido que dejó Ctrl+C desde el ISR */
extern void drain_pending_kill(void);

static PCB *head = NULL;
static PCB *current = NULL;
static PCB *idle_proc = NULL;
static int quantums_remaining = 0;

static int quantums_for(int priority) {
	int q = MAX_QUANTUMS - priority;
	return (q > 0) ? q : 1;
}

void initScheduler(PCB *idleProcess) {
	idle_proc = idleProcess;
	idle_proc->state = READY;
	idle_proc->next = idle_proc; /* lista circular de un nodo */
	head = idle_proc;
	current = NULL;
	quantums_remaining = 0;
}

void addProcess(PCB *pcb) {
	if (!head || !pcb)
		return;
	uint64_t flags = irq_save();
	pcb->state = READY;
	/* insertar al final (justo antes de head). Tiene que ser bajo cli porque
	 * entre p->next = pcb y pcb->next = head el ring queda con un nodo cuyo
	 * next es basura — si el timer dispara ahí, schedule() recorre y crashea */
	PCB *p = head;
	while (p->next != head)
		p = p->next;
	pcb->next = head;
	p->next = pcb;
	irq_restore(flags);
}

uint64_t schedule(uint64_t currentRSP) {
	if (current != NULL)
		current->rsp = currentRSP;

	/* Drenar un Ctrl+C que el ISR del teclado haya dejado pendiente. Acá ya
	 * estamos en un contexto seguro: IF=0 y ningún path del kernel a mitad
	 * de tocar el ring (entramos vía el handler del timer). */
	drain_pending_kill();

	/* Auto-reap: si el proceso actual murió y nadie lo espera, liberarlo ahora.
	 * Si alguien lo espera con waitpid, se queda como DEAD hasta que ese
	 * proceso llame waitForProcess y lo reapee. */
	if (current != NULL && current->state == DEAD && !hasWaiter(current->pid))
		reapProcess(current->pid); /* pone current = NULL internamente */

	/* Si al proceso actual le quedan quantums y sigue listo (y no fue pausado
	 * manualmente), continúa */
	if (current != NULL && current->state == READY && !current->paused && quantums_remaining > 0) {
		quantums_remaining--;
		return current->rsp;
	}

	/* Buscar el siguiente proceso READY y no pausado (no idle) en orden circular */
	PCB *start = (current != NULL) ? current->next : head;
	PCB *p = start;
	PCB *found = NULL;
	do {
		if (p->state == READY && !p->paused && p != idle_proc) {
			found = p;
			break;
		}
		p = p->next;
	} while (p != start);

	if (found == NULL)
		found = idle_proc;

	current = found;
	quantums_remaining = quantums_for(current->priority) - 1;
	return current->rsp;
}

void blockProcess(uint64_t pid) {
	uint64_t flags = irq_save();
	PCB *p = head;
	if (!p) {
		irq_restore(flags);
		return;
	}
	do {
		if (p->pid == pid) {
			p->state = BLOCKED;
			if (p == current)
				quantums_remaining = 0;
			irq_restore(flags);
			return;
		}
		p = p->next;
	} while (p != head);
	irq_restore(flags);
}

void unblockProcess(uint64_t pid) {
	uint64_t flags = irq_save();
	PCB *p = head;
	if (!p) {
		irq_restore(flags);
		return;
	}
	do {
		if (p->pid == pid) {
			if (p->state == BLOCKED)
				p->state = READY;
			irq_restore(flags);
			return;
		}
		p = p->next;
	} while (p != head);
	irq_restore(flags);
}

/* Bloqueo manual (comando block): marca paused. No toca el state, así no
 * interfiere con un bloqueo por semáforo/pipe/waitpid. */
void pauseProcess(uint64_t pid) {
	uint64_t flags = irq_save();
	PCB *p = findProcess(pid);
	if (!p) {
		irq_restore(flags);
		return;
	}
	p->paused = 1;
	if (p == current)
		quantums_remaining = 0; /* si es el actual, que ceda el CPU ya */
	irq_restore(flags);
}

/* Desbloqueo manual (comando unblock): solo limpia paused. NO despierta a un
 * proceso dormido en un semáforo (ese sigue con state == BLOCKED). */
void resumeProcess(uint64_t pid) {
	uint64_t flags = irq_save();
	PCB *p = findProcess(pid);
	if (!p) {
		irq_restore(flags);
		return;
	}
	p->paused = 0;
	irq_restore(flags);
}

void killProcess(uint64_t pid) {
	if (pid <= 2) return;   /* idle y shell son intocables */
	uint64_t flags = irq_save();
	PCB *p = head;
	if (!p) {
		irq_restore(flags);
		return;
	}
	do {
		if (p->pid == pid) {
			/* Cerrar pipes del proceso si tiene alguno abierto */
			if (IS_PIPE_FD(p->fd[0])) pipe_close_read(PIPE_FD_TO_ID(p->fd[0]));
			if (IS_PIPE_FD(p->fd[1])) pipe_close_write(PIPE_FD_TO_ID(p->fd[1]));
			/* Sacarlo de las colas de semáforos donde estuviera bloqueado */
			sem_release_waiter(pid);
			/* Cerrar los semáforos que tuviera abiertos (devolver refcounts) */
			sem_release_owned(p->sems_opened);
			p->state = DEAD;
			if (p == current)
				quantums_remaining = 0;
			wakeWaiters(pid);
			/* Si nadie espera este proceso y no es el actual, reapear */
			if (p != current && !hasWaiter(pid))
				reapProcess(pid);
			irq_restore(flags);
			return;
		}
		p = p->next;
	} while (p != head);
	irq_restore(flags);
}

void setPriority(uint64_t pid, int priority) {
	uint64_t flags = irq_save();
	PCB *p = head;
	if (!p) {
		irq_restore(flags);
		return;
	}
	do {
		if (p->pid == pid) {
			p->priority = priority;
			if (p == current)
				quantums_remaining = 0;
			irq_restore(flags);
			return;
		}
		p = p->next;
	} while (p != head);
	irq_restore(flags);
}

PCB *getCurrentProcess(void) {
	return current;
}

uint64_t getCurrentPID(void) {
	return current ? current->pid : 0;
}

void exitCurrentProcess(int status) {
	PCB *cur = getCurrentProcess();
	if (cur) {
		/* Cerrar extremos de pipe si el proceso los tenía abiertos */
		if (IS_PIPE_FD(cur->fd[0])) pipe_close_read(PIPE_FD_TO_ID(cur->fd[0]));
		if (IS_PIPE_FD(cur->fd[1])) pipe_close_write(PIPE_FD_TO_ID(cur->fd[1]));
		/* Cerrar los semáforos que el proceso tuviera abiertos */
		sem_release_owned(cur->sems_opened);
		cur->exit_status = status;
		cur->state = DEAD;
		quantums_remaining = 0;
		wakeWaiters(cur->pid);
	}
	while (1)
		cpu_halt();
}


/*-----------------Helpers para WaitPid-----------------*/
static void wakeWaiters(uint64_t dead_pid) {
	PCB *p = head;
	if (!p)
		return;
	do {
		if (p->state == BLOCKED && p->wait_pid == dead_pid) {
			p->state = READY;
			p->wait_pid = 0;
		}
		p = p->next;
	} while (p != head);
}
static int hasWaiter(uint64_t pid) {
	if (!head) return 0;
	PCB *p = head;
	do {
		if (p->state == BLOCKED && p->wait_pid == pid)
			return 1;
		p = p->next;
	} while (p != head);
	return 0;
}

void yieldProcess(void) {
	quantums_remaining = 0;
}

PCB *getHeadProcess(void) {
	return head;
}

PCB *findProcess(uint64_t pid) {
	PCB *p = head;
	if (!p)
		return NULL;
	do {
		if (p->pid == pid)
			return p;
		p = p->next;
	} while (p != head);
	return NULL;
}


/*
 * Saca el proceso DEAD de la lista circular y libera su memoria.
 * Solo opera sobre procesos en estado DEAD. No toca al idle.
 */
void reapProcess(uint64_t pid) {
	PCB *target = findProcess(pid);
	if (!target || target == idle_proc || target->state != DEAD)
		return;

	/* Encontrar el nodo anterior en la lista circular */
	PCB *prev = head;
	while (prev->next != target) {
		prev = prev->next;
		if (prev == head)
			return; /* no encontrado, no debería pasar */
	}

	/* Desconectar de la lista */
	prev->next = target->next;
	if (head == target)
		head = target->next;
	if (current == target) {
		current = NULL;
		quantums_remaining = 0;
	}

	destroyProcess(target);
}

int waitForProcess(uint64_t pid) {
	uint64_t flags = irq_save();
	PCB *target = findProcess(pid);
	if (!target) {
		irq_restore(flags);
		return -1;
	}

	PCB *cur = getCurrentProcess();
	if (!cur || cur->pid == pid) {
		irq_restore(flags);
		return -1;
	}

	/* Si ya murió, reapear y retornar su exit status */
	if (target->state == DEAD) {
		int status = target->exit_status;
		reapProcess(pid);
		irq_restore(flags);
		return status;
	}

	/* Bloquearse hasta que el proceso objetivo muera. El check de DEAD y el
	 * set de wait_pid/BLOCKED van bajo el mismo cli: si el hijo muere entre
	 * medio, wakeWaiters no nos ve y quedamos dormidos para siempre. */
	cur->wait_pid = pid;
	cur->state = BLOCKED;
	quantums_remaining = 0;
	irq_restore(flags);

	while (((volatile ProcessState) cur->state) == BLOCKED)
		cpu_halt();

	/* El proceso murió y wakeWaiters nos desbloqueó —
	 * target sigue válido porque aún no fue reaped */
	int status = target->exit_status;
	reapProcess(pid);
	return status;
}
