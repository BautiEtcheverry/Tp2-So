#ifndef USERLAND_LIBC_H
#define USERLAND_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
/*
    Making static inline functions(small ones) essentialy gives each file that includes this header a copy of the function, which can be inlined by the compiler.
    More info about inline functions in C: https://wiki.osdev.org/index.php?search=Inline+Functions+in+C&title=Special%3ASearch&profile=default&fulltext=1
*/

typedef struct {
    size_t total_memory;
    size_t used_memory;
    size_t free_memory;
    size_t allocated_blocks;
} mem_info_t;
/*
    Syscall numbers must match the ones in the kernel in the file: /x64BareBones/Kernel/include/syscall.h
*/
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
    SYS_SET_TEXT_SIZE=18,
    SYS_SET_EXC_RESUME=19,
    SYS_READ_TSC = 20,

    /*Memoria*/
    SYS_MALLOC = 21,                // malloc(size) -> puntero (0 si falla)
    SYS_FREE = 22,                  
    SYS_MEM_INFO = 23,  

    /* Pipes */
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

/* Recurso de pipe: fd value >= PIPE_FD_BASE es un pipe */
#define PIPE_FD_BASE      2
#define PIPE_ID_TO_FD(id) ((id) + PIPE_FD_BASE)

/* Structs compartidos kernel/userland — deben coincidir con Kernel/syscall.c */
typedef struct {
    uint64_t pid;
    char     name[64];
    int      state;        /* 0=READY 1=RUNNING 2=BLOCKED 3=DEAD */
    int      priority;
    int      foreground;
    uint64_t stack_base;
    uint64_t rsp;
} ProcessInfo;

typedef struct {
    int    (*fn)(int, char**);
    int      argc;
    char   **argv;
    int      stdin_res;   /* 0=teclado, PIPE_ID_TO_FD(id)=pipe */
    int      stdout_res;  /* 1=pantalla, PIPE_ID_TO_FD(id)=pipe */
} CreateProcessArgs;

