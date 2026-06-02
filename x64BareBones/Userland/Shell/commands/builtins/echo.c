#include "../../../include/libc.h"
#include "../commands.h"

int echo(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		printf("%s", argv[i]);
		if (i + 1 < argc)
			printf(" ");
	}
	printf("\n");
	return 0;
}