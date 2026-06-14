#include "../../include/libc.h"
#include "headers/readline.h"     
#include "headers/history.h"


// -Returns the length of the line read, including '\n' at the end if Enter was pressed.
// -Backspace deletes character in screen and buffer.
// -Tab completes with history. Repeated Tab cycles between matches.
size_t readline_hist(char *buf, size_t max) {
	size_t n = 0;
	buf[0] = 0;

	int tab_active = 0; // 0 = no hay ciclo, 1 = se está ciclando matches
	const char *matches[HIST_MAX];
	int match_count = 0;
	int match_pos = 0;

	for (;;) {
		int ch = getchar();

		if (ch == -1) {  /* EOF: Ctrl+D */
			puts("\n");
			buf[0] = 0;
			return 0;
		}

		if (ch == '\r')
			ch = '\n';

		if (ch == '\t') {
			// Build or use matches based on the current prefix (buf[0..n))
			if (!tab_active) {
				matches[0] = 0;
				match_count = history_find_matches(buf, matches, HIST_MAX);
				match_pos = 0;
				tab_active = 1;
			}
			if (match_count > 0) {
				const char *sugg = matches[match_pos];
				// Delete the current input
				for (size_t i = 0; i < n; i++)
					puts("\b \b");
				// Copy and show suggestion
				size_t m = strlen(sugg);
				if (m + 1 > max)
					m = (max > 0 ? max - 1 : 0);
				for (size_t i = 0; i < m; i++)
					buf[i] = sugg[i];
				buf[m] = 0;
				n = m;
				puts(sugg);
				// Follow next match for the next Tab
				match_pos = (match_pos + 1) % match_count;
			}
			// If no matches, ignore the Tab
			continue;
		}

		tab_active = 0;

		if (ch == '\n') {
			buf[n++] = '\n';
			buf[n] = 0;
			puts("\n");
			// Adds to history without the '\n'
			if (n > 0 && buf[n - 1] == '\n')
				buf[n - 1] = 0;
			if (buf[0] != 0)
				history_add(buf);

			buf[n - 1] = '\n';
			return n;
		}

		if (ch == '\b' || ch == 127) {
			if (n > 0) {
				n--;
				puts("\b \b");
				buf[n] = 0;
			}
			continue;
		}

		if (n + 1 < max) {
			buf[n++] = (char) ch;
			buf[n] = 0;
			char echo[2] = {(char) ch, 0};
			puts(echo);
		}
	}
}
