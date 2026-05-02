#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stdint.h>

void     initScheduler(PCB * idleProcess);
void     addProcess(PCB * pcb);
uint64_t schedule(uint64_t currentRSP);  // retorna nuevo RSP a cargar
void     blockProcess(uint64_t pid);
void     unblockProcess(uint64_t pid);
void     killProcess(uint64_t pid);
void     setPriority(uint64_t pid, int priority);
PCB    * getCurrentProcess();
uint64_t getCurrentPID();

#endif