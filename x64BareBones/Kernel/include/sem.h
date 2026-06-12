#ifndef SEM_H
#define SEM_H

#include <stdint.h>

#define MAX_SEMS         32
#define MAX_SEM_WAITERS  32

/*
 * Semaforo contador con cola FIFO de procesos bloqueados.
 *
 *   value   : contador del semaforo (>= 0 cuando no hay waiters).
 *   lock    : spinlock interno (0=libre, 1=tomado). Se toma con
 *             atomic_xchg para garantizar atomicidad sobre value
 *             y la cola de waiters frente a interrupciones.
 *   waiters : cola circular de PIDs bloqueados esperando.
 *   in_use  : 1 si este slot esta activo (abierto por al menos un proceso).
 */
typedef struct {
    int               value;
    volatile uint32_t lock;
    uint64_t          waiters[MAX_SEM_WAITERS];
    int               head;
    int               tail;
    int               count;
    int               in_use;
} Sem;

/* Inicializa la tabla global de semaforos. Llamar desde kernel_init. */
void sem_init(void);

#endif
