#include "../../../include/libc.h"

/* Proceso que corre indefinidamente — útil para probar scheduler */
int loop_proc(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	while (1) {
		for (volatile int i = 0; i < 10000000; i++)
			;
	}
	return 0;
}
int cmd_loop(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	int64_t pid = create_process(loop_proc, 0, (char *[]) {0});
	if (pid < 0) {
		printf("loop: no se pudo crear el proceso\n");
		return 1;
	}
	printf("loop: proceso creado con PID %d\n", (int) pid);
	return 0;
}
