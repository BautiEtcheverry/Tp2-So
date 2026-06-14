#include "headers/prompt.h"
#include "../../include/libc.h"

static uint32_t current_fg = 0xFFFFFF;

void prompt_set_fg(uint32_t rgb) { current_fg = rgb; }
uint32_t prompt_get_fg(void)     { return current_fg; }

void prompt(void) {
    uint32_t bg = 0x272827;
    set_colors(0x875FD7, bg);
    write(1, "$TPE-So", 10);
    set_colors(0xFFA657, bg);
    write(1, "> ", 2);
    set_colors(current_fg, bg);   // usa el estado interno
}