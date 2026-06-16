#include <stdint.h>
#include "naiveConsole.h"
#include "syscall.h"
#include "keyboard.h"
#include "videoDriver.h"
#include "gfxConsole.h"
#include "libasm.h"
#include "scheduler.h"
#include "pipe.h"
#include "memory_manager.h"
#include "sem.h"

/* Debe coincidir con ProcessInfo en Userland/include/libc.h */
typedef struct {
    uint64_t pid;
    char     name[64];
    int      state;
    int      priority;
    int      foreground;
    uint64_t stack_base;
    uint64_t rsp;
} ProcessInfo;

/* Debe coincidir con CreateProcessArgs en Userland/include/libc.h */
typedef struct {
    int (*fn)(int, char**);
    int      argc;
    char   **argv;
    int      stdin_res;   /* 0=teclado, PIPE_FD_BASE+id=pipe */
    int      stdout_res;  /* 1=pantalla, PIPE_FD_BASE+id=pipe */
} CreateProcessArgs;

/*
 * Todos los handlers comparten esta firma para poder vivir en la jump table.
 * Cada uno castea/ignora los argumentos que necesita (a1, a2, a3 vienen crudos
 * desde el stub int 0x80). Así el dispatcher no usa un switch/case: indexa la
 * tabla por id y hace un call indirecto.
 */
typedef uint64_t (*syscall_fn)(uint64_t a1, uint64_t a2, uint64_t a3);


/* PID del proceso en foreground — para Ctrl+C desde el keyboard driver */
static uint64_t kernel_foreground_pid = 0;
void set_kernel_foreground(uint64_t pid) { kernel_foreground_pid = pid; }
void kill_foreground(void) {
    if (kernel_foreground_pid) {
        killProcess(kernel_foreground_pid);
        kernel_foreground_pid = 0;
    }
}

extern uint64_t exc_resume_rip;
extern uint64_t read_tsc_asm(void);

// Forward declaration: implemented in Kernel/cdrivers/regs.c
void regs_print(void);

static uint64_t sys_set_exc_resume(uint64_t a1, uint64_t a2, uint64_t a3){
    (void)a2; (void)a3;
    exc_resume_rip = a1;
    return 0;
}

static uint64_t sys_pipe_open(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)pipe_open((int)a1);
}

static uint64_t sys_pipe_close_write(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    pipe_close_write((int)a1);
    return 0;
}

static uint64_t sys_pipe_close_read(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    pipe_close_read((int)a1);
    return 0;
}

/*
 * Redirige el fd[fd_slot] del proceso actual al pipe con pipe_id.
 * fd_slot 0 = stdin, fd_slot 1 = stdout.
 * Así el proceso pasa a leer/escribir del pipe sin cambiar su código.
 */
static uint64_t sys_pipe_set_fd(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    int pipe_id = (int)a1;
    int fd_slot = (int)a2;
    PCB *p = getCurrentProcess();
    if (!p || fd_slot < 0 || fd_slot > 1)
        return (uint64_t)-1;
    p->fd[fd_slot] = PIPE_ID_TO_FD(pipe_id);
    return 0;
}

static uint64_t sys_exit(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    exitCurrentProcess((int)a1);
    return 0;  // inalcanzable
}

/* Helper interno (no es una syscall): escribe n bytes en pantalla. */
static uint64_t sys_write_screen(const char *buf, size_t n)
{
    static char kernel_buffer[8192];
    if (n > sizeof(kernel_buffer) - 1)
        n = sizeof(kernel_buffer) - 1;

    /* kernel_buffer y el estado del cursor del driver son compartidos por todos
     * los procesos. Sin protección, dos write() concurrentes (uno preemptado a
     * mitad) se pisan el buffer / corrompen el cursor -> race condition.
     * La sección es corta, así que alcanza con deshabilitar interrupciones
     * (no usamos semáforo: un mutex acá podría quedar trabado si se mata al
     * proceso mientras lo tiene tomado, congelando TODA la salida). */
    uint64_t flags = irq_save();

    memcpy(kernel_buffer, buf, n);
    kernel_buffer[n] = '\0';

    uint64_t ret;
    if (videoIsLFB()) {
        ret = (uint64_t)gfx_write(kernel_buffer, n);
    } else {
        for (size_t i = 0; i < n; i++) {
            char c = kernel_buffer[i];
            if (c == '\n') ncNewline();
            else           ncPrintChar(c);
        }
        ret = (uint64_t)n;
    }

    irq_restore(flags);
    return ret;
}

