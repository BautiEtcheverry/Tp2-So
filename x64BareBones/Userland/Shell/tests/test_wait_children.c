
#include "sys.h"
#include "test_util.h"
#include <stdint.h>
#include <stdio.h>

static uint64_t child_task(uint64_t argc, char *argv[]) {
	int pid = getPid();
	printf("wait_children child %d starting\n", pid);

	// Simulate a bit of work
	for (int i = 0; i < 3; i++) {
		yield();
	}

	printf("wait_children child %d exiting\n", pid);
	return 0;
}

uint64_t test_wait_children(uint64_t argc, char *argv[]) {
	int child_count = 3;

	if (argc > 1) {
		int parsed = satoi(argv[1]);
		if (parsed <= 0) {
			printf("test_wait_children: invalid child count '%s'\n", argv[1]);
			return -1;
		}
		child_count = parsed;
	}

	printf("wait_children parent %d creating %d child processes\n", getPid(), child_count);

	for (int i = 0; i < child_count; i++) {
		int32_t pid = createProcess((void *) child_task, 0, NULL, 0);
		if (pid < 0) {
			printf("test_wait_children: failed to create child %d\n", i);
			return -1;
		}
		printf("wait_children parent created child pid %d\n", pid);
	}

	printf("wait_children parent waiting for all children...\n");
	if (waitChildren() != 0) {
		printf("test_wait_children: waitChildren returned error\n");
		return -1;
	}

	printf("wait_children parent resumed; all children finished\n");

	// Ensure calling again returns immediately with no children left
	if (waitChildren() != 0) {
		printf("test_wait_children: waitChildren should be idempotent when no children remain\n");
		return -1;
	}

	printf("wait_children test completed successfully\n");
	return 0;
}
