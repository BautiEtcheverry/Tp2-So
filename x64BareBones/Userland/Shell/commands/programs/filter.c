#include "../../../include/libc.h"

int filter_main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	char buf[256];
	int n;
	while ((n = (int)read(0, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < n; i++) {
			char c = buf[i];
			if (c!='a'&&c!='e'&&c!='i'&&c!='o'&&c!='u'&&
			    c!='A'&&c!='E'&&c!='I'&&c!='O'&&c!='U')
				write(1, &c, 1);
		}
	}
	return 0;
}
