#include "../../../include/libc.h"
#include "../commands.h"


static void print2u(unsigned v) {
	if (v < 10)
		printf("0%u", v);
	else
		printf("%u", v);
}

int shell_time(int argc, char *argv[]) {
	unsigned d, mo, y, h, mi, s;
	time_date_hms(&d, &mo, &y, &h, &mi, &s);
	print2u(d);
	printf("/");
	print2u(mo);
	printf("/");
	printf("%u ", y);
	print2u(h);
	printf(":");
	print2u(mi);
	printf(":");
	print2u(s);
	printf("\n");
	return 0;
}