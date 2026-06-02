#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>
// We keep up to HIST_MAX commands, without duplicating consecutive ones.
#define HIST_MAX 32
void history_add(const char *line); // guarda una línea
int history_count(void);			// cuántas hay
void history_clear(void);			// comando "cmd-history -c"
const char *history_get(int i);		// i-ésima (0 = más vieja)

// llena "out" con punteros a los matches por prefijo, del más nuevo al más viejo;
// devuelve cuántos encontró (<= cap)
int history_find_matches(const char *prefix, const char **out, int cap);

#endif