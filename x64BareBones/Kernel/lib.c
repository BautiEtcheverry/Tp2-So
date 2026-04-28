#include <stdint.h>

void *memset(void *destination, int32_t c, uint64_t length)
{
	uint8_t chr = (uint8_t)c;
	char *dst = (char *)destination;

	while (length--)
		dst[length] = chr;

	return destination;
}

void *memcpy(void *destination, const void *source, uint64_t length)
{
	/*
	 * memcpy does not support overlapping buffers, so always do it
	 * forwards. (Don't change this without adjusting memmove.)
	 */
	uint64_t i;

	if ((uint64_t)destination % sizeof(uint32_t) == 0 &&
		(uint64_t)source % sizeof(uint32_t) == 0 &&
		length % sizeof(uint32_t) == 0)
	{
		uint32_t *d = (uint32_t *)destination;
		const uint32_t *s = (const uint32_t *)source;

		for (i = 0; i < length / sizeof(uint32_t); i++)
			d[i] = s[i];
	}
	else
	{
		uint8_t *d = (uint8_t *)destination;
		const uint8_t *s = (const uint8_t *)source;

		for (i = 0; i < length; i++)
			d[i] = s[i];
	}

	return destination;
}

// Compare two memory regions.
int memcmp(const void *s1, const void *s2, uint64_t n)
{
	const uint8_t *a = (const uint8_t *)s1;
	const uint8_t *b = (const uint8_t *)s2;
	for (uint64_t i = 0; i < n; i++)
	{
		if (a[i] != b[i])
			return (int)a[i] - (int)b[i];
	}
	return 0;
}

// Move memory handling overlap safely.
void *memmove(void *dest, const void *src, uint64_t n)
{
	uint8_t *d = (uint8_t *)dest;
	const uint8_t *s = (const uint8_t *)src;
	if (d == s || n == 0)
		return dest;
	if (d < s)
	{
		for (uint64_t i = 0; i < n; i++)
			d[i] = s[i];
	}
	else
	{
		for (uint64_t i = n; i != 0; i--)
			d[i - 1] = s[i - 1];
	}
	return dest;
}
