#include <stdint.h>
#include <stddef.h>
#include "../include/libc.h"

#define CMD_MAX_LEN 64
static uint32_t current_text_fg = 0xFFFFFF;


// We keep up to HIST_MAX commands, without duplicating consecutive ones.
#define HIST_MAX 32
static char hist_buf[HIST_MAX][CMD_MAX_LEN + 1];
static int  hist_count = 0; 
static int hist_head = 0; //Next index to write to
static size_t readline_hist(char *buf, size_t max);
static int history_cmd(int argc, char *argv[]);

 
//Prototypes from our userland libc (implemented in ./libc.c)
int printf(const char *fmt, ...);
int scanf(const char *fmt, ...);
void putChar(char c);
char getChar(void);

// Benchmarks (compiled from ../benchmarks)
int benchmark_float(int argc, char *argv[]);
int benchmark_iolat(int argc, char *argv[]);
int benchmark_memlat(int argc, char *argv[]);

// Implemented in ../Tron/tron.c
extern void tron_menu(void);

// Implemented in ./text.c
extern void trigger_div0(void);
extern void trigger_ud(void);

// Implemented in ./regs.c
static void print2u(unsigned v);

// Implemented in ./video.c
static int streq_nocase(const char *a, const char *b);


/*----------------------------------------*/
//To avoid having an if-else structured main, we chose to use a command + funcp table.
//To do that we need function wrappers for those funcs that don't use the cmdFuncP header definition.

typedef int (*cmdFuncP)(int argc, char *argv[]);

static int help(int argc, char *argv[]);
static int ls(int argc, char *argv[]);
static int echo(int argc, char *argv[]);
static int shell_time(int argc, char *argv[]);
static int textColor(int argc, char *argv[]);
static int textSize(int argc, char *argv[]);
static int history_cmd(int argc, char *argv[]);


static int clear_wrapper(int argc, char *argv[]) {
    (void)argc; (void)argv;
    clear_screen();
    return 0;
}

static int tron_wrapper(int argc, char *argv[]) {
    (void)argc; (void)argv;
    tron_menu();
    return 0;
}

static int regs_wrapper(int argc, char *argv[]) {
    (void)argc; (void)argv;
    regs_print();
    return 0;
}

static int trigger_div0_wrapper(int argc, char *argv[]) {
    (void)argc; (void)argv;
    trigger_div0();
    return 0;
}

static int trigger_ud_wrapper(int argc, char *argv[]) {
    (void)argc; (void)argv;
    trigger_ud();
    return 0;
}

//(Command+funcP+help_cmd_msg) table
static const struct {
    const char *name;
    cmdFuncP    fn;
    const char *help;
} 

commands[] = {
    { "help",      help,      "Usage and commands" },
    { "cmd-history",   history_cmd,   "Print command history from newest to oldest (use 'history -c' to clear)" },
    { "ls",        ls,        "List commands" },
    { "clear",     clear_wrapper,     "Clear screen" },
    { "echo",      echo,      "Echo args" },
    { "time",      shell_time,      "Show date/time" },
    { "textColor", textColor, "Change text color" },
    { "textSize", textSize, "Change text size" },
    { "regs",      regs_wrapper,      "Show regs" },
    { "tron",      tron_wrapper,      "Start Tron" },
    { "benchmark-Float",  benchmark_float,  "Run floating-point benchmark" },
    { "benchmark-IOlat",  benchmark_iolat,  "Measure shell I/O latency" },
    { "benchmark-Memlat", benchmark_memlat, "Measure memory latency" },
    { "trigger-div", trigger_div0_wrapper, "Trigger a division by zero exception" },
    { "trigger-ud", trigger_ud_wrapper, "Trigger a invalid opcode(undefined instruction) exception" },
    { NULL,        NULL,          NULL }
};

/*----------------------------------------*/

