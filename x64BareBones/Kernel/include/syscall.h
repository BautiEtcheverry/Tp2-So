#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Syscall numbers
enum
{
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_READ = 3,
    SYS_TIME = 5,
    SYS_SET_TEXT_COLOR = 6,
    SYS_SET_TEXT_COLOR_NAME = 7,
    SYS_PRINT_AVAILABLE_COLORS = 8,
    SYS_REGS_PRINT = 9,
    SYS_SET_COLORS = 10,
    SYS_GET_SHELL_COLS = 11,
    SYS_GET_SHELL_ROWS = 12,
    SYS_KBD_AVAILABLE = 13,
    SYS_GET_COLOR_BY_NAME = 14,
    SYS_GFX_FILL_BLENDED = 15,
    SYS_GET_SCREEN_PX_WIDTH = 16,
    SYS_GET_SCREEN_PX_HEIGHT = 17,
    SYS_SET_TEXT_SIZE = 18,
    SYS_SET_EXC_RESUME = 19,
    SYS_READ_TSC = 20,

    /* Memoria */
    SYS_MALLOC = 21,
    SYS_FREE = 22,
    SYS_MEM_INFO = 23,

    /* Pipes — IPC */
    SYS_PIPE_OPEN = 25,
    SYS_PIPE_CLOSE_WRITE = 26,
    SYS_PIPE_CLOSE_READ = 27,
    SYS_PIPE_SET_FD = 28,

    /* Gestión de procesos */
    SYS_CREATE_PROCESS = 29,
    SYS_GETPID = 30,
    SYS_KILL = 31,
    SYS_BLOCK = 32,
    SYS_UNBLOCK = 33,
    SYS_NICE = 34,
    SYS_GET_PROCESSES = 35,
    SYS_YIELD = 36,

    /* Semáforos */
    SYS_SEM_OPEN  = 37,
    SYS_SEM_WAIT  = 38,
    SYS_SEM_POST  = 39,
    SYS_SEM_CLOSE = 40,

    SYS_SET_FOREGROUND = 42,

    SYS_EXIT = 60,
    SYS_WAITPID = 61
};

// Kernel-side API
void syscall_init(void);
uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3);

#endif
