/* _loader.c for Shell */
#include <stdint.h>

extern char bss;
extern char endOfBinary;

void * memset(void * destiny, int32_t c, uint64_t length);
int main();

int _start() {
	memset(&bss, 0, &endOfBinary - &bss);

	return main();
}

void * memset(void * destiation, int32_t c, uint64_t length) {
    uint8_t chr = (uint8_t)c;
    unsigned char * dst = (unsigned char*)destiation;
    while (length--) dst[length] = chr;
    return destiation;
}
