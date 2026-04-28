#include <gfxConsole.h>
#include <videoDriver.h>
#define FONT_IMPL
#include <font.h>

static uint32_t fg = 0xFFFFFF;
static uint32_t bg = 0x000000;
static uint32_t cols = 0;
static uint32_t rows = 0;
static uint32_t cur_c = 0;
static uint32_t cur_r = 0;
// font mode: 0=default (base size), 1=large (2x), 2=xlarge (3x)
static int font_mode = 0;
// One-line top margin to avoid text cutting at the very top
static const uint32_t margin_c = 1; // columns on the left
static const uint32_t margin_r = 1; // rows at the top
// Extra vertical spacing (in pixels) between text rows
static const uint32_t line_spacing = 2;
static const uint32_t tab_width = 4;

// Simple in-kernel text buffer to hold printed characters so we can reflow
// them when the font size changes.
#define GFX_TEXT_BUF_CAP 16384
static char gfx_text_buf[GFX_TEXT_BUF_CAP];
static size_t gfx_text_buf_len = 0;
static size_t gfx_text_buf_start = 0;
static int gfx_reflowing = 0;

static inline uint32_t eff_width(void);
static inline uint32_t cell_h(void);
void gfx_putc(char ch);
static void buf_append_char(char ch)
{
    if (gfx_reflowing)
        return;
    if (gfx_text_buf_len < GFX_TEXT_BUF_CAP)
    {
        size_t idx = (gfx_text_buf_start + gfx_text_buf_len) % GFX_TEXT_BUF_CAP;
        gfx_text_buf[idx] = ch;
        gfx_text_buf_len++;
    }
    else
    {
        // buffer full: overwrite oldest and advance start (circular buffer)
        gfx_text_buf[gfx_text_buf_start] = ch;
        gfx_text_buf_start = (gfx_text_buf_start + 1) % GFX_TEXT_BUF_CAP;
    }
}

static void gfx_reflow(void)
{
    // Clear screen and reset cursor; we'll re-render from buffer
    clearScreen(bg);
    if (margin_r > 0)
        drawRectFill(bg, 0, 0, getScreenWidth(), margin_r * cell_h());
    if (margin_c > 0)
        drawRectFill(bg, 0, 0, margin_c * eff_width(), getScreenHeight());
    cur_c = 0;
    cur_r = 0;

    gfx_reflowing = 1;
    for (size_t i = 0; i < gfx_text_buf_len; ++i)
    {
        char ch = gfx_text_buf[(gfx_text_buf_start + i) % GFX_TEXT_BUF_CAP];
        gfx_putc(ch);
    }
    gfx_reflowing = 0;
}

// cell height depends on the font mode
static inline uint32_t eff_width(void)
{
    if (font_mode == 1)
        return (uint32_t)(umr_int.width * 2);
    if (font_mode == 2)
        return (uint32_t)(umr_int.width * 3);
    return (uint32_t)umr_int.width;
}

static inline uint32_t eff_height(void)
{
    if (font_mode == 1)
        return (uint32_t)(umr_int.height * 2);
    if (font_mode == 2)
        return (uint32_t)(umr_int.height * 3);
    return (uint32_t)umr_int.height;
}

static inline uint32_t cell_h(void) { return (uint32_t)(eff_height() + line_spacing); }

static inline void blit_glyph(int ch, uint32_t px, uint32_t py)
{
    int index = (unsigned int)(unsigned char)ch;
    if (index >= umr_int.nChars)
        index = (unsigned int)(unsigned char)'?'; // We use the "?" an unsupported character.

    const uint8_t *glyph = ubuntu_mono_regular[index];

    if (font_mode == 1)
    {
        // upscale 2x: each source pixel becomes a 2x2 block
        for (int ry = 0; ry < (int)umr_int.height; ry++)
        {
            uint8_t row = glyph[ry];
            for (int rx = 0; rx < (int)umr_int.width; rx++)
            {
                uint32_t mask = 1u << ((int)umr_int.width - 1 - rx);
                uint32_t color = (row & mask) ? fg : bg;
                uint32_t outx = px + (uint32_t)((int)umr_int.width - 1 - rx) * 2;
                uint32_t outy = py + (uint32_t)ry * 2;

                putPixel(color, outx + 0, outy + 0);
                putPixel(color, outx + 1, outy + 0);
                putPixel(color, outx + 0, outy + 1);
                putPixel(color, outx + 1, outy + 1);
            }
        }
        return;
    }

    if (font_mode == 2)
    {
        // upscale 3x: each source pixel becomes a 3x3 block
        for (int ry = 0; ry < (int)umr_int.height; ry++)
        {
            uint8_t row = glyph[ry];
            for (int rx = 0; rx < (int)umr_int.width; rx++)
            {
                uint32_t mask = 1u << ((int)umr_int.width - 1 - rx);
                uint32_t color = (row & mask) ? fg : bg;
                uint32_t outx = px + (uint32_t)((int)umr_int.width - 1 - rx) * 3;
                uint32_t outy = py + (uint32_t)ry * 3;
                for (int oy = 0; oy < 3; oy++)
                    for (int ox = 0; ox < 3; ox++)
                        putPixel(color, outx + ox, outy + oy);
            }
        }
        return;
    }

    for (int ry = 0; ry < (int)umr_int.height; ry++)
    {
        uint8_t row = glyph[ry];
        for (int rx = 0; rx < (int)umr_int.width; rx++)
        {
            uint32_t mask = 1u << ((int)umr_int.width - 1 - rx);
            uint32_t color = (row & mask) ? fg : bg;
            putPixel(color, px + (uint32_t)((int)umr_int.width - 1 - rx), py + (uint32_t)ry);
        }
    }
}

