#include "../include/text.h"
#include "../include/libc.h"

#define GLYPH_W 3
#define GLYPH_H 5
#define GLYPH_SPACING 1

typedef struct {
    char ch;
    uint8_t rows[GLYPH_H];
} glyph_t;

/* 3x5 bitmap font; bit 2 is leftmost column. */
static const glyph_t glyph_table[] = {
    // Números
    { '0', {0b111, 0b101, 0b101, 0b101, 0b111} },
    { '1', {0b010, 0b110, 0b010, 0b010, 0b111} },
    { '2', {0b111, 0b001, 0b111, 0b100, 0b111} },
    { '3', {0b111, 0b001, 0b111, 0b001, 0b111} },
    { '4', {0b101, 0b101, 0b111, 0b001, 0b001} },
    { '5', {0b111, 0b100, 0b111, 0b001, 0b111} },
    { '6', {0b111, 0b100, 0b111, 0b101, 0b111} },
    { '7', {0b111, 0b001, 0b010, 0b010, 0b010} },
    { '8', {0b111, 0b101, 0b111, 0b101, 0b111} },
    { '9', {0b111, 0b101, 0b111, 0b001, 0b111} },
    
    // Alfabeto completo
    { 'A', {0b111, 0b101, 0b111, 0b101, 0b101} },
    { 'B', {0b110, 0b101, 0b110, 0b101, 0b110} },
    { 'C', {0b111, 0b100, 0b100, 0b100, 0b111} },
    { 'D', {0b110, 0b101, 0b101, 0b101, 0b110} },
    { 'E', {0b111, 0b100, 0b111, 0b100, 0b111} },
    { 'F', {0b111, 0b100, 0b110, 0b100, 0b100} },
    { 'G', {0b111, 0b100, 0b101, 0b101, 0b111} },
    { 'H', {0b101, 0b101, 0b111, 0b101, 0b101} },
    { 'I', {0b111, 0b010, 0b010, 0b010, 0b111} },
    { 'J', {0b111, 0b001, 0b001, 0b101, 0b111} },
    { 'K', {0b101, 0b110, 0b100, 0b110, 0b101} },
    { 'L', {0b100, 0b100, 0b100, 0b100, 0b111} },
    { 'M', {0b101, 0b111, 0b111, 0b101, 0b101} },
    { 'N', {0b111, 0b101, 0b101, 0b101, 0b101} },
    { 'O', {0b111, 0b101, 0b101, 0b101, 0b111} },
    { 'P', {0b110, 0b101, 0b110, 0b100, 0b100} },
    { 'Q', {0b111, 0b101, 0b101, 0b111, 0b001} },
    { 'R', {0b110, 0b101, 0b110, 0b101, 0b101} },
    { 'S', {0b111, 0b100, 0b111, 0b001, 0b111} },
    { 'T', {0b111, 0b010, 0b010, 0b010, 0b010} },
    { 'U', {0b101, 0b101, 0b101, 0b101, 0b111} },
    { 'V', {0b101, 0b101, 0b101, 0b101, 0b010} },
    { 'W', {0b101, 0b101, 0b111, 0b111, 0b101} },
    { 'X', {0b101, 0b101, 0b010, 0b101, 0b101} },
    { 'Y', {0b101, 0b101, 0b111, 0b010, 0b010} },
    { 'Z', {0b111, 0b001, 0b010, 0b100, 0b111} },
    
    // Símbolos especiales
    { ':', {0b000, 0b010, 0b000, 0b010, 0b000} },
    { '-', {0b000, 0b000, 0b111, 0b000, 0b000} },
    { '!', {0b010, 0b010, 0b010, 0b000, 0b010} },
    { '?', {0b111, 0b001, 0b010, 0b000, 0b010} },
    { '.', {0b000, 0b000, 0b000, 0b000, 0b010} },
    { ',', {0b000, 0b000, 0b000, 0b010, 0b100} },
    { '(', {0b001, 0b010, 0b010, 0b010, 0b001} },
    { ')', {0b100, 0b010, 0b010, 0b010, 0b100} },
    { '/', {0b001, 0b001, 0b010, 0b100, 0b100} },
    { '\\',{0b100, 0b100, 0b010, 0b001, 0b001} },
    { '=', {0b000, 0b111, 0b000, 0b111, 0b000} },
    { '+', {0b000, 0b010, 0b111, 0b010, 0b000} },
    { '*', {0b000, 0b101, 0b010, 0b101, 0b000} },
    { '\'',{0b010, 0b010, 0b000, 0b000, 0b000} },
    { '"', {0b101, 0b101, 0b000, 0b000, 0b000} },
    { '@', {0b111, 0b101, 0b111, 0b100, 0b111} },
    { '#', {0b101, 0b111, 0b101, 0b111, 0b101} },
    { '$', {0b111, 0b110, 0b111, 0b011, 0b111} },
    { '%', {0b101, 0b001, 0b010, 0b100, 0b101} },
    { '&', {0b110, 0b100, 0b111, 0b101, 0b111} },
    { '[', {0b111, 0b100, 0b100, 0b100, 0b111} },
    { ']', {0b111, 0b001, 0b001, 0b001, 0b111} },
    { '<', {0b001, 0b010, 0b100, 0b010, 0b001} },
    { '>', {0b100, 0b010, 0b001, 0b010, 0b100} },
    { '|', {0b010, 0b010, 0b010, 0b010, 0b010} },
    { '^', {0b010, 0b101, 0b000, 0b000, 0b000} },
    { '_', {0b000, 0b000, 0b000, 0b000, 0b111} },
    { '~', {0b000, 0b110, 0b011, 0b000, 0b000} },
    { '`', {0b100, 0b010, 0b000, 0b000, 0b000} },
    
    
    { ' ', {0b000, 0b000, 0b000, 0b000, 0b000} },
};

static const glyph_t *lookup_glyph(char c) {
    int n = (int)(sizeof(glyph_table) / sizeof(glyph_table[0]));
    for (int i = 0; i < n; ++i) {
        if (glyph_table[i].ch == c)
            return &glyph_table[i];
    }
    return &glyph_table[n - 1]; 
}

int text_measure_width(const char *msg, int scale) {
    if (scale < 1)
        scale = 1;
    int width = 0;
    int count = 0;
    for (const char *p = msg; *p; ++p) {
        count++;
        width += GLYPH_W * scale;
        if (*(p + 1))
            width += GLYPH_SPACING * scale;
    }
    return (count == 0) ? 0 : width;
}

int text_measure_height(int scale) {
    if (scale < 1)
        scale = 1;
    return GLYPH_H * scale;
}

void text_draw_string(int x, int y, const char *msg, int scale, uint32_t color) {
    if (scale < 1)
        scale = 1;
    int cursor_x = x;
    for (const char *p = msg; *p; ++p) {
        const glyph_t *g = lookup_glyph(*p);
        for (int row = 0; row < GLYPH_H; ++row) {
            uint8_t bits = g->rows[row];
            for (int col = 0; col < GLYPH_W; ++col) {
                uint8_t mask = (uint8_t)(1u << (GLYPH_W - 1 - col));
                if (bits & mask) {
                    int dx = cursor_x + col * scale;
                    int dy = y + row * scale;
                    fill_rect_blend(dx, dy, scale, scale, color, 255);
                }
            }
        }
        cursor_x += (GLYPH_W + GLYPH_SPACING) * scale;
    }
}
