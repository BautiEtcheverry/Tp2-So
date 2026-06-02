#include "../../../include/libc.h"
#include "../commands.h"

int ls(int argc, char *argv[]) {
	(void) argc;
	(void) argv; // To avoid unused params warings(since we're not using ls as commonly used
				 //  on shells, which is used to print dirs elements. It's static for us rn).
	printf("Commands:\n");
	for (int i = 0; commands[i].name != NULL; ++i) {
		printf("  %s", commands[i].name);
		if (commands[i].help && commands[i].help[0])
			printf(" - %s", commands[i].help);
		printf("\n");
	}
	return 0;
}