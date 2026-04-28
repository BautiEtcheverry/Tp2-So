GLOBAL trigger_div0
GLOBAL trigger_ud

extern sys_1p
%define SYS_SET_EXC_RESUME 19
section .text

; void trigger_div0(void)
trigger_div0:
    lea rsi, [rel .after]     ; rdi = resume addr
    mov rdi, SYS_SET_EXC_RESUME
    call sys_1p
    xor edx, edx              ; 64-bit dividend: RDX:RAX
    xor eax, eax              ; eax = 0 -> div by 0
    div eax                   ; #DE
.after:
    ret

; void trigger_ud(void)
trigger_ud:
    lea rsi, [rel .after]
    mov rdi, SYS_SET_EXC_RESUME
    call sys_1p
    ud2                       ; #IO
.after:
    ret