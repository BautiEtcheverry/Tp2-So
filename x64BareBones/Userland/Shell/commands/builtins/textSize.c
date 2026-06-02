#include "../../../include/libc.h"

int textSize(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: textSize <default|large|xlarge>\n");
		return -1;
	}

	const char *arg = argv[1];

	int mode = -1;
	if (streq_nocase(arg, "default"))
		mode = 0;
	else if (streq_nocase(arg, "large"))
		mode = 1;
	else if (streq_nocase(arg, "xlarge") || streq_nocase(arg, "extra") || streq_nocase(arg, "extra-large"))
		mode = 2;
	else {
		printf("Invalid size. Valid values: default, large, xlarge\n");
		return -1;
	}

	int r = set_text_size(mode);
	if (r == 0) {
		const char *names[3] = {"default", "large", "xlarge"};
		printf("Text size set to %s\n", names[mode]);
		return 0;
	}
	else {
		printf("Failed to set text size (backend error)\n");
		return -1;
	}
}