#include "../include/libc.h"

// Local helpers (userland-only, no stdlib)
static size_t u_strlen(const char *s)
{
    size_t n = 0;
    if (!s)
        return 0;
    while (s[n])
        n++;
    return n;
}
// Character checks
static int u_isdigit(char c) __attribute__((unused));
static int u_isdigit(char c) { return c >= '0' && c <= '9'; }
static int u_isspace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }

// Integer to string conversions
static void u_rev(char *buf, size_t len){
    size_t i = 0, j = (len == 0 ? 0 : len - 1);
    while (i < j)
    {
        char t = buf[i];
        buf[i] = buf[j];
        buf[j] = t;
        i++;
        j--;
    }
}

// Unsigned integer to string
static size_t u_utoa(unsigned long long v, unsigned base, char *out)
{
    static const char digits[] = "0123456789abcdef";
    if (base < 2 || base > 16)
        base = 10;
    if (v == 0)
    {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    size_t i = 0;
    while (v)
    {
        out[i++] = digits[v % base];
        v /= base;
    }
    u_rev(out, i);
    out[i] = 0;
    return i;
}

// Signed integer to string
static size_t u_itoa(long long x, char *out)
{
    unsigned long long u = (x < 0) ? (unsigned long long)(-x) : (unsigned long long)x;
    size_t n = u_utoa(u, 10, out);
    if (x < 0)
    {
        for (size_t i = n; i > 0; --i)
            out[i] = out[i - 1];
        out[0] = '-';
        out[n + 1] = 0;
        return n + 1;
    }
    return n;
}

// Public API
void putChar(char c){
    char tmp = c;
    write(1, &tmp, 1);
}

char getChar(void) { return (char)getchar(); }

int putchar(int ch){
    char c = (char)ch;
    write(1, &c, 1);
    return (int)(unsigned char)c;
}

static void u_pad(int n, char c) {
    while (n-- > 0) putChar(c);
}

static int u_vprintf(const char *fmt, va_list ap)
{
    int printed = 0;
    for (const char *p = fmt; *p; ++p)
    {
        if (*p != '%') { putChar(*p); printed++; continue; }
        ++p;
        if (!*p) break;

        /* Flags */
        int left = 0;
        if (*p == '-') { left = 1; ++p; }

        /* Width */
        int width = 0;
        while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); ++p; }

        /* Length modifier: l or ll */
        int lng = 0;
        if (*p == 'l') { lng = 1; ++p; }
        if (*p == 'l') { lng = 2; ++p; }

        char buf[32];
        buf[0] = 0;

        switch (*p)
        {
        case '%': putChar('%'); printed++; break;
        case 'c': {
            int c = va_arg(ap, int);
            if (!left) u_pad(width - 1, ' ');
            putChar((char)c); printed++;
            if (left)  u_pad(width - 1, ' ');
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int n = (int)u_strlen(s);
            if (!left) u_pad(width - n, ' ');
            write(1, s, (size_t)n); printed += n;
            if (left)  u_pad(width - n, ' ');
            break;
        }
        case 'd': case 'i': {
            long long v = (lng >= 2) ? va_arg(ap, long long)  :
                          (lng == 1) ? (long long)va_arg(ap, long) :
                                       (long long)va_arg(ap, int);
            int n = (int)u_itoa(v, buf);
            if (!left) u_pad(width - n, ' ');
            write(1, buf, (size_t)n); printed += n;
            if (left)  u_pad(width - n, ' ');
            break;
        }
        case 'u': {
            unsigned long long v = (lng >= 2) ? va_arg(ap, unsigned long long)  :
                                   (lng == 1) ? (unsigned long long)va_arg(ap, unsigned long) :
                                                (unsigned long long)va_arg(ap, unsigned int);
            int n = (int)u_utoa(v, 10, buf);
            if (!left) u_pad(width - n, ' ');
            write(1, buf, (size_t)n); printed += n;
            if (left)  u_pad(width - n, ' ');
            break;
        }
        case 'x': {
            unsigned long long v = (lng >= 2) ? va_arg(ap, unsigned long long) :
                                   (lng == 1) ? (unsigned long long)va_arg(ap, unsigned long) :
                                                (unsigned long long)va_arg(ap, unsigned int);
            int n = (int)u_utoa(v, 16, buf);
            if (!left) u_pad(width - n, ' ');
            write(1, buf, (size_t)n); printed += n;
            if (left)  u_pad(width - n, ' ');
            break;
        }
        case 'p': {
            void *pv = va_arg(ap, void *);
            int n = (int)u_utoa((unsigned long long)(uintptr_t)pv, 16, buf);
            if (!left) u_pad(width - n, ' ');
            write(1, buf, (size_t)n); printed += n;
            if (left)  u_pad(width - n, ' ');
            break;
        }
        default:
            putChar('%'); putChar(*p); printed += 2;
            break;
        }
    }
    return printed;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = u_vprintf(fmt, ap);
    va_end(ap);
    return n;
}

