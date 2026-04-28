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