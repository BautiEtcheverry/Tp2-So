GLOBAL cpuVendor
GLOBAL isr_syscall_80
GLOBAL saveRegisters
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL sti_enable
GLOBAL inb
GLOBAL outb
GLOBAL inl
GLOBAL outl
GLOBAL cpu_halt
GLOBAL read_tsc_asm
GLOBAL irq_save
GLOBAL irq_restore

;Funcion para solucionar problema de pantalla negra por que al comenzar el rtc que en arqui
;usabamos para calcular los fps no para de interrumpir (INT 0x28) y como el loader.asm no hace
;cli antes de de hacer call initializeKernelBinary y call main, y el handler de para 0x28 sigue 
;siendo el de Pure64 se queda en un bucle infinito.
GLOBAL disable_rtc

extern syscall_dispatch
extern capture_definitiva

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

; int 0x80 handler: rax=syscall#, rdi=a1, rsi=a2, rdx=a3
isr_syscall_80:
	push rbp
	mov rbp, rsp
	push rbx
	push rcx
	push r8
	push r9
	push r10
	push r11

	; On entry:
	;   rax = syscall id
	;   rdi = a1, rsi = a2, rdx = a3 
	; Map to System V ABI for syscall_dispatch(id, a1, a2, a3)
	; For more information about 
	mov r10, rdi       ; save a1
	mov r11, rsi       ; save a2
	mov rbx, rdx       ; save a3 (use rbx as temp)
	mov rdi, rax       ; id -> rdi (1st arg)
	mov rsi, r10       ; a1 -> rsi (2nd arg)
	mov rdx, r11       ; a2 -> rdx (3rd arg)
	mov rcx, rbx       ; a3 -> rcx (4th arg)
	call syscall_dispatch
	; return value in rax

	pop r11
	pop r10
	pop r9
	pop r8
	pop rcx
	pop rbx
	mov rsp, rbp
	pop rbp
	iretq

; void saveRegisters(uint64_t* regs)
; Copy 19 qwords from capture_definitiva into the buffer pointed by RDI
saveRegisters:
	push rbp
	mov rbp, rsp
	push rsi
	push rcx
	mov rsi, capture_definitiva
	mov rcx, 19
.copy_loop:
	mov rax, [rsi]
	mov [rdi], rax
	add rsi, 8
	add rdi, 8
	loop .copy_loop
	pop rcx
	pop rsi
	mov rsp, rbp
	pop rbp
	ret

; -----------------------------------------------------------------------------
;Declaration form
;uint8_t inb(uint16_t port)
;   RDI: port
; Returns AL(1 byte register) zero-extended in EAX(higher bits in 0)
inb:
	mov dx, di
	in  al, dx
	movzx eax, al
	ret

; void outb(uint16_t port, uint8_t val)
;   RDI: port, RSI: val
outb:
	mov dx, di
	mov al, sil
	out dx, al
	ret

; uint32_t inl(uint16_t port)
;   RDI: port
inl:
	mov dx, di
	in  eax, dx
	ret

; void outl(uint16_t port, uint32_t val)
;   RDI: port, RSI: val
outl:
	mov dx, di
	mov eax, esi
	out dx, eax
	ret

; void cpu_halt(void)
cpu_halt:
	hlt
	ret
	
; void picMasterMask(uint8_t mask);
picMasterMask:
    mov dx, 0x21       ; PIC master
    mov al, dil        ; mask in AL
    out dx, al
    ret

; void picSlaveMask(uint8_t mask);
picSlaveMask:
    mov dx, 0xA1       ; PIC slave
    mov al, dil
    out dx, al
    ret
    
; void sti_enable(void);
sti_enable:
    sti
    ret

; uint64_t read_tsc_asm(void);
read_tsc_asm:
    rdtsc              ; devuelve lo en EAX, hi en EDX
    shl rdx, 32
    or  rax, rdx
    ret
disable_rtc:
mov al, 0x0B 
out 0x70, al ;escribo 0x0B en el puerto 0x70 → "selecciono el registro B"
in al, 0x71  ;leo del puerto 0x71 → al = contenido actual del registro B
             ;que el bit 6 de ese byte es el PIE, Periodic Interrupt Enable

and al, 0xBF ;0xBF = 1011 1111 → AND deja todos los bits igual menos el bit 6
mov bl,al   ; copio el valor modificado a bl para guardarlo

mov al, 0x0B    ; vuelvo a poner el índice del registro B en al
out 0x70, al    ; lo mando a 0x70 → reselecciono el registro B
mov al, bl      ; recupero el valor modificado que había guardado
out 0x71, al    ; lo escribo en 0x71 → reg B queda con PIE=0 (RTC ya no genera IRQ8)

mov al, 0x0C    ; al = 0x0C, índice del registro C del CMOS
out 0x70, al    ; selecciono el registro Cß
in  al, 0x71    ; leo C y descarto el valor → esto "limpia" el flag de interrupción
ret

; uint64_t irq_save(void) — guarda RFLAGS y deshabilita interrupciones (cli)
irq_save:
  pushfq ;Pushea los 64bits del resgistro rflags
  pop rax
  cli
  ret

; void irq_restore(uint64_t flags) — restaura RFLAGS (re-habilita IF solo si estaba en 1)
irq_restore:
push rdi ; En rdi recibimos el parametro flags(primer param se pasa por rdi)
popfq ;Pongo en RFLAGS el rsp, osea los falgs que acabo de pushear.
ret
