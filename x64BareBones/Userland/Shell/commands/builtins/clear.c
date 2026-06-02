#include "../../../include/libc.h"

int clear_wrapper(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    clear_screen();    // Func de libc
    return 0;
}
