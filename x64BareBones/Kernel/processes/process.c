#include "process.h"
#include "memory_manager.h"
#include "scheduler.h"
#include <stdint.h>

#define PROCESS_STACK_SIZE (4096 * 4)

static uint64_t next_pid = 1;
extern void process_exit_helper(void);

static void process_exit(void) {
    int status;
    // El return de entry() vive en EAX cuando se ejecuta su 'ret'.
    __asm__ volatile ("movl %%eax, %0" : "=r"(status));
    exitCurrentProcess(status);
}

/* Implementada en idt.asm — construye el frame inicial para iretq.
   El formato del frame es responsabilidad exclusiva de idt.asm. */
uint64_t _init_process_stack(uint64_t stack_top, ProcessMain entry, int64_t argc, char **argv, void (*on_exit)(void));

PCB *createProcess(const char *name, ProcessMain entry, int argc, char **argv, int priority, int foreground) {
	PCB *pcb = (PCB *) sys_malloc(sizeof(PCB));
	if (!pcb)
		return NULL;

	void *stack = sys_malloc(PROCESS_STACK_SIZE);
	if (!stack) {
		sys_free(pcb);
		return NULL;
	}

	int i;
	for (i = 0; i < 63 && name && name[i]; i++)
		pcb->name[i] = name[i];
	pcb->name[i] = '\0';

	pcb->pid = next_pid++;
	pcb->stackBase = (uint64_t) stack;
	pcb->stackSize = PROCESS_STACK_SIZE;
	pcb->state = READY;
	pcb->priority = priority;
	pcb->foreground = foreground;
	pcb->fd[0] = 0;
	pcb->fd[1] = 1;
	pcb->next = (void *) 0;
	pcb->rbp = 0;
	pcb->exit_status = 0;
	pcb->wait_pid    = 0;

	pcb->rsp = _init_process_stack(pcb->stackBase + PROCESS_STACK_SIZE, entry, (int64_t) argc, argv, process_exit_helper);

	return pcb;
}

void destroyProcess(PCB *pcb) {
	if (!pcb)
		return;
	sys_free((void *) pcb->stackBase);
	sys_free(pcb);
}