// Implemented in Userland/Shell/syscall.asm to avoid inline asm ()
/*------------------------------------------------------------------*/
uint64_t sys_3p(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t sys_0p(uint64_t id);
uint64_t sys_1p(uint64_t id, uint64_t a1);
uint64_t sys_raw(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3);
/*------------------------------------------------------------------*/

// Fill rectangle with blending
static inline int fill_rect_blend(int x, int y, int w, int h, uint32_t color, uint8_t alpha){
    struct blend_args {
        uint32_t w;
        uint32_t h;
        uint32_t color;
        uint32_t alpha;
    } args;
    args.w = (uint32_t)w;
    args.h = (uint32_t)h;
    args.color = color;
    args.alpha = (uint32_t)alpha;
    return (int)sys_3p(SYS_GFX_FILL_BLENDED, (uint64_t)x, (uint64_t)y, (uint64_t)&args);
}

/*-----------------------------------------------------------*/
static inline int get_screen_px_width(void){
    return (int)sys_3p(SYS_GET_SCREEN_PX_WIDTH, 0, 0, 0);
}
static inline int get_screen_px_height(void){
    return (int)sys_3p(SYS_GET_SCREEN_PX_HEIGHT, 0, 0, 0);
}
/*-----------------------------------------------------------*/
int printf(const char *fmt, ...);
uint64_t str_to_uint(const char *s);
int streq_nocase(const char *a, const char *b);
size_t strlen(const char *s);

static inline uint64_t write(int fd, const char *buf, size_t len){
    return sys_3p(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

static inline void clear_screen(void){
    (void)sys_1p(SYS_CLEAR, 0);
}

static inline void puts(const char *s){
    // compute length
    size_t n = 0;
    while (s[n])
        n++;
    write(1, s, n);
}

// Absolute value function
static inline int absVal(int x){
    return (x < 0) ? -x : x;
}

static inline uint64_t read(int fd, char *buf, size_t len){
    return sys_3p(SYS_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

static inline void exit(int status){
    (void)sys_3p(SYS_EXIT, (uint64_t)status, 0, 0);
}

static inline int waitpid(uint64_t pid) {
    return (int) sys_1p(SYS_WAITPID, pid);
}

static inline void set_text_color(uint32_t rgb){
    (void)sys_3p(SYS_SET_TEXT_COLOR, (uint64_t)rgb, 0, 0);
}

static inline void set_colors(uint32_t fg, uint32_t bg){
    (void)sys_3p(SYS_SET_COLORS, (uint64_t)fg, (uint64_t)bg, 0);
}

static inline int get_shell_cols(void){
    return (int)sys_3p(SYS_GET_SHELL_COLS, 0, 0, 0);
}

static inline int get_shell_rows(void){
    return (int)sys_3p(SYS_GET_SHELL_ROWS, 0, 0, 0);
}

static inline int set_text_color_name(const char *name){
    return (int)sys_3p(SYS_SET_TEXT_COLOR_NAME, (uint64_t)name, 0, 0);
}

static inline void print_available_text_colors(void){
    (void)sys_3p(SYS_PRINT_AVAILABLE_COLORS, 0, 0, 0);
}

static inline void regs_print(void){
    (void)sys_3p(SYS_REGS_PRINT, 0, 0, 0);
}

static inline int getchar(void){
    char c;
    if (read(0, &c, 1) == 0) return -1;  /* EOF: Ctrl+D o pipe cerrado */
    return (int)(unsigned char)c;
}

static inline int kbd_available(void) {
    return (int)sys_1p(SYS_KBD_AVAILABLE, 0);
}

static inline uint32_t get_color_by_name(const char *name) {
    return (uint32_t)sys_3p(SYS_GET_COLOR_BY_NAME, (uint64_t)name, 0, 0);
}

static inline void time_hms(unsigned *h, unsigned *m, unsigned *s){
    uint64_t t = sys_3p(SYS_TIME, 0, 0, 0);
    if (h)
        *h = (unsigned)((t >> 16) & 0xFF);
    if (m)
        *m = (unsigned)((t >> 8) & 0xFF);
    if (s)
        *s = (unsigned)(t & 0xFF);
}

static inline void time_date_hms(unsigned *d, unsigned *mo, unsigned *y, unsigned *h, unsigned *mi, unsigned *s){
    uint64_t t = sys_3p(SYS_TIME, 0, 0, 0);
    if (d)
        *d = (unsigned)((t >> 24) & 0xFF);
    if (mo)
        *mo = (unsigned)((t >> 32) & 0xFF);
    if (y)
        *y = 2000u + (unsigned)((t >> 40) & 0xFF);
    if (h)
        *h = (unsigned)((t >> 16) & 0xFF);
    if (mi)
        *mi = (unsigned)((t >> 8) & 0xFF);
    if (s)
        *s = (unsigned)(t & 0xFF);
}

static inline size_t readline(char *buf, size_t max){
    size_t n = 0;
    while (n + 1 < max)
    {
        int ch = getchar();
        if (ch == -1) break;  /* EOF (Ctrl+D) */
        if (ch == '\r')
            ch = '\n';
        if (ch == '\n')
        {
            buf[n++] = '\n';
            buf[n] = 0;
            puts("\n");
            break;
        }
        if (ch == '\b' || ch == 127)
        {
            if (n > 0)
            {
                n--;
                puts("\b \b");
            }
            continue;
        }
        buf[n++] = (char)ch;
        char echo[2] = {(char)ch, 0};
        puts(echo);
    }
    buf[n] = 0;
    return n;
}

//Text sizes mapping: 0=default (base), 1=large, 2=xlarge
static inline int set_text_size(int mode) {
    return (int)sys_1p(SYS_SET_TEXT_SIZE, (uint64_t)mode);
}

static inline void set_exc_resume(void *addr){
    sys_1p(SYS_SET_EXC_RESUME, (uint64_t)addr);
}

/* ------------------------------------------------------------------ */
/* Pipes                                                                */
/* ------------------------------------------------------------------ */

/*
 * pipe_open(id):
 *   id == -1  → pipe anónimo (para shell |), el kernel asigna un ID libre
 *   id >= 0   → pipe nombrado (dos procesos no relacionados acuerdan este ID)
 * Retorna el pipe_id asignado, o -1 si no hay slots libres.
 */
static inline int pipe_open(int id) {
    return (int)sys_1p(SYS_PIPE_OPEN, (uint64_t)id);
}

/* Cierra el extremo escritor. El lector recibirá EOF cuando consuma lo que queda. */
static inline void pipe_close_write(int pipe_id) {
    sys_1p(SYS_PIPE_CLOSE_WRITE, (uint64_t)pipe_id);
}

/* Cierra el extremo lector. El escritor recibirá error (broken pipe). */
static inline void pipe_close_read(int pipe_id) {
    sys_1p(SYS_PIPE_CLOSE_READ, (uint64_t)pipe_id);
}

/*
 * pipe_set_fd(pipe_id, fd_slot):
 *   Redirige el fd_slot del proceso actual al pipe.
 *   fd_slot=0 → stdin del proceso pasa a leer del pipe
 *   fd_slot=1 → stdout del proceso pasa a escribir en el pipe
 *
 *   Después de esto, el proceso usa read(0,...)/write(1,...) normalmente
 *   sin saber que está hablando con un pipe (transparencia).
 */
static inline int pipe_set_fd(int pipe_id, int fd_slot) {
    return (int)sys_3p(SYS_PIPE_SET_FD, (uint64_t)pipe_id, (uint64_t)fd_slot, 0);
}

/* 
    Semáforos                                                            
/* 

/*
 * sem_open(name, initial_value):
 *   Abre un semáforo nombrado (procesos no relacionados lo comparten
 *   acordando el mismo `name`). Si ya existe, retorna su id (incrementa el
 *   refcount). 
 *   Retorna el id (>= 0) o -1 si no hay slots libres.
 */
static inline int sem_open(const char *name, uint64_t initial_value) {
    return (int)sys_3p(SYS_SEM_OPEN, (uint64_t)name, initial_value, 0);
}

/* decrementa; bloquea si el valor pasa a < 0. Retorna 0 o -1. */
static inline int sem_wait(int sem_id) {
    return (int)sys_1p(SYS_SEM_WAIT, (uint64_t)sem_id);
}

/* incrementa; despierta a un proceso bloqueado si lo hay. Rotorna 0 o -1. */
static inline int sem_post(int sem_id) {
    return (int)sys_1p(SYS_SEM_POST, (uint64_t)sem_id);
}

/* Cierra el semáforo (refcount--). Libera el slot al llegar a 0. Retorna 0 o -1. */
static inline int sem_close(int sem_id) {
    return (int)sys_1p(SYS_SEM_CLOSE, (uint64_t)sem_id);
}

/* ------------------------------------------------------------------ */
/* Yield, memoria, foreground                                           */
/* ------------------------------------------------------------------ */

static inline void yield(void) {
    sys_0p(SYS_YIELD);
}

/* Registra el PID del proceso foreground actual para que Ctrl+C lo mate. */
static inline void set_foreground(uint64_t pid) {
    sys_1p(SYS_SET_FOREGROUND, pid);
}

static inline void *mem_alloc(uint64_t size) {
    return (void *)sys_1p(SYS_MALLOC, size);
}

static inline void mem_free(void *ptr) {
    sys_1p(SYS_FREE, (uint64_t)ptr);
}

/* ------------------------------------------------------------------ */
/* Gestión de procesos                                                  */
/* ------------------------------------------------------------------ */

/* Crea proceso con stdin=teclado, stdout=pantalla (uso normal). */
static inline int64_t create_process(int (*fn)(int, char**), int argc, char **argv) {
    CreateProcessArgs a = { fn, argc, argv, 0, 1 };
    return (int64_t)sys_1p(SYS_CREATE_PROCESS, (uint64_t)&a);
}

/* Crea proceso con stdin/stdout redirigidos a un pipe. */
static inline int64_t create_process_piped(int (*fn)(int, char**), int argc, char **argv,
                                            int stdin_res, int stdout_res) {
    CreateProcessArgs a = { fn, argc, argv, stdin_res, stdout_res };
    return (int64_t)sys_1p(SYS_CREATE_PROCESS, (uint64_t)&a);
}

/* PID del proceso actual. */
static inline uint64_t getpid(void) {
    return sys_0p(SYS_GETPID);
}

/* Mata el proceso con ese PID. Retorna 0 o -1. */
static inline int kill(uint64_t pid) {
    return (int)sys_1p(SYS_KILL, pid);
}

/* Bloquea el proceso con ese PID. Retorna 0 o -1. */
static inline int block(uint64_t pid) {
    return (int)sys_1p(SYS_BLOCK, pid);
}

/* Desbloquea el proceso con ese PID. Retorna 0 o -1. */
static inline int unblock(uint64_t pid) {
    return (int)sys_1p(SYS_UNBLOCK, pid);
}

/* Cambia la prioridad del proceso (0=max). Retorna 0 o -1. */
static inline int nice(uint64_t pid, int priority) {
    return (int)sys_3p(SYS_NICE, pid, (uint64_t)priority, 0);
}

/* Llena buf[] con info de hasta max procesos. Retorna la cantidad total. */
static inline int get_processes(ProcessInfo *buf, int max) {
    return (int)sys_3p(SYS_GET_PROCESSES, (uint64_t)buf, (uint64_t)max, 0);
}

static inline void *myMalloc(uint64_t size){
    return (void *) sys_1p(SYS_MALLOC, size);
}

static inline void myFree(void *ptr){
    (void) sys_1p(SYS_FREE, (uint64_t) ptr);
}

static inline int mem_info(mem_info_t *dst){
    return (int) sys_1p(SYS_MEM_INFO, (uint64_t) dst);
}
#endif
