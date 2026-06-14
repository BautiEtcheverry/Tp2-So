#ifndef SEM_H
#define SEM_H

#include <stdint.h>

#define MAX_SEMS         32
#define SEM_NAME_LEN     32
#define MAX_SEM_WAITERS  64   /* PIDs en cola de espera por semáforo */

/* Inicializa la tabla global de semáforos — llamar desde kernel_init */
void sem_init(void);

/*
 * Abre o crea un semáforo nombrado.
    Si ya existe uno con ese nombre, incrementa su refcount y retorna su id(ignora initialValue).
    Si no existe, lo crea con valor inicial initialValue.
    Retorna el id (>= 0) o -1 si no hay slots libres o el nombre es inválido.
 */
int sem_open(const char *name, uint64_t initialValue);

/* Bloquea al proceso actual si el valor pasa a < 0. Retorna 0 o -1. */
int sem_wait(int id);

/* Despierta a un proceso en espera si lo hay. Retorna 0 o -1. */
int sem_post(int id);

/* Cierra el semáforo. Cuando no quedan usuarios (refcount 0), libera el slot. */
int sem_close(int id);

/*
 * Saca a `pid` de las colas de espera de todos los semáforos y deshace su
 * sem_wait pendiente. La llama el scheduler al matar un proceso bloqueado.
 */
void sem_release_waiter(uint64_t pid);

/*
 * Cierra todos los semáforos marcados en `mask` (bitmask por id). La llama el
 * scheduler al terminar un proceso, para devolver sus referencias y liberar
 * el semáforo cuando ya nadie lo usa.
 */
void sem_release_owned(uint32_t mask);

#endif