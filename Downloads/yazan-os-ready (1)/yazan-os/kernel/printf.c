/* Minimal freestanding printf: %s %c %d %u %x %X %p %% with width/0-pad/lengths */
#include "kstring.h"

int kvsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    size_t pos = 0;
#define PUT(ch) do { if (pos + 1 < n) buf[pos] = (char)(ch); pos++; } while (0)

    for (; *fmt; fmt++) {
        if (*fmt != '%') { PUT(*fmt); continue; }
        fmt++;
        int zero = 0, width = 0, lng = 0;
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt++; }
        while (*fmt == 'l' || *fmt == 'z') { lng++; fmt++; }

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int len = (int)strlen(s);
            for (int i = len; i < width; i++) PUT(' ');
            while (*s) PUT(*s++);
            break;
        }
        case 'c': PUT((char)va_arg(ap, int)); break;
        case 'p': {
            uintptr_t ptr = (uintptr_t)va_arg(ap, void *);
            PUT('0'); PUT('x');
            char tmp[20]; int t = 0;
            const char *digits = "0123456789abcdef";
            if (ptr == 0) {
                PUT('0');
            } else {
                do { tmp[t++] = digits[ptr % 16]; ptr /= 16; } while (ptr);
                while (t) PUT(tmp[--t]);
            }
            break;
        }
        case 'd': case 'u': case 'x': case 'X': {
            unsigned long long v;
            int neg = 0;
            if (*fmt == 'd') {
                long long sv = (lng >= 1) ? va_arg(ap, long long) : va_arg(ap, int);
                if (sv < 0) { neg = 1; v = (unsigned long long)(-(sv + 1)) + 1ULL; }
                else v = (unsigned long long)sv;
            } else {
                v = (lng >= 1) ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned int);
            }
            unsigned base = (*fmt == 'd' || *fmt == 'u') ? 10 : 16;
            const char *digits = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            char tmp[32]; int t = 0;
            do { tmp[t++] = digits[v % base]; v /= base; } while (v);
            int len = t + neg;
            if (neg && zero) PUT('-');
            for (int i = len; i < width; i++) PUT(zero ? '0' : ' ');
            if (neg && !zero) PUT('-');
            while (t) PUT(tmp[--t]);
            break;
        }
        case '%': PUT('%'); break;
        case 0:   fmt--; break;
        default:  PUT('%'); PUT(*fmt); break;
        }
    }
    if (n) buf[pos < n ? pos : n - 1] = 0;
#undef PUT
    return (int)pos;
}

int ksnprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = kvsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
