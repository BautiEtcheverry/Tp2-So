#include "../../../include/libc.h"

int regs_wrapper(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	regs_print();
	return 0;
}