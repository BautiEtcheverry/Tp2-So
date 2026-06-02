#include "./headers/history.h"
#define HIST_LINE_MAX 64   // tamaño de cada entrada; debe ser >= CMD_MAX_LEN del lector

static char hist_buf[HIST_MAX][HIST_LINE_MAX + 1];
static int hist_count = 0;
static int hist_head = 0; // Next index to write to


/*-----------------Helpers-----------------*/

static int s_eq(const char *a, const char *b) {
	if (!a || !b)
		return 0;
	while (*a && *b && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static int s_starts_with(const char *s, const char *prefix) {
	if (!s || !prefix)
		return 0;
	while (*prefix) {
		if (*s++ != *prefix++)
			return 0;
	}
	return 1;
}

static void s_copy(char *dst, const char *src, size_t max) {
	if (!dst || !src || max == 0)
		return;
	size_t i = 0;
	while (src[i] && i + 1 < max) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = 0;
}
/*----------------------------------------*/

/*-----------------API-----------------*/
void history_add(const char *line) {
	// Avoid duplicating consecutive entries
	if (!line || line[0] == 0)
		return;
	int last_idx = (hist_count == 0) ? -1 : (hist_head + HIST_MAX - 1) % HIST_MAX;
	if (last_idx >= 0 && s_eq(hist_buf[last_idx], line))
		return;
	// Save in position hist_count % HIST_MAX (ring buffer)
	s_copy(hist_buf[hist_count % HIST_MAX], line, HIST_LINE_MAX + 1);
	hist_head = (hist_head + 1) % HIST_MAX;
	if (hist_count < HIST_MAX)
		hist_count++;
}

// Returns the list of indices of history that match the prefix, from newest to oldest.
int history_find_matches(const char *prefix, const char **out, int cap) {
	int cnt = 0;
	if (cap <= 0)
		return 0;
	for (int i = 0; i < hist_count; i++) {
		int idx = (hist_head + HIST_MAX - 1 - i) % HIST_MAX; // iterate from newest to oldest
		const char *h = hist_buf[idx];
		if (!prefix || prefix[0] == 0 || s_starts_with(h, prefix)) {
			out[cnt++] = h;  
			if (cnt >= cap)
				break;
		}
	}
	return cnt;
}
int history_count(void) {
    return hist_count;
}

void history_clear(void) {
    hist_count = 0;
    hist_head  = 0;
}

const char *history_get(int i) {                           // i en [0, count): 0 = más vieja
    if (i < 0 || i >= hist_count) return NULL;
    int idx = ((hist_head - hist_count + i) % HIST_MAX + HIST_MAX) % HIST_MAX;
    return hist_buf[idx];
}