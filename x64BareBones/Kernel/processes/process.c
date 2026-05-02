#include "process.h"
#include "memory_manager.h"
#include <stdint.h>

#define PROCESS_STACK_SIZE (4096 * 4)

static uint64_t next_pid = 1;

static void process_exit(void) {
    while (1) { ; }
}

PCB * createProcess(const char * name, ProcessMain entry,
                    int argc, char ** argv,
                    int priority, int foreground) {
    PCB * pcb = (PCB *) sys_malloc(sizeof(PCB));
    if (!pcb) return NULL;

    void * stack = sys_malloc(PROCESS_STACK_SIZE);
    if (!stack) {
        sys_free(pcb);
        return NULL;
    }

    int i;
    for (i = 0; i < 63 && name && name[i]; i++)
        pcb->name[i] = name[i];
    pcb->name[i] = '\0';

    pcb->pid        = next_pid++;
    pcb->stackBase  = (uint64_t) stack;
    pcb->stackSize  = PROCESS_STACK_SIZE;
    pcb->state      = READY;
    pcb->priority   = priority;
    pcb->foreground = foreground;
    pcb->fd[0]      = 0;
    pcb->fd[1]      = 1;
    pcb->next       = (void *) 0;
    pcb->rbp        = 0;

    /*
     * Build the initial stack frame so the process can start via iretq.
     *
     * After the IRQ0 handler does:
     *   pop rax; pop rbx; ... pop rdi; pop r8..r15; pop rbp
     *   add rsp, 8   ; skip error-code slot
     *   iretq        ; pops RIP, CS, RFLAGS
     *
     * Layout from pcb->rsp (low) to high:
     *   +0   rax=0
     *   +8   rbx=0
     *   +16  rcx=0
     *   +24  rdx=0
     *   +32  rsi=argv
     *   +40  rdi=argc
     *   +48  r8=0  ...  +104 r15=0
     *   +112 rbp=0
     *   +120 error_code=0
     *   +128 RIP=entry
     *   +136 CS=0x08
     *   +144 RFLAGS=0x202
     *
     * process_exit sits at stackTop so entry() returns there after iretq
     * leaves rsp pointing to it.
     */
    uint64_t * sp = (uint64_t *)((uint8_t *) stack + PROCESS_STACK_SIZE);

    *(--sp) = (uint64_t) process_exit;   /* return address for entry() */

    /* iretq frame */
    *(--sp) = 0x202;                     /* RFLAGS — IF enabled */
    *(--sp) = 0x08;                      /* CS — kernel code segment */
    *(--sp) = (uint64_t) entry;          /* RIP */

    *(--sp) = 0;                         /* error code placeholder */

    /* saved registers (pop order: rax first → rbp last) */
    *(--sp) = 0;                         /* rbp */
    *(--sp) = 0;                         /* r15 */
    *(--sp) = 0;                         /* r14 */
    *(--sp) = 0;                         /* r13 */
    *(--sp) = 0;                         /* r12 */
    *(--sp) = 0;                         /* r11 */
    *(--sp) = 0;                         /* r10 */
    *(--sp) = 0;                         /* r9  */
    *(--sp) = 0;                         /* r8  */
    *(--sp) = (uint64_t)(int64_t) argc;  /* rdi — first  arg */
    *(--sp) = (uint64_t) argv;           /* rsi — second arg */
    *(--sp) = 0;                         /* rdx */
    *(--sp) = 0;                         /* rcx */
    *(--sp) = 0;                         /* rbx */
    *(--sp) = 0;                         /* rax */

    pcb->rsp = (uint64_t) sp;
    return pcb;
}

void destroyProcess(PCB * pcb) {
    if (!pcb) return;
    sys_free((void *) pcb->stackBase);
    sys_free(pcb);
}
