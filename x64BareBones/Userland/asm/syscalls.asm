
GLOBAL sys_3p
GLOBAL sys_1p
GLOBAL sys_raw
GLOBAL sys_0p

SECTION .text
; Generic 'callers'(formally called studs) that executes "int 0x80". Stubs are like bridges(or links) between the Userspace and KernelSpace.

;uint64_t sys_3p(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3)
sys_3p:
    mov     rax, rdi
    ; Shuffle arguments into expected registers
    mov     rdi, rsi    ; a1 -> RDI
    mov     rsi, rdx    ; a2 -> RSI
    mov     rdx, rcx    ; a3 -> RDX
    int     0x80
    ret

; uint64_t sys_1p(uint64_t id, uint64_t a1)
sys_1p:
    mov     rax, rdi    ; id -> RAX
    mov     rdi, rsi    ; a1 -> RDI
    int     0x80
    ret

; Generic raw syscall: rax=id, rdi=a1, rsi=a2, rdx=a3 (caller sets rax)
sys_raw:
    int     0x80
    ret

; uint64_t sys_0p(uint64_t id)
sys_0p:
    mov     rax, rdi
    int     0x80
    ret
