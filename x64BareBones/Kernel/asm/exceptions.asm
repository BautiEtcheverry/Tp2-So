GLOBAL isr_exc_de
GLOBAL isr_exc_ud
extern exceptionDispatcher       ; C: void exceptionDispatcher(int vec, uint64_t *regs)

section .bss
GLOBAL exc_resume_rip
exc_resume_rip:  resq 1

section .text


; PUSH_ALL: pushea de modo que stack_frame[i] coincida con el indice del C:
; [0]=RAX [1]=RBX [2]=RCX [3]=RDX [4]=RSI [5]=RDI [6]=RBP
; [7]=R8  [8]=R9  [9]=R10 [10]=R11 [11]=R12 [12]=R13 [13]=R14 [14]=R15
; El primer push va a la direccion mas alta, asi que se pushea al reves.
%macro PUSH_ALL 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

%macro POP_ALL 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

; -------------------------------
; #DE: Divide Error (vector 0)
; -------------------------------
isr_exc_de:
    PUSH_ALL
    mov rdi, 0
    mov rsi, rsp
    call exceptionDispatcher
    POP_ALL
    mov rax, [rsp]
    add rax, 2
    mov [rsp], rax
    iretq


; -------------------------------
; #UD: Invalid Opcode (vector 6)
; -------------------------------
; Vector 6
isr_exc_ud:
    PUSH_ALL
    mov rdi, 6
    mov rsi, rsp
    call exceptionDispatcher
    POP_ALL
    mov rax, [rsp]
    add rax, 2
    mov [rsp], rax
    iretq