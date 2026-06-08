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

static uint64_t sys_set_exc_resume(uint64_t rip){
    exc_resume_rip = rip;
    return 0;
}

static uint64_t sys_pipe_open(int id) {
    return (uint64_t)pipe_open(id);
}

static uint64_t sys_pipe_close_write(int id) {
    pipe_close_write(id);
    return 0;
}

static uint64_t sys_pipe_close_read(int id) {
    pipe_close_read(id);
    return 0;
}

/*
 * Redirige el fd[fd_slot] del proceso actual al pipe con pipe_id.
 * fd_slot 0 = stdin, fd_slot 1 = stdout.
 * Así el proceso pasa a leer/escribir del pipe sin cambiar su código.
 */
static uint64_t sys_pipe_set_fd(int pipe_id, int fd_slot) {
    PCB *p = getCurrentProcess();
    if (!p || fd_slot < 0 || fd_slot > 1)
        return (uint64_t)-1;
    p->fd[fd_slot] = PIPE_ID_TO_FD(pipe_id);
    return 0;
}

static uint64_t sys_exit(uint64_t status) {
    exitCurrentProcess((int)status);
    return 0;  // inalcanzable
}

static uint64_t sys_write_screen(const char *buf, size_t n)
{
    static char kernel_buffer[8192];
    if (n > sizeof(kernel_buffer) - 1)
        n = sizeof(kernel_buffer) - 1;
    memcpy(kernel_buffer, buf, n);
    kernel_buffer[n] = '\0';

    if (videoIsLFB())
        return (uint64_t)gfx_write(kernel_buffer, n);

    for (size_t i = 0; i < n; i++) {
        char c = kernel_buffer[i];
        if (c == '\n') ncNewline();
        else           ncPrintChar(c);
    }
    return (uint64_t)n;
}

static uint64_t sys_write(uint64_t fd, const char *buf, uint64_t len)
{
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

static uint64_t sys_clear(void)
{
    if (videoIsLFB())
        gfx_clear();
    else
        ncClear();
    return 0;
}

static uint64_t sys_read(uint64_t fd, char *buf, uint64_t len)
{
    if (fd > 1 || buf == 0 || len == 0)
        return 0;

    /* Consultar la tabla de FDs del proceso actual */
    PCB *current = getCurrentProcess();
    int resource = (current != NULL) ? current->fd[(int)fd] : (int)fd;

    if (IS_PIPE_FD(resource))
        return (uint64_t)pipe_read(PIPE_FD_TO_ID(resource), buf, (int)len);

    /* Ctrl+D pendiente del read anterior — devolver EOF */
    static int kbd_eof_pending = 0;
    if (kbd_eof_pending) {
        kbd_eof_pending = 0;
        return 0;
    }

    /* Recurso 0 = teclado */
    while (kbd_available() == 0) { }
    size_t n = kbd_read(buf, len);

    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0x04) {
            /* Newline visual al Ctrl+D para que el cursor quede en línea nueva.
             * Solo si había texto antes (i > 0) o el cursor no estaba en nueva línea. */
            sys_write_screen("\n", 1);
            if (i > 0) kbd_eof_pending = 1;
            return (uint64_t)i;
        }
        if (buf[i] == 0x03) {
            kill_foreground();
            return (uint64_t)i;
        }
    }
    return (uint64_t)n;
}


static uint64_t sys_read_tsc(void) {
    return read_tsc_asm();
}

static uint64_t sys_set_text_color(uint64_t rgb)
{
    if (videoIsLFB())
{
        gfx_set_fg((uint32_t)rgb);
        vdSetColor((uint32_t)rgb);
    } else{
        (void)rgb;
    }
    return 0;
}

