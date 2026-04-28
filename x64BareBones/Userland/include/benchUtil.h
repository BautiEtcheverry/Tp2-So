#ifndef USERLAND_BENCHMARKS_BENCHUTIL_H
#define USERLAND_BENCHMARKS_BENCHUTIL_H

/* Functios shared by benchmarking files, to avoid using not permitted libraries like stdio.h or stdlib.h*/

int printf(const char *fmt, ...);

// Parses an unsigned long from a string, returns fallback on error
static inline unsigned long parse_ulong(const char *s, unsigned long fallback) {
    if (!s)
        return fallback;
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == 0)
        return fallback;
    unsigned long v = 0;
    while (*s >= '0' && *s <= '9') {
        unsigned long digit = (unsigned long)(*s - '0');
        v = v * 10UL + digit;
        s++;
    }
    return v;
}

// Prints an unsigned 64-bit integer to stdout
static inline void bench_print_u64(unsigned long long v) {
    char rev[32];
    unsigned idx = 0;
    if (v == 0) {
        rev[idx++] = '0';
    } else {
        while (v) {
            rev[idx++] = (char)('0' + (v % 10ULL));
            v /= 10ULL;
        }
    }
    char buf[32];
    unsigned out = 0;
    while (idx > 0) {
        buf[out++] = rev[--idx];
    }
    buf[out] = 0;
    printf("%s", buf);
}

// Prints a signed 64-bit integer to stdout
static inline void bench_print_i64(long long v) {
    if (v < 0) {
        printf("-");
        bench_print_u64((unsigned long long)(-v));
    } else {
        bench_print_u64((unsigned long long)v);
    }
}

#endif /* USERLAND_BENCHMARKS_BENCHUTIL_H */
