#include "../../../include/libc.h"
#include "../../core/headers/prompt.h"

int textColor(int argc, char *argv[]) {
	if (argc < 2 || streq_nocase(argv[1], "list")) {
		print_available_text_colors();
		return 0;
	}
	if (set_text_color_name(argv[1]) != 0) {
		printf("Invalid color.\n");
		printf("       textColor list\n");
		return -1;
	}
    prompt_set_fg(get_color_by_name(argv[1]));   
    printf("Text color updated.\n");
	return 0;
}