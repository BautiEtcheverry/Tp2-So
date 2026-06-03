#include "../../../include/libc.h"

int cat_main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	char buf[256];
	int n;
	while ((n = (int)read(0, buf, sizeof(buf))) > 0)
		write(1, buf, (size_t)n);
	return 0;
}
