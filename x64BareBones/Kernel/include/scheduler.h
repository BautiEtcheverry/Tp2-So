#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stdint.h>

void initScheduler(PCB *idleProcess);
void addProcess(PCB *pcb);
uint64_t schedule(uint64_t currentRSP); // retorna nuevo RSP a cargar
void blockProcess(uint64_t pid);
void unblockProcess(uint64_t pid);
void killProcess(uint64_t pid);
void setPriority(uint64_t pid, int priority);
PCB *getCurrentProcess();
uint64_t getCurrentPID();
void exitCurrentProcess(int status);
/* Retorna el exit_status del proceso, o -1 si el pid no existe. Libera el PCB. */
int waitForProcess(uint64_t pid);
/* Saca el proceso DEAD de la lista y libera su memoria. */
void reapProcess(uint64_t pid);

#endif