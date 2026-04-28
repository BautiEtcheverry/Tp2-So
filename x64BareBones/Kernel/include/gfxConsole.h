#ifndef GFX_CONSOLE_H
#define GFX_CONSOLE_H

#include <stdint.h>
#include <lib.h>
#include <stddef.h>

// Simple graphics console over VESA LFB using a fixed-width bitmap font.
// Text color is 0xRRGGBB.

// Initialize console with foreground/background colors and reset cursor.
void gfx_init(uint32_t fg_rgb, uint32_t bg_rgb);

// Clear the entire console area and reset cursor to (0,0).
void gfx_clear(void);

// Write a single character processing control chars (\n, \r, \b).
void gfx_putc(char ch);

// Write a buffer of bytes to the console. Returns bytes consumed.
size_t gfx_write(const char *buf, size_t len);

// Optionally change current colors.
void gfx_set_colors(uint32_t fg_rgb, uint32_t bg_rgb);

// Change only the foreground (text) color.
void gfx_set_fg(uint32_t fg_rgb);

// Change font:
// mode: 0 = default, 1 = large, 2 = xlarge
// starts new shell.
void gfx_set_font_mode(int mode);

#endif // GFX_CONSOLE_H
