#include "scheduler.h"
#include "process.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Round Robin con prioridades por quantums.
 * priority 0 → MAX_QUANTUMS ticks por turno
 * priority N → max(1, MAX_QUANTUMS - N) ticks por turno
 * Proceso bloqueado o muerto = nunca scheduleable.
 */
#define MAX_QUANTUMS 4

static PCB * head             = NULL;
static PCB * current          = NULL;
static PCB * idle_proc        = NULL;
static int   quantums_remaining = 0;

static int quantums_for(int priority) {
    int q = MAX_QUANTUMS - priority;
    return (q > 0) ? q : 1;
}

void initScheduler(PCB * idleProcess) {
    idle_proc          = idleProcess;
    idle_proc->state   = READY;
    idle_proc->next    = idle_proc;   /* lista circular de un nodo */
    head               = idle_proc;
    current            = NULL;
    quantums_remaining = 0;
}

void addProcess(PCB * pcb) {
    if (!head || !pcb) return;
    pcb->state = READY;
    /* insertar al final (justo antes de head) */
    PCB * p = head;
    while (p->next != head) p = p->next;
    p->next   = pcb;
    pcb->next = head;
}

uint64_t schedule(uint64_t currentRSP) {
    if (current != NULL)
        current->rsp = currentRSP;

    /* Si al proceso actual le quedan quantums y sigue listo, continúa */
    if (current != NULL && current->state == READY && quantums_remaining > 0) {
        quantums_remaining--;
        return current->rsp;
    }

    /* Buscar el siguiente proceso READY (no idle) en orden circular */
    PCB * start = (current != NULL) ? current->next : head;
    PCB * p     = start;
    PCB * found = NULL;
    do {
        if (p->state == READY && p != idle_proc) {
            found = p;
            break;
        }
        p = p->next;
    } while (p != start);

    if (found == NULL) found = idle_proc;

    current            = found;
    quantums_remaining = quantums_for(current->priority) - 1;
    return current->rsp;
}

void blockProcess(uint64_t pid) {
    PCB * p = head;
    if (!p) return;
    do {
        if (p->pid == pid) {
            p->state = BLOCKED;
            if (p == current) quantums_remaining = 0;
            return;
        }
        p = p->next;
    } while (p != head);
}

void unblockProcess(uint64_t pid) {
    PCB * p = head;
    if (!p) return;
    do {
        if (p->pid == pid) {
            if (p->state == BLOCKED) p->state = READY;
            return;
        }
        p = p->next;
    } while (p != head);
}

void killProcess(uint64_t pid) {
    PCB * p = head;
    if (!p) return;
    do {
        if (p->pid == pid) {
            p->state = DEAD;
            if (p == current) quantums_remaining = 0;
            return;
        }
        p = p->next;
    } while (p != head);
}

void setPriority(uint64_t pid, int priority) {
    PCB * p = head;
    if (!p) return;
    do {
        if (p->pid == pid) {
            p->priority = priority;
            if (p == current) quantums_remaining = 0;
            return;
        }
        p = p->next;
    } while (p != head);
}

PCB * getCurrentProcess(void) {
    return current;
}

uint64_t getCurrentPID(void) {
    return current ? current->pid : 0;
}
