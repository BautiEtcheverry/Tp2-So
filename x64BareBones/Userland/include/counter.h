#include <stdint.h>

/*
    FPS counter module provides functionality to measure frames per second (FPS) in applications such as games or simulations.
    
    - fps_counter_init: Initializes the FPS counter structure.
    - fps_counter_tick: Updates the FPS counter and returns the current FPS value.
    - fps_counter_value: Retrieves the current FPS value without updating the counter.
*/

// FPS counter structure
typedef struct {
    
    long long prev_ns;
    long long acc_ns;
    unsigned frames;
    unsigned fps;

} fps_counter_t;

void fps_counter_init(fps_counter_t *c);
unsigned fps_counter_tick(fps_counter_t *c);
unsigned fps_counter_value(const fps_counter_t *c);


