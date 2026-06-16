#include "../../../include/libc.h"
#include "../commands.h"

/*
 * Paginado tipo 'more': cuando se llenó la pantalla, muestra un prompt y espera
 * una tecla antes de seguir. Devuelve 1 si el usuario pidió cortar ('q'/EOF).
 * El \r + espacios + \r borra la línea del prompt para reusarla con contenido.
 */
static int help_more(int *line, int page) {
	if (*line < page)
		return 0;
	printf("-- more -- (any key, 'q' to quit)");
	int c = getchar();
	printf("\r                                  \r");
	*line = 0;
	return (c == 'q' || c == 'Q' || c == -1);
}

int help(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	static const struct {
		CmdCategory cat;
		const char *title;
	} sections[] = {
		{CAT_GENERAL, "General"}, {CAT_MEM, "Memory"}, {CAT_PROC, "Processes"}, {CAT_IPC, "IPC"}, {CAT_TEST, "Tests"},
	};

	int page = get_shell_rows() - 2; /* una fila de margen + la del prompt '-- more --' */
	if (page < 4)
		page = 22; /* fallback (modo texto / sin LFB) */
	int line = 0;

	if (help_more(&line, page)) return 0;
	printf("Shortcuts:\n"); line++;
	if (help_more(&line, page)) return 0;
	printf("  'F1' : captures registers (then 'regs' to print them)\n"); line++;
	if (help_more(&line, page)) return 0;
	printf("  Tab : autocomplete from history; if the line is empty, browse history\n"); line++;

	int nsec = (int) (sizeof(sections) / sizeof(sections[0]));
	for (int s = 0; s < nsec; s++) {
		int first = 1;
		for (int i = 0; commands[i].name != NULL; i++) {
			if (commands[i].category != sections[s].cat)
				continue;
			if (first) { // imprime el título solo si la sección tiene comandos
				if (help_more(&line, page)) return 0;
				printf("%s:\n", sections[s].title); line++;
				first = 0;
			}
			if (help_more(&line, page)) return 0;
			printf("  %-14s %s\n", commands[i].name, commands[i].help); line++;
		}
	}
	return 0;
}
