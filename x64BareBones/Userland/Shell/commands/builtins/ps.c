#include "../../../include/libc.h"

int ps(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	ProcessInfo buf[32];
	int n = get_processes(buf, 32);
	static const char *states[] = {"READY", "RUNNING", "BLOCKED", "DEAD"};
	printf("%-5s %-16s %-8s %-4s %-3s %-18s %s\n",
	       "PID", "NAME", "STATE", "PRI", "FG", "STACK_BASE", "RSP");
	printf("%-5s %-16s %-8s %-4s %-3s %-18s %s\n",
	       "---", "----", "-----", "---", "--", "----------", "---");
	for (int i = 0; i < n; i++) {
		const char *st = (buf[i].state >= 0 && buf[i].state <= 3) ? states[buf[i].state] : "?";
		printf("%-5u %-16s %-8s %-4d %-3s 0x%016llx 0x%llx\n",
		       (unsigned)buf[i].pid, buf[i].name, st, buf[i].priority,
		       buf[i].foreground ? "yes" : "no",
		       (unsigned long long)buf[i].stack_base,
		       (unsigned long long)buf[i].rsp);
	}
	return 0;
}