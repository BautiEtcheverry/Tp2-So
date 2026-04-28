GLOBAL syscall_gate_init
GLOBAL isr_irq1_keyboard
GLOBAL _irq01Handler

extern isr_syscall_80
extern keyboard_isr_handler
extern isr_exc_de
extern isr_exc_ud

section .bss

GLOBAL capture_provisoria
GLOBAL capture_definitiva
capture_provisoria: resq 19
capture_definitiva: resq 19

section .text

; Install IDT gate for interrupt 0x80
syscall_gate_init:
    push rbp
    mov rbp, rsp
    sub rsp, 16                ; space to store IDTR (10 bytes used)
    sidt [rsp]                 ; store IDTR at [rsp]: limit(2), base(8)

    ; Read base address of IDT
    mov rbx, [rsp+2]           ; base is at offset +2
    add rsp, 16

    ; Helper macro-like: rsi = vector, rax = handler, rbx = idt base
%macro SET_GATE 2
    mov rdi, rbx
    mov rsi, %1
    mov rax, %2
    mov rcx, rsi
    shl rcx, 4
    add rdi, rcx
    mov dx, 0x08
    mov word [rdi], ax
    mov word [rdi+2], dx
    mov byte [rdi+4], 0x00
    mov byte [rdi+5], 0x8E     ; kernel gate by default
    shr rax, 16
    mov word [rdi+6], ax
    shr rax, 16
    mov dword [rdi+8], eax
    mov dword [rdi+12], 0
%endmacro

    ; Set syscall 0x80 with DPL=3
    mov rax, isr_syscall_80
    mov rdi, rbx
    mov rcx, 0x80
    shl rcx, 4
    add rdi, rcx
    mov word [rdi], ax
    mov word [rdi+2], 0x08
    mov byte [rdi+4], 0x00
    mov byte [rdi+5], 0xEF     
    shr rax, 16
    mov word [rdi+6], ax
    shr rax, 16
    mov dword [rdi+8], eax
    mov dword [rdi+12], 0

    ; Set keyboard IRQ (0x21) gate (kernel only)
    SET_GATE 0x21, isr_irq1_keyboard
    
    ; Exceptions: 0 = Divide Error, 6 = Invalid Opcode
    SET_GATE 0x00, isr_exc_de
    SET_GATE 0x06, isr_exc_ud
    
    mov rsp, rbp
    pop rbp
    ret

; Keyboard IRQ (0x21) handler stub 

isr_irq1_keyboard:
_irq01Handler:
    ; Capture registers BEFORE pushing any state
    ; General purpose
    mov [capture_provisoria + 0*8], rax    ; RAX
    mov [capture_provisoria + 1*8], rbx    ; RBX
    mov [capture_provisoria + 2*8], rcx    ; RCX
    mov [capture_provisoria + 3*8], rdx    ; RDX
    mov [capture_provisoria + 4*8], rsi    ; RSI
    mov [capture_provisoria + 5*8], rdi    ; RDI
    mov [capture_provisoria + 6*8], rbp    ; RBP
    lea rax, [rsp+24]                      ; RSP as seen by caller before CPU pushes
    mov [capture_provisoria + 7*8], rax    ; RSP (pre-interrupt)
    mov [capture_provisoria + 8*8], r8     ; R8
    mov [capture_provisoria + 9*8], r9     ; R9
    mov [capture_provisoria +10*8], r10    ; R10
    mov [capture_provisoria +11*8], r11    ; R11
    mov [capture_provisoria +12*8], r12    ; R12
    mov [capture_provisoria +13*8], r13    ; R13
    mov [capture_provisoria +14*8], r14    ; R14
    mov [capture_provisoria +15*8], r15    ; R15
    ; CPU-saved state at [rsp]: RIP, CS, RFLAGS 
    mov rax, [rsp]                         ; RIP
    mov [capture_provisoria +16*8], rax
    mov rax, [rsp+8]                       ; CS
    mov [capture_provisoria +17*8], rax
    mov rax, [rsp+16]                      ; RFLAGS
    mov [capture_provisoria +18*8], rax
  
    mov rax, [capture_provisoria + 0*8]

    ; Now preserve state and proceed
    push rbp
    mov rbp, rsp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Read scancode from port 0x60
    xor rax, rax
    in al, 0x60
    movzx r15, al           ; keep scancode in r15
    
    ; Call C handler: arg0=scancode
    mov rdi, r15
    call keyboard_isr_handler

    ; EOI PIC
    mov al, 0x20
    out 0x20, al

    ; Restore regs
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    mov rsp, rbp
    pop rbp
    iretq
