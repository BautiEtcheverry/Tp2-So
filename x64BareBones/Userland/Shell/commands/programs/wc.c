#include "../../../include/libc.h"

int wc_main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	char buf[256];
	int n, lines = 0, last_nl = 1;
	while ((n = (int)read(0, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < n; i++) {
			if (buf[i] == '\n') { lines++; last_nl = 1; }
			else last_nl = 0;
		}
	}
	if (!last_nl) lines++;
	printf("%d\n", lines);
	return 0;
}
