GLOBAL isr_exc_de
GLOBAL isr_exc_ud
GLOBAL isr_exc_gp
GLOBAL isr_exc_pf
extern exceptionDispatcher       ; C: void exceptionDispatcher(int vec, uint64_t *regs)

section .bss
GLOBAL exc_resume_rip
exc_resume_rip:  resq 1
GLOBAL exc_cr2
exc_cr2:     resq 1              ; CR2 (dir que causó el #PF), seteado por isr_exc_pf
GLOBAL exc_errcode
exc_errcode: resq 1             ; error code que pushea el CPU en #PF/#GP

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


; -------------------------------
; #GP: General Protection (vector 13) — el CPU pushea error code
; -------------------------------
isr_exc_gp:
    push rax
    xor rax, rax
    mov [exc_cr2], rax           ; CR2 no aplica a #GP
    mov rax, [rsp+8]             ; error code (quedó en [rsp+8] tras el push rax)
    mov [exc_errcode], rax
    pop rax
    add rsp, 8                   ; descartar el error code del CPU -> [rsp]=RIP
    PUSH_ALL
    mov rdi, 13
    mov rsi, rsp
    call exceptionDispatcher
.hang_gp:
    cli
    hlt
    jmp .hang_gp


; -------------------------------
; #PF: Page Fault (vector 14) — el CPU pushea error code; CR2 = dir fallida
; -------------------------------
isr_exc_pf:
    push rax
    mov rax, cr2
    mov [exc_cr2], rax
    mov rax, [rsp+8]             ; error code (tras el push rax)
    mov [exc_errcode], rax
    pop rax
    add rsp, 8                   ; descartar el error code del CPU -> [rsp]=RIP
    PUSH_ALL
    mov rdi, 14
    mov rsi, rsp
    call exceptionDispatcher
.hang_pf:
    cli
    hlt
    jmp .hang_pf