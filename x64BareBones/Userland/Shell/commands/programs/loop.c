#include "../../../include/libc.h"

/* loop: imprime su PID con un saludo cada cierto tiempo (espera activa) */
int loop_main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	unsigned my_pid = (unsigned)getpid();
	while (1) {
		printf("loop: hola desde PID %u\n", my_pid);
		for (volatile long i = 0; i < 500000000L; i++);
	}
	return 0;
}

/* endless_loop: corre indefinidamente sin imprimir (para tests de scheduler) */
int endless_loop(int argc, char *argv[]) {
	(void)argc; (void)argv;
	while (1) {
		for (volatile int i = 0; i < 10000000; i++);
	}
	return 0;
}

