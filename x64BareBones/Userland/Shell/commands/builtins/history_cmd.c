#include "../../../include/libc.h"
#include "../core/headers/history.h" 


int history_cmd(int argc, char *argv[]) {
    if (argc >= 2 && streq_nocase(argv[1], "-c")) {
        history_clear();
        printf("History cleared.\n");
        return 0;
    }
    int n = history_count();
    if (n == 0) { printf("History is empty.\n"); return 0; }
    for (int i = n - 1, num = 1; i >= 0; i--, num++)
        printf(" %d: %s\n", num, history_get(i));
    return 0;
}