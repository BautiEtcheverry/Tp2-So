#include <stdint.h>
#include <videoDriver.h>
#include <naiveConsole.h>
#include <gfxConsole.h>
#include <keyboard.h>

typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, cs, rflags;
} registers_t;

static void vd_print_hex64(uint64_t v){
    static const char hex[]="0123456789ABCDEF";
    char b[19]; b[0]='0'; b[1]='x';
    for (int i=0;i<16;i++) b[2+i]=hex[(v>>(4*(15-i)))&0xF];
    b[18]=0;
    vdPrint(b);
}

static void print_registers(registers_t *r) {
    struct { const char *name; uint64_t val; } rows[] = {
        {" RAX: 0x", r->rax}, {" RBX: 0x", r->rbx}, {" RCX: 0x", r->rcx}, {" RDX: 0x", r->rdx},
        {" RSI: 0x", r->rsi}, {" RDI: 0x", r->rdi}, {" RSP: 0x", r->rsp}, {" RBP: 0x", r->rbp},
        {" R8 : 0x", r->r8 }, {" R9 : 0x", r->r9 }, {" R10: 0x", r->r10}, {" R11: 0x", r->r11},
        {" R12: 0x", r->r12}, {" R13: 0x", r->r13}, {" R14: 0x", r->r14}, {" R15: 0x", r->r15},
        {" RIP: 0x", r->rip}, {"RFL: 0x", r->rflags}
    };
    for (unsigned i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
        vdPrint((char*)rows[i].name);
        vd_print_hex64(rows[i].val);
        vdPrint("\n");
    }
}

static void printExp(registers_t *regs, const char *message) {
    vdSetColor(0xFF0000);
    vdPrint(message);
    print_registers(regs);
    vdPrint("\nReturning to shell...\n\n");
    vdSetColor(0xFFFFFF);
}

// stack_frame layout (long mode siempre pushea SS:RSP):
// [0]=RAX [1]=RBX [2]=RCX [3]=RDX [4]=RSI [5]=RDI [6]=RBP
// [7]=R8  [8]=R9  [9]=R10 [10]=R11 [11]=R12 [12]=R13 [13]=R14 [14]=R15
// [15]=RIP [16]=CS [17]=RFLAGS [18]=RSP [19]=SS
void exceptionDispatcher(uint64_t exception, uint64_t *stack_frame){
    registers_t regs;
    regs.rax = stack_frame[0];
    regs.rbx = stack_frame[1];
    regs.rcx = stack_frame[2];
    regs.rdx = stack_frame[3];
    regs.rsi = stack_frame[4];
    regs.rdi = stack_frame[5];
    regs.rbp = stack_frame[6];
    regs.r8  = stack_frame[7];
    regs.r9  = stack_frame[8];
    regs.r10 = stack_frame[9];
    regs.r11 = stack_frame[10];
    regs.r12 = stack_frame[11];
    regs.r13 = stack_frame[12];
    regs.r14 = stack_frame[13];
    regs.r15 = stack_frame[14];
    regs.rip = stack_frame[15];
    regs.cs  = stack_frame[16];
    regs.rflags = stack_frame[17];
    regs.rsp = stack_frame[18];

    switch (exception) {
        case 0:  printExp(&regs, "[EXCEPTION] Division by zero\n\n"); break;
        case 6:  printExp(&regs, "[EXCEPTION] Invalid opcode\n\n"); break;
        default:
            vdSetColor(0xFF0000);
            vdPrint("[EXCEPTION]\n\n");
            print_registers(&regs);
            vdSetColor(0xFFFFFF);
            break;
    }

    keyboard_clear_buffer();
}
