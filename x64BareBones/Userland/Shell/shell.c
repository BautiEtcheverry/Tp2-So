#include "../include/libc.h"
#include "core/headers/readline.h"
#include "core/headers/prompt.h"
#include "./commands/commands.h"
#include "tests/tests.h"
#include <stddef.h>
#include <stdint.h>


// Prototypes from our userland libc (implemented in ./libc.c)
int printf(const char *fmt, ...);
int scanf(const char *fmt, ...);
void putChar(char c);
char getChar(void);

// Implemented in ./text.c
extern void trigger_div0(void);
extern void trigger_ud(void);


int streq_nocase(const char *a, const char *b);

/*----------------------------------------*/
// To avoid having an if-else structured main, we chose to use a command + funcp table.
// To do that we need function wrappers for those funcs that don't use the cmdFuncP header definition.

int help(int argc, char *argv[]);
int ls(int argc, char *argv[]);
int echo(int argc, char *argv[]);
int shell_time(int argc, char *argv[]);
int textColor(int argc, char *argv[]);
int textSize(int argc, char *argv[]);
int history_cmd(int argc, char *argv[]);
int mem_state(int argc, char *argv[]);
int clear_wrapper(int argc, char *argv[]);
int regs_wrapper(int argc, char *argv[]);

/* ---- Comandos de gestión de procesos ---- */
int ps(int argc, char *argv[]);
int cmd_kill(int argc, char *argv[]);
int cmd_nice(int argc, char *argv[]);
int cmd_block(int argc, char *argv[]);
int cmd_unblock(int argc, char *argv[]);
int cmd_loop(int argc, char *argv[]);


/* ---------------------------------------- */


static int trigger_div0_wrapper(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	trigger_div0();
	return 0;
}

static int trigger_ud_wrapper(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	trigger_ud();
	return 0;
}
static int test_mm_wrapper(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	test_mm(argc, argv);
	return 0;
}

const Command commands[] = {{"help", help, "Usage and commands", CAT_GENERAL},
							{"cmd-history", history_cmd, "Command history (use 'cmd-history -c')", CAT_GENERAL},
							{"ls", ls, "List commands", CAT_GENERAL},
							{"clear", clear_wrapper, "Clear screen", CAT_GENERAL},
							{"echo", echo, "Echo args", CAT_GENERAL},
							{"time", shell_time, "Show date/time", CAT_GENERAL},
							{"textColor", textColor, "Change text color", CAT_GENERAL},
							{"textSize", textSize, "Change text size", CAT_GENERAL},
							{"regs", regs_wrapper, "Show captured registers", CAT_GENERAL},
							{"trigger-div", trigger_div0_wrapper, "Trigger a divide-by-zero exception", CAT_GENERAL},
							{"trigger-ud", trigger_ud_wrapper, "Trigger an invalid-opcode exception", CAT_GENERAL},
							{"mem", mem_state, "Print memory status (total/used/free)", CAT_MEM},
							{"ps", ps, "List running processes", CAT_PROC},
							{"kill", cmd_kill, "Kill a process: kill <pid>", CAT_PROC},
							{"nice", cmd_nice, "Change priority: nice <pid> <prio>", CAT_PROC},
							{"block", cmd_block, "Block a process: block <pid>", CAT_PROC},
							{"unblock", cmd_unblock, "Unblock a process: unblock <pid>", CAT_PROC},
							{"loop", cmd_loop, "Spawn a looping process", CAT_PROC},
							{"test_mm", test_mm_wrapper, "Memory manager stress test: test_mm <b>", CAT_TEST},
							{NULL, NULL, NULL, 0}};

/*----------------------------------------*/


static int streq(const char *a, const char *b) {
	while (*a && *b && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static int tokenize(char *s, char *argv[], int maxv) {
	int argc = 0;
	// Skip leading spaces
	while (*s == ' ')
		s++;
	while (*s && argc < maxv) {
		argv[argc++] = s;
		while (*s && *s != ' ')
			s++;
		if (!*s)
			break;
		*s++ = 0; // terminate token
		while (*s == ' ')
			s++;
	}
	return argc;
}


int main(void) {
	set_colors(0xFFFFFF, 0x272827);
	clear_screen();
	printf("Shell inicial\n");
	char line[CMD_MAX_LEN + 2];
	char *argv[8];

	for (;;) {
		prompt();
		size_t n = readline_hist(line, CMD_MAX_LEN + 1);
		if (n == 0)
			continue;
		if (n > 0 && line[n - 1] == '\n')
			line[n - 1] = 0;

		int argc = tokenize(line, argv, 8);
		if (argc == 0)
			continue;

		const char *name = argv[0];
		int found = 0;
		for (int i = 0; commands[i].name != NULL; ++i) {
			if (streq(commands[i].name, name) || streq(commands[i].name, name)) {
				commands[i].fn(argc, argv);
				found = 1;
				break;
			}
		}
		if (!found) {
			printf(" Comando no encontrado\n");
		}
	}
	return 0;
}
