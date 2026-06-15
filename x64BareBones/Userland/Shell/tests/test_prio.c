#include "sys.h"
#include "test_util.h"
#include <stdint.h>

#define TOTAL_PROCESSES 3

#define LOWEST 0
#define MEDIUM 1
#define HIGHEST 2

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

uint64_t max_value = 0;

void zero_to_max() {
	uint64_t value = 0;

	while (value++ != max_value)
		;

	printf("PROCESS %d DONE!\n", getPid());
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
	int64_t pids[TOTAL_PROCESSES];
	char *ztm_argv[] = {0};
	uint64_t i;

	if (argc != 2)
		return -1;

	if ((max_value = satoi(argv[1])) <= 0)
		return -1;

	printf("SAME PRIORITY...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++)
		pids[i] = createProcess((void *) zero_to_max, 0, (uint8_t **) ztm_argv, 0);

	// Expect to see them finish at the same time

	for (i = 0; i < TOTAL_PROCESSES; i++)
		waitPid(pids[i]);

	printf("SAME PRIORITY, THEN CHANGE IT...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = createProcess((void *) zero_to_max, 0, (uint8_t **) ztm_argv, 0);
		nice(pids[i], prio[i]);
		printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], prio[i]);
	}

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		waitPid(pids[i]);

	printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = createProcess((void *) zero_to_max, 0, (uint8_t **) ztm_argv, 0);
		block(pids[i]);
		nice(pids[i], prio[i]);
		printf("  PROCESS %d NEW PRIORITY: %d\n", pids[i], prio[i]);
	}

	for (i = 0; i < TOTAL_PROCESSES; i++)
		unblock(pids[i]);

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		waitPid(pids[i]);

	printf("test_prio: DONE\n");
	return 0;
}
