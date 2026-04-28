#ifndef USERLAND_LIBC_H
#define USERLAND_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
/*
    Making static inline functions(small ones) essentialy gives each file that includes this header a copy of the function, which can be inlined by the compiler.
    More info about inline functions in C: https://wiki.osdev.org/index.php?search=Inline+Functions+in+C&title=Special%3ASearch&profile=default&fulltext=1
*/
/*
    Syscall numbers must match the ones in the kernel in the file: /x64BareBones/Kernel/include/syscall.h
*/
enum
{
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_READ = 3,
    SYS_EXIT = 4,
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
    SYS_AUDIO_PLAY_TONE = 19,
    SYS_AUDIO_STOP_TONE = 20,
    SYS_AUDIO_MUTE = 22,
    SYS_SET_EXC_RESUME=23,
    SYS_READ_TSC = 24
};

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
    while (read(0, &c, 1) == 0)
    {
    }
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


/* Audio driver functions */
/*----------------------------------------------------------------------*/
static inline int audioPlayTone(uint32_t frequency) {
    return (int)sys_3p(SYS_AUDIO_PLAY_TONE, (uint64_t)frequency, 0, 0);
}

static inline int audioStopTone(void) {
    return (int)sys_3p(SYS_AUDIO_STOP_TONE, 0, 0, 0);
}

static inline int audioMute(uint8_t enable) {
    return (int)sys_3p(SYS_AUDIO_MUTE, (uint64_t)enable, 0, 0);
}
/*-----------------------------------------------------------------------*/
#endif