static uint64_t sys_write(uint64_t a1, uint64_t a2, uint64_t a3)
{
    uint64_t fd = a1;
    const char *buf = (const char *)a2;
    uint64_t len = a3;

    if (buf == 0 || fd > 1)
        return 0;

    size_t n = (size_t)len;
    if (n == 0) {
        const char *p = buf;
        size_t max = 4096;
        while (max-- && *p) p++;
        n = (size_t)(p - buf);
    }
    if (n == 0)
        return 0;

    /* Consultar la tabla de FDs del proceso actual para saber a qué recurso
     * apunta este descriptor. Un proceso no sabe si escribe en pantalla o pipe. */
    PCB *current = getCurrentProcess();
    int resource = (current != NULL) ? current->fd[(int)fd] : (int)fd;

    if (IS_PIPE_FD(resource))
        return (uint64_t)pipe_write(PIPE_FD_TO_ID(resource), buf, (int)n);

    return sys_write_screen(buf, n);
}

static uint64_t sys_clear(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
        gfx_clear();
    else
        ncClear();
    return 0;
}

static uint64_t sys_read(uint64_t a1, uint64_t a2, uint64_t a3)
{
    uint64_t fd = a1;
    char *buf = (char *)a2;
    uint64_t len = a3;

    if (fd > 1 || buf == 0 || len == 0)
        return 0;

    /* Consultar la tabla de FDs del proceso actual */
    PCB *current = getCurrentProcess();
    int resource = (current != NULL) ? current->fd[(int)fd] : (int)fd;

    if (IS_PIPE_FD(resource))
        return (uint64_t)pipe_read(PIPE_FD_TO_ID(resource), buf, (int)len);

    /* Recurso 0 = teclado: bloquear (sin busy-wait) hasta tener un carácter.
     * kbd_read_blocking hace sem_wait sobre el semáforo del teclado; el ISR hace
     * sem_post por cada tecla, así el proceso queda BLOCKED (no gira) mientras espera. */
    char c = kbd_read_blocking();

    if (c == 0x04) {            /* Ctrl+D = EOF */
        sys_write_screen("\n", 1);
        return 0;
    }
    if (c == 0x03) {            /* Ctrl+C (normalmente ya lo atrapa el ISR antes de encolar) */
        kill_foreground();
        return 0;
    }
    buf[0] = c;
    return 1;
}

static uint64_t sys_read_tsc(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    return read_tsc_asm();
}

static uint64_t sys_set_text_color(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a2; (void)a3;
    if (videoIsLFB())
    {
        gfx_set_fg((uint32_t)a1);
        vdSetColor((uint32_t)a1);
    }
    return 0;
}