// Minimal scanf: supports %d %u %x %s %c (space-separated tokens), stops on newline
static const char *next_token(const char *s)
{
    while (*s && u_isspace(*s))
        s++;
    return s;
}
static const char *read_token(const char *s, char *buf, size_t max)
{
    size_t i = 0;
    while (*s && !u_isspace(*s) && i + 1 < max)
        buf[i++] = *s++;
    buf[i] = 0;
    return s;
}

static int parse_uint(const char *t, unsigned base, unsigned long long *out)
{
    unsigned long long v = 0;
    if (base == 0)
        base = 10;
    if (base == 16 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
        t += 2;
    if (*t == 0)
        return 0;
    for (; *t; ++t)
    {
        unsigned d;
        if (*t >= '0' && *t <= '9')
            d = *t - '0';
        else if (*t >= 'a' && *t <= 'f')
            d = *t - 'a' + 10;
        else if (*t >= 'A' && *t <= 'F')
            d = *t - 'A' + 10;
        else
            return 0;
        if (d >= base)
            return 0;
        v = v * base + d;
    }
    *out = v;
    return 1;
}

static int parse_int(const char *t, long long *out)
{
    int neg = 0;
    if (*t == '+' || *t == '-')
    {
        neg = (*t == '-');
        t++;
    }
    unsigned long long u;
    if (!parse_uint(t, 10, &u))
        return 0;
    *out = neg ? -(long long)u : (long long)u;
    return 1;
}

int scanf(const char *fmt, ...)
{
    // Read a line from stdin and parse against fmt
    char line[256];
    size_t n = readline(line, sizeof(line));
    (void)n;
    const char *s = line;
    va_list ap;
    va_start(ap, fmt);
    int assigned = 0;
    for (const char *p = fmt; *p; ++p)
    {
        if (*p != '%')
            continue;
        ++p;
        if (!*p)
            break;
        s = next_token(s);
        if (!*s)
            break;
        char tok[128];
        s = read_token(s, tok, sizeof(tok));
        switch (*p)
        {
        case 'c':
        {
            char *pc = va_arg(ap, char *);
            *pc = tok[0] ? tok[0] : '\0';
            assigned++;
        }
        break;
        case 's':
        {
            char *ps = va_arg(ap, char *); // copy token
            size_t L = u_strlen(tok);
            for (size_t i = 0; i <= L; i++)
                ps[i] = tok[i];
            assigned++;
        }
        break;
        case 'd':
        case 'i':
        {
            long long *pv = va_arg(ap, long long *);
            long long v;
            if (parse_int(tok, &v))
            {
                *pv = v;
                assigned++;
            }
        }
        break;
        case 'u':
        {
            unsigned long long *pv = va_arg(ap, unsigned long long *);
            unsigned long long v;
            if (parse_uint(tok, 10, &v))
            {
                *pv = v;
                assigned++;
            }
        }
        break;
        case 'x':
        {
            unsigned long long *pv = va_arg(ap, unsigned long long *);
            unsigned long long v;
            if (parse_uint(tok, 16, &v))
            {
                *pv = v;
                assigned++;
            }
        }
        break;
        default:
            break;
        }
    }
    va_end(ap);
    return assigned;
}
