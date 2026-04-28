#ifndef NAIVE_CONSOLE_H
#define NAIVE_CONSOLE_H

#include <stdint.h>

void ncPrint(const char *string);
void ncPrintChar(char character);
void ncNewline();
void ncPrintDec(uint64_t value);
void ncPrintHex(uint64_t value);
void ncPrintBin(uint64_t value);
void ncPrintBase(uint64_t value, uint32_t base);
void ncClear();

// Optional: set a left/top margin (in character cells) for text printing in text mode.
// Affects ncClear() starting cursor and ncNewline() indentation on new lines.
void ncSetMargin(uint32_t x, uint32_t y);

// Set foreground/background color attribute (VGA text mode attribute byte).
// Typical values: 0x07 light gray on black, 0x0F white on black, 0x1F white on blue, etc.
void ncSetColor(uint8_t attr);

#endif