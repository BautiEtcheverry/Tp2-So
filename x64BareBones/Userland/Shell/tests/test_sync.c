#include "sys.h"
#include "test_util.h"
#include <stdint.h>
#include <stdio.h>

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

int64_t global; // shared memory
void *sem;

void slowInc(int64_t *p, int64_t inc) {
	uint64_t aux = *p;
	yield(); // This makes the race condition highly probable
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
		slowInc(&global, inc);
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