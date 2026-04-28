#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Syscall numbers
enum
{
    SYS_WRITE = 1,                  // write(fd, buf, len)
    SYS_CLEAR = 2,                  // clear_screen()
    SYS_READ = 3,                   // read(fd, buf, len)
    SYS_TIME = 5,                   // returns packed Y(8)-M(8)-D(8)-H(8)-m(8)-S(8)
    SYS_SET_TEXT_COLOR = 6,         // set text color by RGB (foreground)
    SYS_SET_TEXT_COLOR_NAME = 7,    // set text color by name (uses driver table)
    SYS_PRINT_AVAILABLE_COLORS = 8, // kernel prints available colors
    SYS_REGS_PRINT = 9,             // print last captured registers
    SYS_SET_COLORS = 10,            // set foreground and background colors (fg,bg)
    SYS_GET_SHELL_COLS = 11,        // returns console columns
    SYS_GET_SHELL_ROWS = 12,        // returns console rows
    SYS_KBD_AVAILABLE = 13,         // number of bytes available in keyboard buffer
    SYS_GET_COLOR_BY_NAME = 14,     // returns rgb for a color name
    SYS_GFX_FILL_BLENDED = 15,      // fill rectangle with alpha blending (x,y,ptr->(w,h,color,alpha))
    SYS_GET_SCREEN_PX_WIDTH = 16,
    SYS_GET_SCREEN_PX_HEIGHT = 17,
    SYS_SET_TEXT_SIZE=18,            // Change the shell textSize (textSize command<default,large,xlarge>)         // Change the shell textSize (textSize command<default,large,xlarge>)
    SYS_AUDIO_PLAY_TONE = 19,       // audioPlayTone(frequency in hz 20-20000 range)
    SYS_AUDIO_STOP_TONE = 20,       // audioStopTone()
    SYS_AUDIO_SET_VOLUME = 21,      // audioSetVolume(volume 0-100)
    SYS_AUDIO_MUTE = 22,             // audioMute(enable)
    SYS_SET_EXC_RESUME = 23 ,        // set exception resume point
    SYS_READ_TSC = 24                 // read Time Stamp Counter
};

// Kernel-side API
void syscall_init(void);
uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3);

#endif
