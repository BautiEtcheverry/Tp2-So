// Video driver implementation for VESA Linear Framebuffer (LFB).
//
// Overview
// - Reads VBE mode information populated by the bootloader at 0x5C00.
// - Provides basic drawing primitives (pixel, clear, lines, rectangles) over the LFB.
// - Color format is 0xRRGGBB on the API, converted to B,G,R bytes in memory.
// - Supports common 24 bpp and 32 bpp modes; other modes are ignored as safe no-ops.

#include <videoDriver.h>
#include <gfxConsole.h>
#include <font.h>
#include <lib.h>

/*
    For information about VESA Video models refere to: https://wiki.osdev.org/VESA_Video_Modes
*/

#ifndef NULL
#define NULL ((void *)0)
#endif

struct vbe_mode_info_structure
{
    uint16_t attributes;  // deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
    uint8_t window_a;     // deprecated
    uint8_t window_b;     // deprecated
    uint16_t granularity; // deprecated; used while calculating bank numbers
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr; // deprecated; used to switch banks from protected mode without returning to real mode
    uint16_t pitch;        // number of bytes per horizontal line
    uint16_t width;        // width in pixels
    uint16_t height;       // height in pixels
    uint8_t w_char;        // unused...
    uint8_t y_char;        // ...
    uint8_t planes;
    uint8_t bpp;   // bits per pixel in this mode
    uint8_t banks; // deprecated; total number of banks in this mode
    uint8_t memory_model;
    uint8_t bank_size; // deprecated; size of a bank, almost always 64 KB but may be 16 KB...
    uint8_t image_pages;
    uint8_t reserved0;

    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;

    uint32_t framebuffer; // physical address of the linear frame buffer; write here to draw to the screen
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size; // size of memory in the framebuffer but not being displayed on the screen
    uint8_t reserved1[206];
} __attribute__((packed));

typedef struct vbe_mode_info_structure *VBEInfoPtr;

VBEInfoPtr VBE_mode_info = (VBEInfoPtr)0x0000000000005C00;

typedef struct
{
    const char *colorname;
    uint32_t color;
} t_color;

// To avoid warnings from the compilers in case colors is not used.
#if defined(__GNUC__)
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

static const t_color colors[] MAYBE_UNUSED = {
    {"black", 0x000000},
    {"white", 0xFFFFFF},
    {"red", 0xFF0000},
    {"green", 0x00FF00},
    {"blue", 0x0000FF},
    {"yellow", 0xFFFF00},
    {"cyan", 0x00FFFF},
    {"magenta", 0xFF00FF},
    {"silver", 0xC0C0C0},
    {"gray", 0x808080},
    {"olive", 0x808000},
    {"dark_green", 0x008000},
    {"purple", 0x800080},
    {"teal", 0x008080},
    {"navy", 0x000080},
    {"orange", 0xFFA500},
    {"brown", 0x8B4513},
    {"pink", 0xFFC0CB},
    {"lavender", 0xE6E6FA},
    {NULL, 0}};

static inline int lfb_usable(void)
{
    if (VBE_mode_info == NULL)
        return 0;

    uint16_t w = VBE_mode_info->width;
    uint16_t h = VBE_mode_info->height;
    uint8_t  bpp = VBE_mode_info->bpp;
    uint16_t pitch = VBE_mode_info->pitch;
    uint64_t fb = (uint64_t)VBE_mode_info->framebuffer;

    if (w == 0 || h == 0) return 0;
    if (bpp != 24 && bpp != 32) return 0;
    if (fb == 0) return 0;

    uint32_t bpp_bytes = (uint32_t)(bpp / 8);
    if (pitch < (uint32_t)w * bpp_bytes) return 0;

    return 1;
}
static inline int mode_is_lfb(void){ return lfb_usable(); }
// 

