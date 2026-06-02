#include "scheduler.h"
#include "process.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Round Robin con prioridades por quantums.
 * priority 0 → MAX_QUANTUMS ticks por turno
 * priority N → max(1, MAX_QUANTUMS - N) ticks por turno
 * Proceso bloqueado o muerto = nunca scheduleable.
 */
#define MAX_QUANTUMS 4

static void wakeWaiters(uint64_t dead_pid);
static int hasWaiter(uint64_t pid);

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
	pcb->state = READY;
	/* insertar al final (justo antes de head) */
	PCB *p = head;
	while (p->next != head)
		p = p->next;
	p->next = pcb;
	pcb->next = head;
}

uint64_t schedule(uint64_t currentRSP) {
	if (current != NULL)
		current->rsp = currentRSP;

	/* Auto-reap: si el proceso actual murió y nadie lo espera, liberarlo ahora.
	 * Si alguien lo espera con waitpid, se queda como DEAD hasta que ese
	 * proceso llame waitForProcess y lo reapee. */
	if (current != NULL && current->state == DEAD && !hasWaiter(current->pid))
		reapProcess(current->pid); /* pone current = NULL internamente */

	/* Si al proceso actual le quedan quantums y sigue listo, continúa */
	if (current != NULL && current->state == READY && quantums_remaining > 0) {
		quantums_remaining--;
		return current->rsp;
	}

	/* Buscar el siguiente proceso READY (no idle) en orden circular */
	PCB *start = (current != NULL) ? current->next : head;
	PCB *p = start;
	PCB *found = NULL;
	do {
		if (p->state == READY && p != idle_proc) {
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
	PCB *p = head;
	if (!p)
		return;
	do {
		if (p->pid == pid) {
			p->state = BLOCKED;
			if (p == current)
				quantums_remaining = 0;
			return;
		}
		p = p->next;
	} while (p != head);
}

void unblockProcess(uint64_t pid) {
	PCB *p = head;
	if (!p)
		return;
	do {
		if (p->pid == pid) {
			if (p->state == BLOCKED)
				p->state = READY;
			return;
		}
		p = p->next;
	} while (p != head);
}

void killProcess(uint64_t pid) {
	PCB *p = head;
	if (!p)
		return;
	do {
		if (p->pid == pid) {
			p->state = DEAD;
			if (p == current)
				quantums_remaining = 0;
			wakeWaiters(pid);
			/* Si nadie espera este proceso y no es el actual,
			 * reapearlo de inmediato para liberar memoria */
			if (p != current && !hasWaiter(pid))
				reapProcess(pid);
			return;
		}
		p = p->next;
	} while (p != head);
}

void setPriority(uint64_t pid, int priority) {
	PCB *p = head;
	if (!p)
		return;
	do {
		if (p->pid == pid) {
			p->priority = priority;
			if (p == current)
				quantums_remaining = 0;
			return;
		}
		p = p->next;
	} while (p != head);
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
		cur->exit_status = status;
		cur->state = DEAD;
		quantums_remaining = 0;
		wakeWaiters(cur->pid);
	}
	while (1)
		__asm__ volatile("hlt"); // Espera hasta que el proximo tick del timer y despues no se lo vuelve a elgir.
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
	PCB *target = findProcess(pid);
	if (!target)
		return -1;

	PCB *cur = getCurrentProcess();
	if (!cur || cur->pid == pid)
		return -1;

	/* Si ya murió, reapear y retornar su exit status */
	if (target->state == DEAD) {
		int status = target->exit_status;
		reapProcess(pid);
		return status;
	}

	/* Bloquearse hasta que el proceso objetivo muera */
	cur->wait_pid = pid;
	cur->state = BLOCKED;
	quantums_remaining = 0;
	while (((volatile ProcessState) cur->state) == BLOCKED)
		__asm__ volatile("hlt");

	/* El proceso murió y wakeWaiters nos desbloqueó —
	 * target sigue válido porque aún no fue reaped */
	int status = target->exit_status;
	reapProcess(pid);
	return status;
}