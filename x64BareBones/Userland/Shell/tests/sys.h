#ifndef SYS_H
#define SYS_H

/*
 * Adaptador entre la API que esperan los tests de la cátedra
 * (semInit/semWait/semPost/semDestroy, createProcess, waitPid) y la API
 * real de nuestra libc de userland (sem_open/.../create_process/waitpid).
 * Para no modificar los tests
 */

#include <stdint.h>
#include "../../include/libc.h"

/* 
 Semáforos
*/ 

/*
 * El handle void* que devuelve semInit codifica el id del kernel como
 * (id + 1), de modo que el id 0 no se confunda con NULL (= error).
 * semWait/semPost/semDestroy decodifican restando 1.
 */
static inline void *semInit(char *name, uint64_t initialValue) {
    int id = sem_open(name, initialValue);
    return (id < 0) ? (void *)0 : (void *)((uint64_t)id + 1);
}

static inline int semWait(void *sem) {
    return sem_wait((int)(uint64_t)sem - 1);
}

static inline int semPost(void *sem) {
    return sem_post((int)(uint64_t)sem - 1);
}

static inline int semDestroy(void *sem) {
    return sem_close((int)(uint64_t)sem - 1);
}

/* ------------------------------------------------------------------ */
/* Procesos                                                             */
/* ------------------------------------------------------------------ */

/* Firma que usan los tests: entry como void*, argv como uint8_t**. */
static inline int64_t createProcess(void *entry, uint64_t argc, uint8_t **argv, uint8_t priority) {
    (void)priority;
    return create_process((int (*)(int, char **))entry, (int)argc, (char **)argv);
}

static inline int64_t waitPid(uint64_t pid) {
    return waitpid(pid);
}

/* Nombre con mayúscula que usan los tests de la cátedra. */
static inline uint64_t getPid(void) {
    return getpid();
}


#endif // SYS_H