static uint64_t sys_set_text_color_name(const char *name)
{
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

static uint64_t sys_print_available_colors(void)
{
    if (videoIsLFB())
    {
        vdPrintAvailableColors();
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_regs_print(void)
{
    if (videoIsLFB())
    {
        regs_print();
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_set_colors(uint64_t fg, uint64_t bg)
{
    if (videoIsLFB())
    {
        gfx_set_colors((uint32_t)fg, (uint32_t)bg);
        vdSetColor((uint32_t)fg);
        return 0;
    }
    return (uint64_t)-1;
}

static uint64_t sys_get_shell_cols(void)
{
    if (videoIsLFB())
        return (uint64_t)vdGetShellCols();
    return (uint64_t)0;
}

static uint64_t sys_get_shell_rows(void)
{
    if (videoIsLFB())
        return (uint64_t)vdGetShellRows();
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

static uint64_t sys_time(void) {
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
static uint64_t sys_set_text_size(uint64_t mode)
{
    int m = (int)mode;
    if (m < 0 || m > 2)
        return (uint64_t)-1;

    if (videoIsLFB()) {
        int r = vdSetFontSize((uint8_t)m);
        return (r == 0) ? (uint64_t)0 : (uint64_t)-1;
    }
    return (uint64_t)-1;
}

/* ---------- syscalls de gestión de procesos ---------- */

static uint64_t sys_create_process(uint64_t args_ptr) {
    CreateProcessArgs *a = (CreateProcessArgs *)args_ptr;
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

static uint64_t sys_getpid(void) {
    return getCurrentPID();
}

static uint64_t sys_kill(uint64_t pid) {
    /* idle (PID 1) y shell (PID 2) son intocables — matarlos freezea el sistema */
    if (pid <= 2)
        return (uint64_t)-1;
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    killProcess(pid);
    return 0;
}

static uint64_t sys_block(uint64_t pid) {
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    blockProcess(pid);
    return 0;
}

static uint64_t sys_unblock(uint64_t pid) {
    PCB *p = findProcess(pid);
    if (!p || p->state != BLOCKED)
        return (uint64_t)-1;
    unblockProcess(pid);
    return 0;
}

static uint64_t sys_nice(uint64_t pid, uint64_t priority) {
    PCB *p = findProcess(pid);
    if (!p || p->state == DEAD)
        return (uint64_t)-1;
    setPriority(pid, (int)priority);
    return 0;
}

/*
 * Llena buf[] con información de hasta max procesos.
 * Retorna la cantidad real de procesos en la lista.
 * Usado por el comando ps de la shell.
 */
static uint64_t sys_get_processes(ProcessInfo *buf, uint64_t max) {
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

static uint64_t sys_yield(void) {
    yieldProcess();
    return 0;
}

static uint64_t sys_get_mem_info(uint64_t buf_ptr) {
    mem_info_t *out = (mem_info_t *)buf_ptr;
    if (!out) return (uint64_t)-1;
    *out = sys_mem_info();
    return 0;
}

static uint64_t sys_set_foreground(uint64_t pid) {
    set_kernel_foreground(pid);
    return 0;
}

static uint64_t sys_kmalloc(uint64_t size) {
    return (uint64_t)sys_malloc((size_t)size);
}

static uint64_t sys_kfree(uint64_t ptr) {
    sys_free((void *)ptr);
    return 0;
}

static uint64_t sys_sem_open(const char *name, uint64_t init) {
    return (uint64_t) sem_open(name, init);
}

static uint64_t sys_sem_wait(int id) {
    return (uint64_t) sem_wait(id);
}

static uint64_t sys_sem_post(int id) {
    return (uint64_t) sem_post(id);
}

static uint64_t sys_sem_close(int id) {
    return (uint64_t) sem_close(id);
}
/* ----------------------------------------------------- */

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3)
{
    switch (id)
    {
    case SYS_WRITE:
        return sys_write(a1, (const char *)a2, a3);
    case SYS_CLEAR:
        return sys_clear();
    case SYS_READ:
        return sys_read(a1, (char *)a2, a3);
    case SYS_TIME:
        return sys_time();
    case SYS_EXIT:
        return sys_exit(a1);
    case SYS_WAITPID:
        return (uint64_t) waitForProcess(a1);
    case SYS_SET_TEXT_COLOR:
        return sys_set_text_color(a1);
    case SYS_SET_TEXT_COLOR_NAME:
        return sys_set_text_color_name((const char *)a1);
    case SYS_PRINT_AVAILABLE_COLORS:
        return sys_print_available_colors();
    case SYS_SET_COLORS:
        return sys_set_colors(a1, a2);
    case SYS_GET_SHELL_COLS:
        return sys_get_shell_cols();
    case SYS_GET_SHELL_ROWS:
        return sys_get_shell_rows();
    case SYS_KBD_AVAILABLE:
        return (uint64_t)kbd_available();
    case SYS_GET_COLOR_BY_NAME:
        if (videoIsLFB())
            return (uint64_t)vdGetColorByName((const char *)a1);
        return (uint64_t)0xFFFFFF;
    case SYS_GET_SCREEN_PX_WIDTH:
        if (videoIsLFB())
            return (uint64_t)vdGetScreenWidth();
        return (uint64_t)0;
    case SYS_GET_SCREEN_PX_HEIGHT:
        if (videoIsLFB())
            return (uint64_t)vdGetScreenHeight();
        return (uint64_t)0;
    case SYS_GFX_FILL_BLENDED:
        if (videoIsLFB())
        {
            /* a1 = x, a2 = y, a3 = pointer to args { uint32_t w,h,color,alpha } */
            struct blend_args {
                uint32_t w;
                uint32_t h;
                uint32_t color;
                uint32_t alpha;
            };
            const struct blend_args *args = (const struct blend_args *)a3;
            if (args)
            {
                drawRectFillBlend((uint32_t)args->color, (uint64_t)a1, (uint64_t)a2,
                                  (uint64_t)args->w, (uint64_t)args->h, (uint8_t)args->alpha);
                return (uint64_t)0;
            }
            return (uint64_t)-1;
        }
        return (uint64_t)-1;
    case SYS_REGS_PRINT:
        return sys_regs_print();
    case SYS_SET_TEXT_SIZE:
        return (uint64_t)sys_set_text_size((int)a1);
    case SYS_SET_EXC_RESUME:
        return sys_set_exc_resume(a1);
    case SYS_READ_TSC:
        return sys_read_tsc();
    case SYS_PIPE_OPEN:
        return sys_pipe_open((int)a1);
    case SYS_PIPE_CLOSE_WRITE:
        return sys_pipe_close_write((int)a1);
    case SYS_PIPE_CLOSE_READ:
        return sys_pipe_close_read((int)a1);
    case SYS_PIPE_SET_FD:
        return sys_pipe_set_fd((int)a1, (int)a2);
    case SYS_CREATE_PROCESS:
        return sys_create_process(a1);
    case SYS_GETPID:
        return sys_getpid();
    case SYS_KILL:
        return sys_kill(a1);
    case SYS_BLOCK:
        return sys_block(a1);
    case SYS_UNBLOCK:
        return sys_unblock(a1);
    case SYS_NICE:
        return sys_nice(a1, a2);
    case SYS_GET_PROCESSES:
        return sys_get_processes((ProcessInfo *)a1, a2);
    case SYS_YIELD:
        return sys_yield();
    case SYS_MEM_INFO:
        return sys_get_mem_info(a1);
    case SYS_SET_FOREGROUND:
        return sys_set_foreground(a1);
    case SYS_MALLOC:
        return sys_kmalloc(a1);
    case SYS_FREE:
        return sys_kfree(a1);
        case SYS_SEM_OPEN:
        return sys_sem_open((const char *)a1, a2);
    case SYS_SEM_WAIT:
        return sys_sem_wait((int)a1);
    case SYS_SEM_POST:
        return sys_sem_post((int)a1);
    case SYS_SEM_CLOSE:
        return sys_sem_close((int)a1);
    default:
        return (uint64_t)-1; // ENOSYS
    }
}

void syscall_init(void)
{
    // No runtime initialization required at the moment; IDT gate for int 0x80
    // is set up in assembly (idt.asm). This symbol exists so kernel link succeeds
    // and future syscall table initialization can be placed here.
}
