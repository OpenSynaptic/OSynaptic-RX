/* osrx_b62.c -- Base62 decode for OSynaptic-RX (C89, decode-only) */
#include "osrx_b62.h"

osrx_i32 osrx_b62_decode(const char *s, int len, int *ok)
{
    osrx_u32    val;
    const char *p;
    const char *end;
    unsigned char c;
    int neg;
    int d;

    if (ok) { *ok = 0; }
    if (!s || len <= 0) { return 0; }

    val = 0u;
    p   = s;
    end = s + len;
    neg = 0;

    if (*p == '-') {
        neg = 1;
        ++p;
        if (p >= end) { return 0; }
    }

    while (p < end) {
        c = (unsigned char)*p;
        d = -1;
        if (c >= '0' && c <= '9') {
            d = (int)(c - '0');
        } else if (c >= 'a' && c <= 'z') {
            d = 10 + (int)(c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            d = 36 + (int)(c - 'A');
        } else {
            return 0; /* invalid character */
        }
        val = val * 62u + (osrx_u32)d;
        ++p;
    }

    if (ok) { *ok = 1; }
    return neg ? -(osrx_i32)val : (osrx_i32)val;
}
