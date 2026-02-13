#include "string.h"

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    size_t i;

    for (i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];

        if (ca != cb) return (int)ca - (int)cb;
        if (ca == 0) return 0;
    }

    return 0;
}

void* memcpy(void* dst, const void* src, size_t n) {
    size_t i;
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    for (i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void* memset(void* dst, int value, size_t n) {
    size_t i;
    unsigned char* d = (unsigned char*)dst;

    for (i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dst;
}
