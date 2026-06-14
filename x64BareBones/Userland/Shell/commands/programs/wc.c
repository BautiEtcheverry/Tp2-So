#include "../../../include/libc.h"

/* Cuenta las líneas leídas de un fd (stdin o pipe) hasta EOF. */
static int count_lines_fd(int fd) {
	char buf[256];
	int n, lines = 0, last_nl = 1;
	while ((n = (int)read(fd, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < n; i++) {
			if (buf[i] == '\n') { lines++; last_nl = 1; }
			else last_nl = 0;
		}
	}
	if (!last_nl) lines++;
	return lines;
}

int wc_main(int argc, char *argv[]) {
	/* Sin operandos: contar de stdin (uso típico: `cmd | wc`). */
	if (argc <= 1) {
		printf("%d\n", count_lines_fd(0));
		return 0;
	}

	/* Con operandos, como en Linux: cada uno es un archivo a contar, y '-'
	 * representa stdin. Este SO no tiene filesystem, así que cualquier nombre
	 * distinto de '-' no se puede abrir → error (igual que wc ante un archivo
	 * inexistente). Importante: NO se lee stdin por nombres de archivo, lo que
	 * evita que `wc . . .` quede colgado esperando el teclado. */
	int status = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 0)
			printf("%d\n", count_lines_fd(0));
		else {
			printf("wc: %s: No such file or directory\n", argv[i]);
			status = 1;
		}
	}
	return status;
}
