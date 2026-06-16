#include "sys.h"
#include "test_util.h"
#include <stdint.h>
#include <stdio.h>

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

/* Espera activa entre leer y escribir la global. Ensancha la ventana de la race
 * lo suficiente para que el timer preempte al proceso y otro se intercale.
 * Si da 0 sin semaforo (la race no se ve), subir este valor; si tarda mucho, bajarlo. */
#define SLOWINC_SPIN 1000000

int64_t global; // shared memory
void *sem;

void slowInc(int64_t *p, int64_t inc, uint32_t spin) {
	uint64_t aux = *p;
	yield(); // cede el CPU: el quantum queda en 0, el switch cae en el proximo tick
	/* Espera activa para que ese switch ocurra ENTRE la lectura y la escritura.
	 * Sin esto la ventana son 2 instrucciones y el timer nunca la pisa, asi que la
	 * race no se ve (siempre 0). Con la ventana ancha, otro proceso lee el valor
	 * viejo y se pierden updates -> resultado != 0 y variable. Con semaforo se pasa
	 * spin=0: la seccion critica ya esta serializada (da 0) y no hace falta esperar. */
	for (volatile uint32_t i = 0; i < spin; i++)
		;
	aux += inc;
	*p = aux;
}

uint64_t my_process_inc(uint64_t argc, char *argv[]) {
	uint64_t n;
	int8_t inc;
	int8_t use_sem;

	if (argc != 3)
		return -1;

	if ((n = satoi(argv[0])) <= 0)
		return -1;
	if ((inc = satoi(argv[1])) == 0)
		return -1;
	if ((use_sem = satoi(argv[2])) < 0)
		return -1;

	uint64_t i;
	for (i = 0; i < n; i++) {
		if (use_sem)
			semWait(sem);
		/* La espera activa solo SIN semaforo: es para exponer la race. Con semaforo
		 * la seccion critica ya esta serializada (resultado 0) y la espera solo
		 * haria el test lentisimo, asi que la salteamos (spin=0). */
		slowInc(&global, inc, use_sem ? 0 : SLOWINC_SPIN);
		if (use_sem)
			semPost(sem);
	}

	return 0;
}

uint64_t test_sync(uint64_t argc, char *argv[]) { //{n, use_sem, 0}
	uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

	if (argc != 2)
		return -1;

	char *argvDec[] = {argv[0], "-1", argv[1], NULL};
	char *argvInc[] = {argv[0], "1", argv[1], NULL};

	int use_sem = satoi(argv[1]);
	if (use_sem) {
		sem = semInit(SEM_ID, 1);
		if (sem == NULL) {
			printf("test_sync: ERROR opening semaphore\n");
			return -1;
		}
	}

	global = 0;

	uint64_t i;
	for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
		pids[i] = createProcess((void *) my_process_inc, 3, (uint8_t **) argvDec, 0);
		pids[i + TOTAL_PAIR_PROCESSES] = createProcess((void *) my_process_inc, 3, (uint8_t **) argvInc, 0);
	}

	for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
		waitPid(pids[i]);
		waitPid(pids[i + TOTAL_PAIR_PROCESSES]);
	}

	if (use_sem)
		semDestroy(sem);

	printf("Final value: %d\n", global);

	return 0;
}