static int help(int argc, char *argv[]) {
    printf("Usage:\n");
    printf("  Press 0 to capture registers, then run 'regs' to print them.\n");
    printf("  If text is written - Use tab to autocomplete commands with matches from the command history.\n");
    printf("  If no text is written - Use tab to navigate through command history.\n");
    printf("\n");
    printf("  Available commands:\n");
    printf("    cmd-history\n");
    printf("    help\n");
    printf("    ls\n");
    printf("    clear\n");
    printf("    echo [args...]\n");
    printf("    time\n");
    printf("    textColor <name>\n");
    printf("    textColor list\n");
    printf("    textSize <default|large|xlarge>\n");
    printf("    tron\n");
    printf("    regs\n");
    printf("    benchmark-Float [OUT= iterations | ops | warmup]\n");
    printf("    benchmark-IOlat <write|kbd> [OUT= mode | iterations]\n");
    printf("    benchmark-Memlat [OUT= size_kb | stride | iterations]\n");
    printf("    trigger-div\n");
    printf("    trigger-ud\n");
    return 0;
}

static int ls(int argc, char *argv[]){
    (void)argc; (void)argv; //To avoid unused params warings(since we're not using ls as commonly used 
                            // on shells, which is used to print dirs elements. It's static for us rn).
    printf("Commands:\n");
    for (int i = 0; commands[i].name != NULL; ++i) {
        printf("  %s", commands[i].name);
        if (commands[i].help && commands[i].help[0])
            printf(" - %s", commands[i].help);
        printf("\n");
    }
    return 0;
}

static int echo(int argc, char * argv[]){
    for (int i = 1; i < argc; ++i) {
        printf("%s", argv[i]);
        if (i + 1 < argc) printf(" ");
    }
    printf("\n");
    return 0;
}

static int shell_time(int argc, char * argv[]){
    unsigned d, mo, y, h, mi, s;
    time_date_hms(&d, &mo, &y, &h, &mi, &s);
    print2u(d); printf("/"); print2u(mo); printf("/"); printf("%u ", y);
    print2u(h); printf(":"); print2u(mi); printf(":"); print2u(s); printf("\n");
    return 0;
}

static int textColor(int argc, char * argv[]){
    if (argc < 2 || streq_nocase(argv[1], "list")) {
        print_available_text_colors();
        return 0;
    }
    if (set_text_color_name(argv[1]) != 0) {
        printf("Invalid color.\n");
        printf("       textColor list\n");
        return -1;
    }
    current_text_fg = get_color_by_name(argv[1]);
    printf("Text color updated.\n");
    return 0;
}

static int textSize(int argc, char *argv[]){
    if (argc < 2) {
        printf("Usage: textSize <default|large|xlarge>\n");
        return -1;
    }

    const char *arg = argv[1];

    int mode = -1;
    if (streq_nocase(arg, "default")) mode = 0;
    else if (streq_nocase(arg, "large")) mode = 1;
    else if (streq_nocase(arg, "xlarge") || streq_nocase(arg, "extra") || streq_nocase(arg, "extra-large")) mode = 2;
    else {
        printf("Invalid size. Valid values: default, large, xlarge\n");
        return -1;
    }

    int r = set_text_size(mode);
    if (r == 0) {
        const char *names[3] = {"default", "large", "xlarge"};
        printf("Text size set to %s\n", names[mode]);
        return 0;
    } else {
        printf("Failed to set text size (backend error)\n");
        return -1;
    }
}


static void prompt(void){
    /* shell background color: 0x272827 */
    uint32_t shell_bg = 0x272827;
    /* print colored segments via write so colors apply to exactly this output */
    set_colors(0x875FD7, shell_bg);
    write(1, "$TPE-Arqui", 10);
    set_colors(0xFFA657, shell_bg);
    write(1, "> ", 2);
    /* restore default text color for user input */
    set_colors(current_text_fg, shell_bg);
}

static void print2u(unsigned v){
    if (v < 10)
        printf("0%u", v);
    else
        printf("%u", v);
}

static int streq(const char *a, const char *b){
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *a == *b;
}

