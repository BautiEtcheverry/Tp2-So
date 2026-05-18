GLOBAL syscall_gate_init
GLOBAL isr_irq1_keyboard
GLOBAL _irq01Handler
GLOBAL _irq00Handler
GLOBAL _init_process_stack
extern schedule
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

    SET_GATE 0x20, _irq00Handler    ; IRQ0 = vector 0x20

    ; Set keyboard IRQ (0x21) gate (kernel only)
    SET_GATE 0x21, isr_irq1_keyboard
    
    ; Exceptions: 0 = Divide Error, 6 = Invalid Opcode
    SET_GATE 0x00, isr_exc_de
    SET_GATE 0x06, isr_exc_ud
    
    mov rsp, rbp
    pop rbp
    ret

; Handler IRQ0 (timer) — agrega context switch
_irq00Handler:
    pushq 0                  ; código de error ficticio (alineación)
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov al, 0x20
    out 0x20, al             ; EOI al PIC master

    mov rdi, rsp             ; pasar RSP actual como argumento a schedule()
    call schedule            ; retorna el nuevo RSP a cargar
    mov rsp, rax             ; cargar stack del nuevo proceso

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    add rsp, 8               ; descartar el código de error ficticio
    iretq

; Arma el frame inicial de un proceso nuevo en su stack, con el mismo layout
; que usa _irq00Handler, para que pueda ser lanzado via iretq.
;
; uint64_t _init_process_stack(uint64_t stack_top, ProcessMain entry,
;                               int64_t argc, char **argv, void (*on_exit)(void))
; rdi=stack_top  rsi=entry  rdx=argc  rcx=argv  r8=on_exit
; Retorna rax = RSP inicial del proceso
;
; Usa r12 como puntero manual al stack del nuevo proceso — nunca modifica rsp,
; así el stack del kernel que llama permanece intacto.
_init_process_stack:
    push rbp
    mov  rbp, rsp
    push r12
    push r13
    push r14

    mov  r12, rdi        ; r12 = cursor sobre el stack del nuevo proceso
    mov  r13, rsi        ; r13 = entry
    mov  r14, r8         ; r14 = on_exit
    ; rdx = argc, rcx = argv (se usan directo más abajo)

    ; Dirección de retorno para cuando entry() haga ret
    sub  r12, 8
    mov  [r12], r14

    ; iretq frame — mismo orden que espera iretq en 64 bits (ring0→ring0)
    sub  r12, 8
    mov  qword [r12], 0x202    ; RFLAGS (IF habilitado)
    sub  r12, 8
    mov  qword [r12], 0x08     ; CS (segmento de código del kernel)
    sub  r12, 8
    mov  [r12], r13            ; RIP = entry

    ; Error code ficticio (el handler lo descarta con add rsp, 8)
    sub  r12, 8
    mov  qword [r12], 0

    ; Registros guardados — orden inverso al push de _irq00Handler:
    ; push rbp,r15..r8,rdi,rsi,rdx,rcx,rbx,rax  →  pop rax primero ([rsp+0])
    sub  r12, 8
    mov  qword [r12], 0        ; rbp
    sub  r12, 8
    mov  qword [r12], 0        ; r15
    sub  r12, 8
    mov  qword [r12], 0        ; r14
    sub  r12, 8
    mov  qword [r12], 0        ; r13
    sub  r12, 8
    mov  qword [r12], 0        ; r12
    sub  r12, 8
    mov  qword [r12], 0        ; r11
    sub  r12, 8
    mov  qword [r12], 0        ; r10
    sub  r12, 8
    mov  qword [r12], 0        ; r9
    sub  r12, 8
    mov  qword [r12], 0        ; r8
    sub  r12, 8
    mov  [r12], rdx            ; rdi = argc  (1er arg de entry)
    sub  r12, 8
    mov  [r12], rcx            ; rsi = argv  (2do arg de entry)
    sub  r12, 8
    mov  qword [r12], 0        ; rdx
    sub  r12, 8
    mov  qword [r12], 0        ; rcx
    sub  r12, 8
    mov  qword [r12], 0        ; rbx
    sub  r12, 8
    mov  qword [r12], 0        ; rax

    mov  rax, r12              ; retornar el RSP inicial del proceso

    pop  r14
    pop  r13
    pop  r12
    pop  rbp
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
