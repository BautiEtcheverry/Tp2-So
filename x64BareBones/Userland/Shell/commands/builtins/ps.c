#include "../../../include/libc.h"

int ps(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	ProcessInfo buf[32];
	int n = get_processes(buf, 32);
	static const char *states[] = {"READY", "RUNNING", "BLOCKED", "DEAD"};
	printf("%-5s %-16s %-8s %-4s %s\n", "PID", "NAME", "STATE", "PRIO", "FG");
	printf("%-5s %-16s %-8s %-4s %s\n", "----", "----------------", "--------", "----", "--");
	for (int i = 0; i < n; i++) {
		const char *st = (buf[i].state >= 0 && buf[i].state <= 3) ? states[buf[i].state] : "?";
		printf("%-5u %-16s %-8s %-4d %s\n", (unsigned) buf[i].pid, buf[i].name, st, buf[i].priority,
			   buf[i].foreground ? "yes" : "no");
	}
	return 0;
}