static inline void clear_cell(uint32_t c, uint32_t r)
{
    drawRectFill(bg,
                 (margin_c + c) * eff_width(),
                 (margin_r * cell_h()) + (r * cell_h()),
                 eff_width(),
                 cell_h());
}

void gfx_init(uint32_t fg_rgb, uint32_t bg_rgb)
{
    fg = fg_rgb;
    bg = bg_rgb;
    cols = getScreenWidth() / eff_width();
    // Horizontal margin in columns
    cols = (cols > margin_c) ? (cols - margin_c) : 0;
    // Compute rows with vertical spacing and top margin (in row units)
    uint32_t ch = cell_h();
    uint32_t usable_h = getScreenHeight();
    if (usable_h > margin_r * ch)
        usable_h -= margin_r * ch;
    else
        usable_h = 0;
    rows = (ch > 0) ? (usable_h / ch) : 0;
    cur_c = 0;
    cur_r = 0;
    if (cols < 1) cols = 1;
     if (rows < 1) rows = 1;
    clearScreen(bg);
    // Ensure margins area remains blank
    if (margin_r > 0)
        drawRectFill(bg, 0, 0, getScreenWidth(), margin_r * cell_h());
    if (margin_c > 0)
        drawRectFill(bg, 0, 0, margin_c * eff_width(), getScreenHeight());
}

void gfx_clear(void)
{
    /* Clear screen and reset cursor */
    clearScreen(bg);
    /* Clear the internal text buffer too*/
    gfx_text_buf_len = 0;
    gfx_text_buf_start = 0;
    cur_c = 0;
    cur_r = 0;
    if (margin_r > 0)
        drawRectFill(bg, 0, 0, getScreenWidth(), margin_r * cell_h());
    if (margin_c > 0)
        drawRectFill(bg, 0, 0, margin_c * eff_width(), getScreenHeight());
}

// Advance to a new line. If at bottom, scroll the framebuffer up by one
// character row (cell_h()) using video driver's scrollUp(), then clear the
// new bottom row.
static inline void new_line(void)
{
    cur_c = 0;
    cur_r++;
    if (cur_r >= rows)
    {
        // scroll up by one character row
        scrollUp((uint16_t)cell_h(), bg);
        // after scroll, cursor stays at last row
        cur_r = rows ? rows - 1 : 0;

        // keep sup margin clean
        if (margin_r > 0) {
            drawRectFill(bg, 0, 0, getScreenWidth(), margin_r * cell_h());
        }

        uint32_t y0 = getScreenHeight() - cell_h();
        drawRectFill(bg, 0, y0, getScreenWidth(), cell_h());

    }
}

void gfx_putc(char ch)
{
    /*record to buffer for reflow*/
    buf_append_char(ch);

    if (ch == '\r')
    {
        cur_c = 0;
        return;
    }
    if (ch == '\n')
    {
        new_line();
        return;
    }
    if (ch == '\b')
    {
        if (cur_c > 0)
        {
            cur_c--;
            clear_cell(cur_c, cur_r);
        }
        return;
    }
    if (ch == '\t')
    {
        uint32_t next = ((cur_c / tab_width) + 1) * tab_width;
        if (next >= cols)
            next = cols;

        
        if (next > cur_c)
        {
            uint32_t x0 = (margin_c + cur_c) * eff_width();
            uint32_t w  = (next - cur_c) * eff_width();
            uint32_t y0 = margin_r * cell_h() + cur_r * cell_h();
            drawRectFill(bg, x0, y0, w, cell_h());
            cur_c = next;
        }
        if (cur_c >= cols)
            return;
        return;
    }


    blit_glyph((int)(unsigned char)ch,
               (margin_c + cur_c) * eff_width(),
               margin_r * cell_h() + cur_r * cell_h());
    cur_c++;
    if (cur_c >= cols)
        new_line();
}

size_t gfx_write(const char *buf, size_t len)
{
    size_t n = 0;
    for (; n < len; n++)
        gfx_putc(buf[n]);
    return n;
}

void gfx_set_colors(uint32_t fg_rgb, uint32_t bg_rgb)
{
    fg = fg_rgb;
    bg = bg_rgb;
}

void gfx_set_fg(uint32_t fg_rgb)
{
    fg = fg_rgb;
}

// Expose console geometry (in character cells) to other kernel code
uint32_t vdGetShellCols(void)
{
    return cols;
}

uint32_t vdGetShellRows(void)
{
    return rows;
}

// Change font rendering mode at runtime (0=default,1=large,2=xlarge)
void gfx_set_font_mode(int mode)
{
    if (mode < 0 || mode > 2)
        return;
    font_mode = mode;
    // Previous content is now re-drawn.
    cols = getScreenWidth() / eff_width();
    cols = (cols > margin_c) ? (cols - margin_c) : 0;
    uint32_t ch = cell_h();
    uint32_t usable_h = getScreenHeight();
    if (usable_h > margin_r * ch)
        usable_h -= margin_r * ch;
    else
        usable_h = 0;
    rows = (ch > 0) ? (usable_h / ch) : 0;
    if (cols < 1) cols = 1; 
    if (rows < 1) rows = 1;
    cur_c = 0;
    cur_r = 0;

    if (gfx_text_buf_len > 0)
        gfx_reflow();
}