static int tokenize(char *s, char *argv[], int maxv){
    int argc = 0;
    // Skip leading spaces
    while (*s == ' ')
        s++;
    while (*s && argc < maxv)
    {
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

static char tolower_c(char c){
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static int streq_nocase(const char *a, const char *b){
    while (*a && *b)
    {
        if (tolower_c(*a) != tolower_c(*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
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

// --- Command history + tab navigation ---
static size_t s_len(const char *s) {
    size_t n = 0;
    while (s && s[n]) n++;
    return n;
}

static int s_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

static int s_starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

static void s_copy(char *dst, const char *src, size_t max) {
    if (!dst || !src || max == 0) return;
    size_t i = 0;
    while (src[i] && i + 1 < max) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void history_add(const char *line) {
    // Avoid duplicating consecutive entries
    if (!line || line[0] == 0) 
        return;
    int last_idx = (hist_count == 0) ? -1 : (hist_head + HIST_MAX - 1) % HIST_MAX;
    if (last_idx >= 0 && s_eq(hist_buf[last_idx], line))
        return;
    // Save in position hist_count % HIST_MAX (ring buffer)
    s_copy(hist_buf[hist_count % HIST_MAX], line, CMD_MAX_LEN + 1);
      hist_head = (hist_head + 1) % HIST_MAX;
    if (hist_count < HIST_MAX)
        hist_count++;

}

// Returns the list of indices of history that match the prefix, from newest to oldest. 
static int history_find_matches(const char *prefix, int *out_idx, int cap) {
    int cnt = 0;
    if (cap <= 0) return 0;
    for (int i = 0; i < hist_count; i++) {
         int idx = (hist_head + HIST_MAX - 1 - i) % HIST_MAX;           // iterate from newest to oldest
        const char *h = hist_buf[idx];
        if (!prefix || prefix[0] == 0 || s_starts_with(h, prefix)) {
            out_idx[cnt++] = idx;
            if (cnt >= cap) 
                break;
        }
    }
    return cnt;
}

// -Returns the length of the line read, including '\n' at the end if Enter was pressed.
// -Backspace deletes character in screen and buffer.
// -Tab completes with history. Repeated Tab cycles between matches.
static size_t readline_hist(char *buf, size_t max) {
    size_t n = 0;
    buf[0] = 0;

    int tab_active = 0;          // 0 = no hay ciclo, 1 = se está ciclando matches
    int matches[HIST_MAX];
    int match_count = 0;
    int match_pos = 0;

    for (;;) {
        int ch = getchar();

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
                const char *sugg = hist_buf[matches[match_pos]];
                //Delete the current input
                for (size_t i = 0; i < n; i++) puts("\b \b");
                //Copy and show suggestion
                size_t m = s_len(sugg);
                if (m + 1 > max) 
                    m = (max > 0 ? max - 1 : 0);
                for (size_t i = 0; i < m; i++) 
                    buf[i] = sugg[i];
                buf[m] = 0;
                n = m;
                puts(sugg);
                //Follow next match for the next Tab
                match_pos = (match_pos + 1) % match_count;
            }
            //If no matches, ignore the Tab
            continue;
        }

        tab_active = 0;

        if (ch == '\n') {
            buf[n++] = '\n';
            buf[n] = 0;
            puts("\n");
            //Adds to history without the '\n'
            if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = 0;
            if (buf[0] != 0) history_add(buf);
            
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
            buf[n++] = (char)ch;
            buf[n] = 0;
            char echo[2] = {(char)ch, 0};
            puts(echo);
        }
    }
}

// Prints the history and allows clearing it with "-c".
// Requires that hist_buf[HIST_MAX][CMD_MAX_LEN+1] and hist_count exist.
static int history_cmd(int argc, char *argv[]) {
    // history -c -> clean history
    if (argc >= 2 && streq_nocase(argv[1], "-c")) {

        hist_head = 0;
        hist_count = 0;
        printf("History cleared.\n");
        return 0;
    }
    if (hist_count == 0) {
            printf("History is empty.\n");
            return 0;
    }
        
    for (int i = hist_count - 1, idx = 1; i > 0; --i, ++idx){
           printf(" %d: %s\n", idx, hist_buf[idx]);
       }
    return 0;
}
