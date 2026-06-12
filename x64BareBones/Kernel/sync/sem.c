#include "sem.h"
#include "libasm.h"

static Sem sems[MAX_SEMS];

/*
 * Spinlock corto basado en atomic_xchg.
 *
 * atomic_xchg(addr, 1) escribe 1 y devuelve el valor previo en una
 * sola operacion atomica. Si devolvio 0 el lock estaba libre y ahora
 * es nuestro; si devolvio 1 alguien mas lo tiene y giramos.
 *
 * Solo protege secciones criticas de ~10 instrucciones (cuerpo del
 * semaforo). El bloqueo real de procesos lo hace blockProcess fuera
 * del spinlock, por lo que no hay busy-waiting de userland.
 */
/* Marcadas unused: las consumen sem_wait/sem_post en el proximo commit. */
__attribute__((unused))
static void spin_lock(volatile uint32_t *l) {
    while (atomic_xchg(l, 1) == 1) {
        /* spin */
    }
}

__attribute__((unused))
static void spin_unlock(volatile uint32_t *l) {
    *l = 0;
}

void sem_init(void) {
    for (int i = 0; i < MAX_SEMS; i++) {
        sems[i].value  = 0;
        sems[i].lock   = 0;
        sems[i].head   = 0;
        sems[i].tail   = 0;
        sems[i].count  = 0;
        sems[i].in_use = 0;
    }
}
