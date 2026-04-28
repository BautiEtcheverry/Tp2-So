#include <naiveConsole.h>

static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base);

static char buffer[64] = {'0'};
static uint8_t *const video = (uint8_t *)0xB8000;
static uint8_t *currentVideo = (uint8_t *)0xB8000;
static const uint32_t width = 80;
static const uint32_t height = 25;
// Default: permanent 1-column left border
static uint32_t marginX = 1;
static uint32_t marginY = 0;
static int cursorInitialized = 0;
static uint8_t attr = 0x07; // light gray on black by default

static inline void setCursor(uint32_t x, uint32_t y)
{
	if (x >= width)
		x = width - 1;
	if (y >= height)
		y = height - 1;
	currentVideo = video + ((y * width + x) * 2);
}

void ncSetMargin(uint32_t x, uint32_t y)
{
	marginX = (x < width) ? x : (width - 1);
	marginY = (y < height) ? y : (height - 1);
	setCursor(marginX, marginY);
	cursorInitialized = 1;
}

void ncSetColor(uint8_t a)
{
	attr = a;
}

void ncPrint(const char *string)
{
	int i;

	for (i = 0; string[i] != 0; i++)
		ncPrintChar(string[i]);
}

void ncPrintChar(char character)
{
	if (!cursorInitialized)
	{
		setCursor(marginX, marginY);
		cursorInitialized = 1;
	}
	*currentVideo = character;
	*(currentVideo + 1) = attr;
	currentVideo += 2;
}

void ncNewline()
{
	// Move to start of next line at current margin
	uint64_t offsetBytes = (uint64_t)(currentVideo - video);
	uint32_t col = (offsetBytes / 2) % width;
	// advance to end of line
	while (col++ < width)
	{
		ncPrintChar(' ');
	}
	// set cursor at next line, same marginX
	uint64_t newOffsetChars = (((offsetBytes / 2) / width) + 1) * width; // next line start
	uint32_t row = (uint32_t)(newOffsetChars / width);
	if (row >= height)
	{
		// simple clear on overflow
		ncClear();
		return;
	}
	setCursor(marginX, row);
}

void ncPrintDec(uint64_t value)
{
	ncPrintBase(value, 10);
}

void ncPrintHex(uint64_t value)
{
	ncPrintBase(value, 16);
}

void ncPrintBin(uint64_t value)
{
	ncPrintBase(value, 2);
}

void ncPrintBase(uint64_t value, uint32_t base)
{
	uintToBase(value, buffer, base);
	ncPrint(buffer);
}

void ncClear()
{
	int i;

	for (i = 0; i < height * width; i++)
	{
		video[i * 2] = ' ';
		video[i * 2 + 1] = attr;
	}
	setCursor(marginX, marginY);
	cursorInitialized = 1;
}

static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base)
{
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	// Calculate characters for each digit
	do
	{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	} while (value /= base);

	// Terminate string in buffer.
	*p = 0;

	// Reverse string in buffer.
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}
