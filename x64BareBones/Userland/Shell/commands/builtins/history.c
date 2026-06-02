#include "../../../include/libc.h"
#include "../core/headers/history.h" 

// Prints the history and allows clearing it with "-c".
// Requires that hist_buf[HIST_MAX][CMD_MAX_LEN+1] and hist_count exist.
int history_find_matches(const char *prefix, const char **out, int cap) {
    int cnt = 0;
    if (cap <= 0)
        return 0;
    for (int i = 0; i < hist_count; i++) {
        int idx = ((hist_head - 1 - i) % HIST_MAX + HIST_MAX) % HIST_MAX;
        const char *h = hist_buf[idx];
        if (!prefix || prefix[0] == 0 || s_starts_with(h, prefix)) {
            out[cnt++] = h;          // puntero, no índice
            if (cnt >= cap)
                break;
        }
    }
    return cnt;
}