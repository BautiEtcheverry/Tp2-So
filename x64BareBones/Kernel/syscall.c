#include <stdint.h>
#include <naiveConsole.h>
#include <syscall.h>
#include <keyboard.h>
#include <videoDriver.h>
#include <gfxConsole.h>
#include <audioDriver.h>
#include <libasm.h>

extern uint64_t exc_resume_rip; 
extern uint64_t read_tsc_asm(void);

// Forward declaration: implemented in Kernel/cdrivers/regs.c
void regs_print(void);

static uint64_t sys_set_exc_resume(uint64_t rip){
    exc_resume_rip = rip;
    return 0;
}

static uint64_t sys_write(uint64_t fd, const char *buf, uint64_t len)
{
    // Only support stdout (fd=1) and valid buffer pointer
    if (fd != 1 || buf == 0)
        return 0;

    // Determine number of bytes to print.
    size_t n = (size_t)len;
    if (n == 0) {
        // Compute length of C-string up to a safe bound.
        // This prevents infinite loops if userland passes non-null-terminated strings
        const char *p = buf;
        size_t max = 4096; // safety bound for user strings
        while (max-- && *p)
            p++;
        n = (size_t)(p - buf);
    }

    if (n == 0)
        return 0;


    static char kernel_buffer[8192];  // Buffer in kernel memory space
    
    // Limit copy size to prevent buffer overflow
    if (n > sizeof(kernel_buffer) - 1)
        n = sizeof(kernel_buffer) - 1;
    
    // Copy from userland buffer to kernel buffer
    memcpy(kernel_buffer, buf, n);
    kernel_buffer[n] = '\0';  // Null terminator for safety

    if (videoIsLFB())
    {
        // Graphics console - use kernel buffer (safe to access)
        return (uint64_t)gfx_write(kernel_buffer, n); 
    }
    else
    {
        // VGA text fallback - use kernel buffer (safe to access)
        for (size_t i = 0; i < n; i++)
        {
            char c = kernel_buffer[i]; 
            if (c == '\n') ncNewline();
            else           ncPrintChar(c);
        }
        return (uint64_t)n;
    }
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
    if (fd != 0 || buf == 0 || len == 0)
        return 0;
    // Block until at least one byte is available
    while (kbd_available() == 0){ }
    size_t n = kbd_read(buf, len);
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

static uint64_t sys_audio_play_tone(uint64_t frequency)
{
    return (uint64_t)audioPlayTone((uint32_t)frequency);
}

static uint64_t sys_audio_stop_tone(void)
{
    return (uint64_t)audioStopTone();
}

static uint64_t sys_audio_mute(uint64_t enable)
{
    return (uint64_t)audioMute((uint8_t)enable);
}

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
    case SYS_AUDIO_PLAY_TONE:
        return sys_audio_play_tone(a1);
    case SYS_AUDIO_STOP_TONE:
        return sys_audio_stop_tone();
    case SYS_AUDIO_MUTE:
        return sys_audio_mute(a1);
    case SYS_READ_TSC:
        return sys_read_tsc();
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
