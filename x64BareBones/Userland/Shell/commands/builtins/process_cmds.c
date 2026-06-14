#include "../../../include/libc.h"

int cmd_nice(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Usage: nice <pid> <priority>\n");
		return 1;
	}
	uint64_t pid  = str_to_uint(argv[1]);
	int prio = (int) str_to_uint(argv[2]);
	if (prio < 0 || prio > 3) {
		printf("nice: prioridad invalida %d (rango valido: 0-3)\n", prio);
		return 1;
	}
	if (nice(pid, prio) < 0) {
		printf("nice: proceso %u no encontrado o protegido\n", (unsigned) pid);
		return 1;
	}
	return 0;
}
int cmd_kill(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: kill <pid>\n");
		return 1;
	}
	uint64_t pid = str_to_uint(argv[1]);
	if (kill(pid) < 0) {
		printf("kill: no such process %u\n", (unsigned) pid);
		return 1;
	}
	return 0;
}
int cmd_block(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: block <pid>\n");
		return 1;
	}
	uint64_t pid = str_to_uint(argv[1]);
	if (block(pid) < 0) {
		printf("block: no such process %u\n", (unsigned) pid);
		return 1;
	}
	return 0;
}
int cmd_unblock(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: unblock <pid>\n");
		return 1;
	}
	uint64_t pid = str_to_uint(argv[1]);
	if (unblock(pid) < 0) {
		printf("unblock: no such process %u\n", (unsigned) pid);
		return 1;
	}
	return 0;
}