static inline int in_bounds(uint64_t x, uint64_t y)
{
    return (x < VBE_mode_info->width) && (y < VBE_mode_info->height);
    // Draw a single pixel at (x,y) in color 0xRRGGBB.
    // - No-op if LFB unsupported, coordinates are out of bounds, or bpp not 24/32.
    // - Converts to B,G,R in memory (little-endian), respects pitch (bytes per scanline).
}

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y)
{
    if (!mode_is_lfb())
        return;
    if (!in_bounds(x, y))
        return;

    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return; // Unsupported format

    // Framebuffer is a physical address provided by VBE (32-bit field). Cast via uintptr_t to avoid size warnings.
    uint8_t *framebuffer = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint64_t offset = (x * (bpp / 8)) + (y * VBE_mode_info->pitch);

    // Colors are passed as 0xRRGGBB; VBE expects B, G, R in memory (little-endian)
    uint8_t b = (uint8_t)(hexColor & 0xFF);
    uint8_t g = (uint8_t)((hexColor >> 8) & 0xFF);
    uint8_t r = (uint8_t)((hexColor >> 16) & 0xFF);

    if (bpp == 24)
    {
        framebuffer[offset + 0] = b;
        framebuffer[offset + 1] = g;
        framebuffer[offset + 2] = r;
    }
    else // 32 bits per pixel
    {
        // Write 4 bytes: B, G, R, 0x00
        uint32_t *p = (uint32_t *)(framebuffer + offset);
        *p = ((uint32_t)hexColor) & 0x00FFFFFF; // ensures [B,G,R,00] in little-endian
    }
}

uint16_t getScreenWidth(void) {return VBE_mode_info->width;}
uint16_t getScreenHeight(void) {return VBE_mode_info->height;}

// Returns bits-per-pixel for the current VBE mode.
uint8_t getScreenBpp(void) {return VBE_mode_info->bpp;}

// Pitch: number of bytes per scanline. May exceed width * bytes_per_pixel due to alignment.
uint16_t getScreenPitch(void) {return VBE_mode_info->pitch;}

// Whether LFB is active/supported (1) or not (0).
int videoIsLFB(void) {return mode_is_lfb();}

// Fill entire visible framebuffer with a single color.
// Plain double loop
void clearScreen(uint32_t hexColor)
{
    if (!lfb_usable())
        return;

    uint16_t width = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    uint16_t pitch = VBE_mode_info->pitch;
    uint8_t bpp = VBE_mode_info->bpp;

    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;

    if (bpp == 32) {
        uint32_t color32 = ((uint32_t)hexColor) & 0x00FFFFFF; // B,G,R,00
        for (uint16_t y = 0; y < height; y++) {
            uint32_t *row = (uint32_t *)(fb + (uint64_t)y * pitch);
            for (uint16_t x = 0; x < width; x++) {
                row[x] = color32;
            }
        }
    } else { // 24 bpp
        uint8_t b = (uint8_t)(hexColor & 0xFF);
        uint8_t g = (uint8_t)((hexColor >> 8) & 0xFF);
        uint8_t r = (uint8_t)((hexColor >> 16) & 0xFF);
        for (uint16_t y = 0; y < height; y++) {
            uint8_t *row = fb + (uint64_t)y * pitch;
            for (uint16_t x = 0; x < width; x++) {
                uint64_t off = (uint64_t)x * 3;
                row[off + 0] = b;
                row[off + 1] = g;
                row[off + 2] = r;
            }
        }
    }
}

// Draw a 1-pixel thick horizontal line starting at (x,y) of width 'w'.  
void drawHLine(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w)
{
    if (!mode_is_lfb())
        return;

    uint16_t width = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    uint16_t pitch = VBE_mode_info->pitch;
    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return;

    if (y >= height || x >= width || w == 0)
        return;

    if (x + w > width)
        w = width - x; // clamp

    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint8_t *row = fb + (uint64_t)y * pitch;

    if (bpp == 32)
    {
        uint32_t color32 = ((uint32_t)hexColor) & 0x00FFFFFF;
        uint32_t *p = (uint32_t *)(row + x * 4);
        for (uint64_t i = 0; i < w; i++)
            p[i] = color32;
    }
    else // 24 bpp
    {
        uint8_t b = (uint8_t)(hexColor & 0xFF);
        uint8_t g = (uint8_t)((hexColor >> 8) & 0xFF);
        uint8_t r = (uint8_t)((hexColor >> 16) & 0xFF);
        uint64_t off = x * 3;
        for (uint64_t i = 0; i < w; i++)
        {
            row[off + 0] = b;
            row[off + 1] = g;
            row[off + 2] = r;
            off += 3;
        }
    }
}

