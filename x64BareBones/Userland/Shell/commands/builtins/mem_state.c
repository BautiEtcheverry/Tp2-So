#include "../../../include/libc.h"

int mem_state(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	mem_info_t info;
	if (mem_info(&info) != 0) {
		printf("mem: no se pudo obtener el estado de memoria\n");
		return 1;
	}
	printf("Memoria total:    %llu bytes\n", (unsigned long long) info.total_memory);
	printf("Memoria ocupada:  %llu bytes\n", (unsigned long long) info.used_memory);
	printf("Memoria libre:    %llu bytes\n", (unsigned long long) info.free_memory);
	printf("Bloques vivos:    %llu\n", (unsigned long long) info.allocated_blocks);
	return 0;
}