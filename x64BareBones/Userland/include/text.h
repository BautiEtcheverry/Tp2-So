#include <stdint.h>

/*
    Basic text rendering module using a bitmap font.
    
    - text_draw_string: Renders a string at specified coordinates with scaling and color.
    - text_measure_width: Calculates the width of a string when rendered with a given scale.
    - text_measure_height: Calculates the height of the text for a given scale.
*/

void text_draw_string(int x, int y, const char *msg, int scale, uint32_t color);
int text_measure_width(const char *msg, int scale);
int text_measure_height(int scale);

