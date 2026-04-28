#include "../include/counter.h"
#include "../include/actualTime.h"

#define FPS_INTERVAL_NS (NS_PER_SEC / 4)

void fps_counter_init(fps_counter_t *c) {
    if (!c){return;}

    c->prev_ns = actualTime();
    c->acc_ns = 0;
    c->frames = 0;
    c->fps = 0;
}

unsigned fps_counter_value(const fps_counter_t *c) {
    return c ? c->fps : 0;
}

unsigned fps_counter_tick(fps_counter_t *c) {
    if (!c)
        return 0;
    long long now = actualTime();
    long long dt = now - c->prev_ns;
    c->prev_ns = now;
    if (dt < 0)
        dt = -dt;
    c->acc_ns += dt;
    c->frames++;

    if (c->acc_ns >= FPS_INTERVAL_NS) {
        unsigned long long frames = c->frames ? c->frames : 1u;
        unsigned long long acc = (unsigned long long)c->acc_ns;
        if (acc == 0)
            acc = 1;
        unsigned val = (unsigned)((frames * (unsigned long long)NS_PER_SEC + acc / 2) / acc);
        c->fps = val;
        c->acc_ns = 0;
        c->frames = 0;
    }
    return c->fps;
}