static uint64_t sys_set_text_color_name(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a2; (void)a3;
    const char *name = (const char *)a1;
    if (!name)
        return (uint64_t)-1;
    if (videoIsLFB())
    {
        uint32_t rgb = vdGetColorByName(name);
        gfx_set_fg(rgb);
        vdSetColor(rgb);
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_print_available_colors(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
    {
        vdPrintAvailableColors();
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_regs_print(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
    {
        regs_print();
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_set_colors(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a3;
    if (videoIsLFB())
    {
        gfx_set_colors((uint32_t)a1, (uint32_t)a2);
        vdSetColor((uint32_t)a1);
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_get_shell_cols(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
        return (uint64_t)vdGetShellCols();
    return (uint64_t)0;
}

static uint64_t sys_get_shell_rows(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
        return (uint64_t)vdGetShellRows();
    return (uint64_t)0;
}

static uint64_t sys_kbd_available(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    return (uint64_t)kbd_available();
}

static uint64_t sys_get_color_by_name(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a2; (void)a3;
    if (videoIsLFB())
        return (uint64_t)vdGetColorByName((const char *)a1);
    return (uint64_t)0xFFFFFF;
}

static uint64_t sys_get_screen_px_width(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
        return (uint64_t)vdGetScreenWidth();
    return (uint64_t)0;
}

static uint64_t sys_get_screen_px_height(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a1; (void)a2; (void)a3;
    if (videoIsLFB())
        return (uint64_t)vdGetScreenHeight();
    return (uint64_t)0;
}

static uint64_t sys_gfx_fill_blended(uint64_t a1, uint64_t a2, uint64_t a3)
{
    if (!videoIsLFB())
        return (uint64_t)-1;

    /* a1 = x, a2 = y, a3 = pointer to args { uint32_t w,h,color,alpha } */
    struct blend_args {
        uint32_t w;
        uint32_t h;
        uint32_t color;
        uint32_t alpha;
    };
    const struct blend_args *args = (const struct blend_args *)a3;
    if (!args)
        return (uint64_t)-1;

    drawRectFillBlend((uint32_t)args->color, (uint64_t)a1, (uint64_t)a2,
                      (uint64_t)args->w, (uint64_t)args->h, (uint8_t)args->alpha);
    return (uint64_t)0;
}

static inline uint8_t cmos_read(uint8_t reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t b)
{
    return (uint8_t)((b & 0x0F) + ((b >> 4) * 10));
}

static uint64_t sys_time(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    // Read RTC seconds, minutes, hours, day, month, year; handle BCD vs binary
    uint8_t sec = cmos_read(0x00);
    uint8_t min = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t day = cmos_read(0x07);
    uint8_t mon = cmos_read(0x08);
    uint8_t yr = cmos_read(0x09); // last two digits
    uint8_t regB = cmos_read(0x0B);
    int is_bcd = ((regB & 0x04) == 0); // bit 2 = 0 -> BCD
    int is_24h = ((regB & 0x02) != 0); // bit 1 = 1 -> 24-hour
    if (is_bcd)
    {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        yr = bcd_to_bin(yr);
    }
    if (!is_24h)
    {
        // Convert 12h to 24h if needed (bit 7 is PM when in 12-hour mode)
        uint8_t pm = (hour & 0x80) != 0;
        hour &= 0x7F;
        if (pm && hour < 12)
            hour = (uint8_t)(hour + 12);
        if (!pm && hour == 12)
            hour = 0;
    }
    // Pack as Y(8)-M(8)-D(8)-H(8)-m(8)-S(8), year is 2000+yr
    uint8_t fullY = (uint8_t)(2000 + yr - 2000); // store last 2 digits or 2000-based offset
    uint64_t packed = ((uint64_t)fullY << 40) | ((uint64_t)mon << 32) | ((uint64_t)day << 24) |
                      ((uint64_t)hour << 16) | ((uint64_t)min << 8) | (uint64_t)sec;
    return packed;
}

static uint64_t sys_set_text_size(uint64_t a1, uint64_t a2, uint64_t a3)
{
    (void)a2; (void)a3;
    int m = (int)a1;
    if (m < 0 || m > 2)
        return (uint64_t)-1;

    if (videoIsLFB()) {
        int r = vdSetFontSize((uint8_t)m);
        return (r == 0) ? (uint64_t)0 : (uint64_t)-1;
    }
    return (uint64_t)-1;
}

/* ---------- syscalls de gestión de procesos ---------- */

static uint64_t sys_create_process(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    CreateProcessArgs *a = (CreateProcessArgs *)a1;
    if (!a || !a->fn) return (uint64_t)-1;
    /* Usar argv[0] como nombre del proceso para que ps muestre el comando real */
    const char *name = (a->argc > 0 && a->argv && a->argv[0]) ? a->argv[0] : "proc";
    PCB *pcb = createProcess(name, (ProcessMain)a->fn, a->argc, a->argv, 0, 0);
    if (!pcb) return (uint64_t)-1;
    pcb->fd[0] = a->stdin_res;
    pcb->fd[1] = a->stdout_res;
    addProcess(pcb);
    return (uint64_t)pcb->pid;
}

static uint64_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    return getCurrentPID();
}

static uint64_t sys_waitpid(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)waitForProcess(a1);
}

static uint64_t sys_kill(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    uint64_t pid = a1;
    /* idle (PID 1) y shell (PID 2) son intocables — matarlos freezea el sistema */
    if (pid <= 2)
        return (uint64_t)-1;
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    killProcess(pid);
    return 0;
}

static uint64_t sys_block(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    uint64_t pid = a1;
    if (pid <= 2)                   /* idle y shell no son bloqueables desde userland */
        return (uint64_t)-1;
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    /* Bloqueo MANUAL: marca paused (no toca el bloqueo por semáforo/pipe/waitpid). */
    pauseProcess(pid);
    return 0;
}

static uint64_t sys_unblock(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    uint64_t pid = a1;
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    /* Desbloqueo MANUAL: solo limpia paused. Un proceso dormido en un semáforo
     * NO se despierta por acá (sigue con state == BLOCKED). */
    resumeProcess(pid);
    return 0;
}

static uint64_t sys_nice(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    uint64_t pid = a1;
    uint64_t priority = a2;
    if (pid <= 2)                   /* idle y shell tienen prioridad fija */
        return (uint64_t)-1;
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    if (priority > MAX_PRIORITY)    /* rango válido: [0, MAX_QUANTUMS-1] */
        return (uint64_t)-1;
    setPriority(pid, (int)priority);
    return 0;
}

/*
 * Llena buf[] con información de hasta max procesos.
 * Retorna la cantidad real de procesos en la lista.
 * Usado por el comando ps de la shell.
 */
static uint64_t sys_get_processes(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    ProcessInfo *buf = (ProcessInfo *)a1;
    uint64_t max = a2;
    PCB *head = getHeadProcess();
    if (!head || !buf || max == 0)
        return 0;
    uint64_t count = 0;
    PCB *p = head;
    do {
        if (count < max) {
            buf[count].pid        = p->pid;
            buf[count].state      = (int)p->state;
            buf[count].priority   = p->priority;
            buf[count].foreground = p->foreground;
            buf[count].stack_base = p->stackBase;
            buf[count].rsp        = p->rsp;
            int i;
            for (i = 0; i < 63 && p->name[i]; i++)
                buf[count].name[i] = p->name[i];
            buf[count].name[i] = '\0';
        }
        count++;
        p = p->next;
    } while (p != head);
    return count;
}

static uint64_t sys_yield(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    yieldProcess();
    return 0;
}

static uint64_t sys_get_mem_info(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    mem_info_t *out = (mem_info_t *)a1;
    if (!out) return (uint64_t)-1;
    *out = sys_mem_info();
    return 0;
}

static uint64_t sys_set_foreground(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    set_kernel_foreground(a1);
    return 0;
}

static uint64_t sys_kmalloc(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)sys_malloc((size_t)a1);
}

static uint64_t sys_kfree(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    sys_free((void *)a1);
    return 0;
}

static uint64_t sys_sem_open(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    return (uint64_t)sem_open((const char *)a1, a2);
}

static uint64_t sys_sem_wait(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)sem_wait((int)a1);
}

static uint64_t sys_sem_post(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)sem_post((int)a1);
}

static uint64_t sys_sem_close(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    return (uint64_t)sem_close((int)a1);
}
/* ----------------------------------------------------- */

/*
 * Jump table: cada id mapea a su handler. Los huecos del enum (4, 24, 41, ...)
 * y cualquier id fuera de rango quedan en NULL gracias a los designated
 * initializers, y el dispatcher los trata como ENOSYS. El indexado por id +
 * call indirecto reemplaza al switch/case y mantiene la complejidad ciclomática
 * del dispatcher en O(1).
 */
static const syscall_fn syscall_table[] = {
    [SYS_WRITE]                 = sys_write,
    [SYS_CLEAR]                 = sys_clear,
    [SYS_READ]                  = sys_read,
    [SYS_TIME]                  = sys_time,
    [SYS_SET_TEXT_COLOR]        = sys_set_text_color,
    [SYS_SET_TEXT_COLOR_NAME]   = sys_set_text_color_name,
    [SYS_PRINT_AVAILABLE_COLORS]= sys_print_available_colors,
    [SYS_REGS_PRINT]            = sys_regs_print,
    [SYS_SET_COLORS]            = sys_set_colors,
    [SYS_GET_SHELL_COLS]        = sys_get_shell_cols,
    [SYS_GET_SHELL_ROWS]        = sys_get_shell_rows,
    [SYS_KBD_AVAILABLE]         = sys_kbd_available,
    [SYS_GET_COLOR_BY_NAME]     = sys_get_color_by_name,
    [SYS_GFX_FILL_BLENDED]      = sys_gfx_fill_blended,
    [SYS_GET_SCREEN_PX_WIDTH]   = sys_get_screen_px_width,
    [SYS_GET_SCREEN_PX_HEIGHT]  = sys_get_screen_px_height,
    [SYS_SET_TEXT_SIZE]         = sys_set_text_size,
    [SYS_SET_EXC_RESUME]        = sys_set_exc_resume,
    [SYS_READ_TSC]              = sys_read_tsc,

    [SYS_MALLOC]                = sys_kmalloc,
    [SYS_FREE]                  = sys_kfree,
    [SYS_MEM_INFO]              = sys_get_mem_info,

    [SYS_PIPE_OPEN]             = sys_pipe_open,
    [SYS_PIPE_CLOSE_WRITE]      = sys_pipe_close_write,
    [SYS_PIPE_CLOSE_READ]       = sys_pipe_close_read,
    [SYS_PIPE_SET_FD]           = sys_pipe_set_fd,

    [SYS_CREATE_PROCESS]        = sys_create_process,
    [SYS_GETPID]                = sys_getpid,
    [SYS_KILL]                  = sys_kill,
    [SYS_BLOCK]                 = sys_block,
    [SYS_UNBLOCK]               = sys_unblock,
    [SYS_NICE]                  = sys_nice,
    [SYS_GET_PROCESSES]         = sys_get_processes,
    [SYS_YIELD]                 = sys_yield,

    [SYS_SEM_OPEN]              = sys_sem_open,
    [SYS_SEM_WAIT]              = sys_sem_wait,
    [SYS_SEM_POST]              = sys_sem_post,
    [SYS_SEM_CLOSE]             = sys_sem_close,

    [SYS_SET_FOREGROUND]        = sys_set_foreground,

    [SYS_EXIT]                  = sys_exit,
    [SYS_WAITPID]               = sys_waitpid,
};

#define SYSCALL_COUNT (sizeof(syscall_table) / sizeof(syscall_table[0]))

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3)
{
    if (id >= SYSCALL_COUNT || syscall_table[id] == NULL)
        return (uint64_t)-1;   /* ENOSYS */
    return syscall_table[id](a1, a2, a3);
}

void syscall_init(void)
{
    // No runtime initialization required at the moment; IDT gate for int 0x80
    // is set up in assembly (idt.asm). This symbol exists so kernel link succeeds
    // and future syscall table initialization can be placed here.
}
