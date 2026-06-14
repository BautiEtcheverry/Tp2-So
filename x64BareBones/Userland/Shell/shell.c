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
/* Nuevos programas IPC */
int cat_main(int argc, char *argv[]);
int wc_main(int argc, char *argv[]);
int filter_main(int argc, char *argv[]);
int loop_main(int argc, char *argv[]);
int endless_loop(int argc, char *argv[]);

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

const Command commands[] = {
	{"help",        help,                "Usage and commands",                              CAT_GENERAL, BUILTIN},
	{"cmd-history", history_cmd,         "Command history (use 'cmd-history -c')",          CAT_GENERAL, BUILTIN},
	{"ls",          ls,                  "List commands",                                   CAT_GENERAL, BUILTIN},
	{"clear",       clear_wrapper,       "Clear screen",                                    CAT_GENERAL, BUILTIN},
	{"echo",        echo,                "Echo args",                                       CAT_GENERAL, BUILTIN},
	{"time",        shell_time,          "Show date/time",                                  CAT_GENERAL, BUILTIN},
	{"textColor",   textColor,           "Change text color",                               CAT_GENERAL, BUILTIN},
	{"textSize",    textSize,            "Change text size",                                CAT_GENERAL, BUILTIN},
	{"regs",        regs_wrapper,        "Show captured registers",                         CAT_GENERAL, BUILTIN},
	{"trigger-div", trigger_div0_wrapper,"Trigger a divide-by-zero exception",              CAT_GENERAL, BUILTIN},
	{"trigger-ud",  trigger_ud_wrapper,  "Trigger an invalid-opcode exception",             CAT_GENERAL, BUILTIN},
	{"mem",         mem_state,           "Print memory status (total/used/free)",           CAT_MEM,     BUILTIN},
	{"ps",          ps,                  "List running processes",                          CAT_PROC,    BUILTIN},
	{"kill",        cmd_kill,            "Kill a process: kill <pid>",                      CAT_PROC,    BUILTIN},
	{"nice",        cmd_nice,            "Change priority: nice <pid> <prio>",              CAT_PROC,    BUILTIN},
	{"block",       cmd_block,           "Toggle block/unblock: block <pid>",               CAT_PROC,    BUILTIN},
	{"unblock",     cmd_unblock,         "Unblock a process: unblock <pid>",                CAT_PROC,    BUILTIN},
	{"loop",        loop_main,           "Print PID greeting (use & for background)",       CAT_PROC,    PROGRAM},
	{"endless_loop",endless_loop,        "Spin forever silently (for tests)",               CAT_PROC,    PROGRAM},
	{"cat",         cat_main,            "Print stdin to stdout",                           CAT_IPC,     PROGRAM},
	{"wc",          wc_main,             "Count lines from stdin",                          CAT_IPC,     PROGRAM},
	{"filter",      filter_main,         "Filter vowels from stdin",                        CAT_IPC,     PROGRAM},
	{"test_mm",     test_mm_wrapper,     "Memory manager stress test: test_mm <max_bytes>", CAT_TEST,    PROGRAM},
	{NULL, NULL, NULL, 0, 0}
};

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


/* Busca la función de un comando por nombre */
static cmdFuncP find_cmd_fn(const char *name) {
    for (int i = 0; commands[i].name; i++)
        if (streq(commands[i].name, name))
            return commands[i].fn;
    return (cmdFuncP)0;
}
static const Command *find_cmd(const char *name) {
    for (int i = 0; commands[i].name != NULL; i++)
        if (streq(commands[i].name, name))
            return &commands[i];
    return NULL;
}

/* PID del proceso foreground actual (para Ctrl+C en readline) */
static uint64_t fg_pid = 0;

int main(void) {
    set_colors(0xFFFFFF, 0x272827);
    clear_screen();
    printf("Shell started. Write 'help' to se available commands.\n");
    /* Buffer ampliado para soportar cmd1 | cmd2 */
    char line[CMD_MAX_LEN * 2 + 4];
    char left[CMD_MAX_LEN + 2], right[CMD_MAX_LEN + 2];
    char *argv1[8], *argv2[8];

    for (;;) {
        prompt();
        size_t n = readline_hist(line, sizeof(line) - 1);
        if (n == 0) continue;
        if (line[n-1] == '\n') line[n-1] = 0;

        /* ---- Detectar pipe ---- */
        char *pipe_sep = (char*)0;
        for (char *p = line; *p; p++) {
            if (*p == '|') { pipe_sep = p; break; }
        }

        if (pipe_sep) {
            /* cmd1 | cmd2 */
            int len1 = (int)(pipe_sep - line);
            for (int i = 0; i < len1 && i < CMD_MAX_LEN; i++) left[i] = line[i];
            left[len1 < CMD_MAX_LEN ? len1 : CMD_MAX_LEN] = 0;

            char *r = pipe_sep + 1;
            while (*r == ' ') r++;
            int len2 = 0;
            while (r[len2] && len2 < CMD_MAX_LEN) { right[len2] = r[len2]; len2++; }
            right[len2] = 0;

            int argc1 = tokenize(left,  argv1, 8);
            int argc2 = tokenize(right, argv2, 8);
            if (argc1 == 0 || argc2 == 0) { printf("Pipe invalido\n"); continue; }

            cmdFuncP fn1 = find_cmd_fn(argv1[0]);
            cmdFuncP fn2 = find_cmd_fn(argv2[0]);
            if (!fn1 || !fn2) { printf("Comando no encontrado\n"); continue; }

            int pipe_id = pipe_open(-1);
            if (pipe_id < 0) { printf("Error al crear pipe\n"); continue; }

            int pipe_res = PIPE_ID_TO_FD(pipe_id);
            int64_t pid1 = create_process_piped(fn1, argc1, argv1, 0,        pipe_res);
            int64_t pid2 = create_process_piped(fn2, argc2, argv2, pipe_res, 1);

            if (pid1 < 0 || pid2 < 0) { printf("Error creando procesos del pipe\n"); continue; }

            fg_pid = (uint64_t)pid2;
            set_foreground(fg_pid);
            waitpid((uint64_t)pid1);
            waitpid((uint64_t)pid2);
            fg_pid = 0;
            set_foreground(0);

        } else {
            /* Comando simple, posiblemente con & */
            char *argv[8];
            int argc = tokenize(line, argv, 8);
            if (argc == 0) continue;

            /* Detectar & al final */
            int background = 0;
            if (argc > 0 && argv[argc-1][0] == '&' && argv[argc-1][1] == 0) {
                background = 1;
                argv[--argc] = (char*)0;
            }
            if (argc == 0) continue;

        
            const Command *cmd = find_cmd(argv[0]);
                if (!cmd) { printf("Comando no encontrado: %s\n", argv[0]); continue; }
                if (cmd->kind == BUILTIN) {
                    cmd->fn(argc, argv);        // corre EN la shell, sincrónico
                    continue;
                }   

            int64_t pid = create_process(cmd->fn, argc, argv);
            if (pid < 0) { printf("Error creando proceso\n"); continue; }

            if (background) {
                printf("[bg] PID %d\n", (int)pid);
            } else {
                fg_pid = (uint64_t)pid;
                set_foreground(fg_pid);
                waitpid((uint64_t)pid);
                fg_pid = 0;
                set_foreground(0);
            }
        }
    }
    return 0;
}

