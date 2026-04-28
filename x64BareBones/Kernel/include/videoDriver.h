#include <stdint.h>

// Video driver API for VESA linear framebuffer (LFB) drawing.
// Color format: 0xRRGGBB (the driver writes B,G,R in little-endian memory).
// If VESA LFB is not active/supported, all drawing functions are safe no-ops.

// Draw one pixel at (x,y) with the given color.
// - Bounds-checked.
// - Supports 24 bpp and 32 bpp modes (common VBE modes).
void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);

// Quick queries for screen geometry and mode details.
uint16_t getScreenWidth(void);  // width in pixels
uint16_t getScreenHeight(void); // height in pixels
uint8_t getScreenBpp(void);     // bits per pixel (e.g., 24 or 32)
uint16_t getScreenPitch(void);  // bytes per scanline
int videoIsLFB(void);           // 1 if LFB mode is active/supported; 0 otherwise

// Fill entire screen with a solid color.
// Hot path used to clear the console background.
void clearScreen(uint32_t hexColor);

// Draw axis-aligned 1px lines.
void drawHLine(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w);
void drawVLine(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t h);

// Draw a filled rectangle with top-left corner at (x,y) and size (w,h).
void drawRectFill(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h);
// Fill rectangle with alpha blending. Alpha: 0 (transparent) .. 255 (opaque)(will use it for the tron game)
void drawRectFillBlend(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint8_t alpha);

// Scroll the framebuffer contents up by 'pixels' rows.
// Newly exposed rows at the bottom are filled with 'fillColor'.
// Intended for text console line-scrolling (use the font height as 'pixels').
void scrollUp(uint16_t pixels, uint32_t fillColor);

// ---------------- Console-style API (minimal wrappers over gfx console) ----------------
void vdPrintChar(char character);
void vdPrint(const char *string);
uint64_t vdPrintCharStyled(char character, uint32_t color, uint32_t bgColor);
void vdPrintStyled(char *s, uint32_t color, uint32_t bgColor);
uint64_t vdNPrintStyled(const char *string, uint32_t color, uint32_t bgColor, uint64_t N);
void vdNewline(void);
void vdClear(void);
void vdDelete(void);
void drawRectangle(uint32_t color, uint16_t up_l_x, uint16_t up_l_y, uint16_t lo_r_x, uint16_t lo_r_y);
// vdSetFontSize modes: 0 = default (base size), 1 = large, 2 = xlarge.
int vdSetFontSize(uint8_t size);
void vdSetColor(uint32_t fgColor);
uint32_t vdGetColorByName(const char *colorName);
void vdPrintAvailableColors(void);
void vdFillScreen(uint32_t color);
void drawRectFillFast(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h);
uint16_t vdGetScreenWidth(void);
uint16_t vdGetScreenHeight(void);
int vdIsValidMode(void);

// Console character cell geometry
uint32_t vdGetShellCols(void);
uint32_t vdGetShellRows(void);