// Draw a 1-pixel thick vertical line starting at (x,y) of height 'h'.
void drawVLine(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t h)
{
    if (!mode_is_lfb())
        return;

    uint16_t width = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    uint16_t pitch = VBE_mode_info->pitch;
    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return;

    if (x >= width || y >= height || h == 0)
        return;

    if (y + h > height)
        h = height - y; 

    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;

    if (bpp == 32)
    {
        uint32_t color32 = ((uint32_t)hexColor) & 0x00FFFFFF;
        for (uint64_t i = 0; i < h; i++)
        {
            uint32_t *p = (uint32_t *)(fb + (uint64_t)(y + i) * pitch + x * 4);
            *p = color32;
        }
    }
    else // 24 bpp
    {
        uint8_t b = (uint8_t)(hexColor & 0xFF);
        uint8_t g = (uint8_t)((hexColor >> 8) & 0xFF);
        uint8_t r = (uint8_t)((hexColor >> 16) & 0xFF);
        for (uint64_t i = 0; i < h; i++)
        {
            uint8_t *row = fb + (uint64_t)(y + i) * pitch;
            uint64_t off = (uint64_t)x * 3;
            row[off + 0] = b;
            row[off + 1] = g;
            row[off + 2] = r;
        }
    }
}

// Draw a filled rectangle with top-left (x,y) and size (w,h).
void drawRectFill(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h)
{
    if (!mode_is_lfb())
        return;

    uint16_t width = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    if (x >= width || y >= height || w == 0 || h == 0)
        return;

    if (x + w > width)
        w = width - x;
    if (y + h > height)
        h = height - y;

    for (uint64_t i = 0; i < h; i++)
    {
        drawHLine(hexColor, x, y + i, w);
    }
}

