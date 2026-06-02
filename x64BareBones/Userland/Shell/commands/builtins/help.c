#include "../../../include/libc.h"
#include "../commands.h"

int help(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	static const struct {
		CmdCategory cat;
		const char *title;
	} sections[] = {
		{CAT_GENERAL, "General"}, {CAT_MEM, "Memory"}, {CAT_PROC, "Processes"}, {CAT_IPC, "IPC"}, {CAT_TEST, "Tests"},
	};

	printf("Atajos:\n");
	printf("  '0' : captures registers (then 'regs' to print them)\n");
	printf("  Tab : autocompletes written text with matching commands in the history or navigates the command "
		   "history if no text is written \n");

	int nsec = (int) (sizeof(sections) / sizeof(sections[0]));
	for (int s = 0; s < nsec; s++) {
		int first = 1;
		for (int i = 0; commands[i].name != NULL; i++) {
			if (commands[i].category != sections[s].cat)
				continue;
			if (first) { // imprime el título solo si la sección tiene comandos
				printf("\n%s:\n", sections[s].title);
				first = 0;
			}
			printf("  %-14s %s\n", commands[i].name, commands[i].help);
		}
	}
	return 0;
}