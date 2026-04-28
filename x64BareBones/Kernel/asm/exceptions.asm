GLOBAL isr_exc_de
GLOBAL isr_exc_ud
extern exceptionDispatcher       ; C: void exceptionDispatcher(int vec, uint64_t *regs)

section .bss
GLOBAL exc_resume_rip
exc_resume_rip:  resq 1

section .text


%macro PUSH_ALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
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