//read a pixel from framebuffer and return 0xRRGGBB. Supports 24/32bpp.
static uint32_t read_pixel(uint64_t x, uint64_t y)
{
    if (!mode_is_lfb() || !in_bounds(x, y))
        return 0x000000;
    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return 0x000000;
    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint16_t pitch = VBE_mode_info->pitch;
    uint64_t off = y * pitch + x * (bpp / 8);
    if (bpp == 32)
    {
        uint8_t b = fb[off + 0];
        uint8_t g = fb[off + 1];
        uint8_t r = fb[off + 2];
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
    else
    {
        uint8_t b = fb[off + 0];
        uint8_t g = fb[off + 1];
        uint8_t r = fb[off + 2];
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
}

//Write a 0xRRGGBB pixel to framebuffer. Supports 24/32bpp.
static void write_pixel(uint64_t x, uint64_t y, uint32_t color)
{
    if (!mode_is_lfb() || !in_bounds(x, y))
        return;
    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return;
    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint16_t pitch = VBE_mode_info->pitch;
    uint64_t off = y * pitch + x * (bpp / 8);
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    if (bpp == 32)
    {
        fb[off + 0] = b;
        fb[off + 1] = g;
        fb[off + 2] = r;
        fb[off + 3] = 0x00;
    }
    else
    {
        fb[off + 0] = b;
        fb[off + 1] = g;
        fb[off + 2] = r;
    }
}

// src and dst are 0xRRGGBB, alpha in [0..255] where 255=opaque. 
static uint32_t blend_rgb(uint32_t src, uint32_t dst, uint8_t alpha)
{
    if (alpha == 255)
        return src;
    if (alpha == 0)
        return dst;
    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8) & 0xFF;
    uint32_t sb = src & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;
    uint32_t r = (sr * alpha + dr * (255 - alpha)) / 255;
    uint32_t g = (sg * alpha + dg * (255 - alpha)) / 255;
    uint32_t b = (sb * alpha + db * (255 - alpha)) / 255;
    return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

void drawRectFillBlend(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint8_t alpha)
{
    if (!mode_is_lfb())
        return;
    if (w == 0 || h == 0)
        return;
    
    if (alpha >= 250) {
        drawRectFillFast(hexColor, x, y, w, h);
        return;
    }
    
    uint64_t x1 = x + w;
    uint64_t y1 = y + h;
    uint64_t maxw = VBE_mode_info->width;
    uint64_t maxh = VBE_mode_info->height;
    
    if (x >= maxw || y >= maxh)
        return;
    if (x1 > maxw)
        x1 = maxw;
    if (y1 > maxh)
        y1 = maxh;
    
    // Only blend if necessary (alpha < 250)
    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return;
        
    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint16_t pitch = VBE_mode_info->pitch;
    
    for (uint64_t yy = y; yy < y1; ++yy) {
        if (bpp == 32) {
            uint32_t *row = (uint32_t *)(fb + yy * pitch);
            for (uint64_t xx = x; xx < x1; ++xx) {
                uint32_t dst = row[xx] & 0x00FFFFFF;
                uint32_t out = blend_rgb(hexColor, dst, alpha);
                row[xx] = out & 0x00FFFFFF;
            }
        } else {
            for (uint64_t xx = x; xx < x1; ++xx) {
                uint32_t dst = read_pixel(xx, yy);
                uint32_t out = blend_rgb(hexColor, dst, alpha);
                write_pixel(xx, yy, out);
            }
        }
    }
}

void drawRectFillFast(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t w, uint64_t h)
{
    if (!mode_is_lfb() || w == 0 || h == 0)
        return;

    uint16_t width = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    if (x >= width || y >= height)
        return;

    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;

    uint8_t bpp = VBE_mode_info->bpp;
    if (bpp != 32) {
        // Fallback for 24bpp
        drawRectFill(hexColor, x, y, w, h);
        return;
    }

    uint16_t pitch = VBE_mode_info->pitch;
    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;
    uint32_t color32 = hexColor & 0x00FFFFFF;

    // Direct access, no nested loops
    uint32_t *base = (uint32_t *)(fb + y * pitch) + x;
    uint32_t pitch_in_pixels = pitch / 4;


    if (w == 1 && h == 1) {
        *base = color32;
        return;
    }
    if (w == 1 && h <= 4) {
        for (uint64_t i = 0; i < h; i++) {
            base[i * pitch_in_pixels] = color32;
        }
        return;
    }
    if (h == 1 && w <= 8) {
        for (uint64_t i = 0; i < w; i++) {
            base[i] = color32;
        }
        return;
    }

    for (uint64_t row = 0; row < h; row++) {
        uint32_t *row_ptr = base + row * pitch_in_pixels;
        for (uint64_t col = 0; col < w; col++) {
            row_ptr[col] = color32;
        }
    }
}

// Scroll the framebuffer up by 'pixels' rows.
// Used by a text console to scroll by one character row (font height).
// Bottom 'pixels' rows are cleared to 'fillColor'.
void scrollUp(uint16_t pixels, uint32_t fillColor)
{
    if (!mode_is_lfb() || pixels == 0)
        return;

    uint16_t width  = VBE_mode_info->width;
    uint16_t height = VBE_mode_info->height;
    uint16_t pitch  = VBE_mode_info->pitch;
    uint8_t  bpp    = VBE_mode_info->bpp;
    if (bpp != 24 && bpp != 32)
        return;

    if (pixels >= height) {
        clearScreen(fillColor);
        return;
    }

    uint8_t *fb = (uint8_t *)(uintptr_t)VBE_mode_info->framebuffer;

    //Move the entire framebuffer up by 'pixels' rows
    uint64_t bytes_per_row = (uint64_t)pitch;
    uint64_t rows_to_move  = (uint64_t)height - pixels;
    uint64_t bytes_to_move = rows_to_move * bytes_per_row;
    memmove(fb, fb + (uint64_t)pixels * bytes_per_row, bytes_to_move);

    
    if (bpp == 32)
    {
        uint32_t color32 = ((uint32_t)fillColor) & 0x00FFFFFF;
        for (uint16_t y = (uint16_t)(height - pixels); y < height; y++)
        {
            uint32_t *row = (uint32_t *)(fb + (uint64_t)y * pitch);
            for (uint16_t x = 0; x < width; x++)
                row[x] = color32;
        }
    }
    else // 24 bpp
    {
        uint8_t b = (uint8_t)(fillColor & 0xFF);
        uint8_t g = (uint8_t)((fillColor >> 8) & 0xFF);
        uint8_t r = (uint8_t)((fillColor >> 16) & 0xFF);
        for (uint16_t y = (uint16_t)(height - pixels); y < height; y++)
        {
            uint8_t *row = fb + (uint64_t)y * pitch;
            for (uint16_t x = 0; x < width; x++)
            {
                uint32_t off = (uint32_t)x * 3;
                row[off + 0] = b;
                row[off + 1] = g;
                row[off + 2] = r;
            }
        }
    }
}
// ---------------- Console-style API (wrappers over gfx console) ----------------
static uint32_t vd_default_color = 0xFFFFFF;

void vdPrintChar(char character)
{
    if (!mode_is_lfb())
        return;
    gfx_putc(character);
}

void vdPrint(const char *string)
{
    if (!mode_is_lfb())
        return;
    // write until null terminator
    const char *p = string;
    while (p && *p)
        gfx_putc(*p++);
}

uint64_t vdPrintCharStyled(char character, uint32_t color, uint32_t bgColor)
{
    if (!mode_is_lfb())
        return 0;
    uint32_t prev_fg = vd_default_color;
    gfx_set_colors(color, bgColor);
    gfx_putc(character);
    gfx_set_colors(prev_fg, bgColor);
    return (uint64_t)umr_int.width; 
}

void vdPrintStyled(char *s, uint32_t color, uint32_t bgColor)
{
    if (!mode_is_lfb())
        return;
    uint32_t prev_fg = vd_default_color;
    gfx_set_colors(color, bgColor);
    for (char *p = s; p && *p; ++p)
        gfx_putc(*p);
    gfx_set_colors(prev_fg, bgColor);
}

uint64_t vdNPrintStyled(const char *string, uint32_t color, uint32_t bgColor, uint64_t N)
{
    if (!mode_is_lfb())
        return 0;
    uint32_t prev_fg = vd_default_color;
    gfx_set_colors(color, bgColor);
    uint64_t i = 0;
    for (; i < N && string && string[i]; ++i)
        gfx_putc(string[i]);
    gfx_set_colors(prev_fg, bgColor);
    return i;
}

void vdNewline(void)
{
    if (!mode_is_lfb())
        return;
    gfx_putc('\n');
}

void vdClear(void)
{
    if (!mode_is_lfb())
        return;
    gfx_clear();
}

void vdDelete(void)
{
    if (!mode_is_lfb())
        return;
    gfx_putc('\b');
}

void drawRectangle(uint32_t color, uint16_t up_l_x, uint16_t up_l_y, uint16_t lo_r_x, uint16_t lo_r_y)
{
    if (!mode_is_lfb())
        return;
    if (lo_r_x <= up_l_x || lo_r_y <= up_l_y)
        return;
    drawRectFill(color, up_l_x, up_l_y, (uint64_t)(lo_r_x - up_l_x), (uint64_t)(lo_r_y - up_l_y));
}

int vdSetFontSize(uint8_t size)
{
    // Map sizes: 0=default, 1=large, 2=xlarge
    if (size > 2)
        return -1;
    // Forward to gfx console to adjust font size.
    gfx_set_font_mode((int)size);
    return 0;
}

void vdSetColor(uint32_t fgColor)  {vd_default_color = fgColor;}

uint32_t vdGetColorByName(const char *colorName)
{
    // Simple lookup against the static table in this file
    if (!colorName)
        return 0xFFFFFFFF; /* sentinel: not found */
    for (int i = 0; colors[i].colorname != NULL; i++)
    {
        const char *a = colors[i].colorname;
        const char *b = colorName;
        int equal = 1;
        while (*a || *b)
        {
            char ca = *a;
            char cb = *b;
            // case-insensitive compare
            if (ca >= 'A' && ca <= 'Z')
                ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z')
                cb = (char)(cb - 'A' + 'a');
            if (ca != cb)
            {
                equal = 0;
                break;
            }
            a++;
            b++;
        }
        if (equal)
            return colors[i].color;
    }
    return 0xFFFFFFFF; /* sentinel: not found */
}

void vdPrintAvailableColors(void)
{
    if (!mode_is_lfb())
        return;
    gfx_write("Available colors:\n", 19);
    for (int i = 0; colors[i].colorname != NULL; i++)
    {
        gfx_write(" - ", 3);
        const char *p = colors[i].colorname;
        while (p && *p)
            gfx_putc(*p++);
        gfx_write("\n", 1);
    }
}

void vdFillScreen(uint32_t color)
{
    clearScreen(color);
}

uint16_t vdGetScreenWidth(void)
{
    return getScreenWidth();
}

uint16_t vdGetScreenHeight(void)
{
    return getScreenHeight();
}

int vdIsValidMode(void)
{
    return mode_is_lfb();
}
