#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef enum { READY, RUNNING, BLOCKED, DEAD } ProcessState;

typedef struct PCB {
    uint64_t      pid;
    char          name[64];
    uint64_t      rsp;        // stack pointer guardado en context switch
    uint64_t      rbp;
    uint64_t      stackBase;
    uint64_t      stackSize;
    ProcessState  state;
    int           priority;   // 0 = máxima, mayor número = menor prioridad
    int           foreground; // 1 si es foreground de la shell
    int           fd[2];      // fd[0]=stdin fd[1]=stdout — para pipes (Paso 6)
    struct PCB  * next;
} PCB;

typedef int (*ProcessMain)(int argc, char ** argv);

PCB * createProcess(const char * name, ProcessMain entry,
                    int argc, char ** argv,
                    int priority, int foreground);
void  destroyProcess(PCB * pcb);

#endif