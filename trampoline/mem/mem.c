/*
 * mem.c
 * implementation of basic memory operations
 */

#include "../../include/types.h"

void *memcpy(void *dst, const void *src, size_t n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    u8 *d = (u8 *)dst;
    while (n--) *d=(u8)c;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const u8 *pa=(const u8 *)a;
    const u8 *pb=(const u8 *)b;
    while (n--) {
        if (*pa != *pb) return (int)*pa-(int)*pb;
        pa++;pb++;
    }
    return 0;
}

size_t strlen16(const u16 *s)
{
    size_t n = 0;
    while (*s++) n++;
    return n;
}

static u8 *alloc_base;
static u8 *alloc_ptr;
static u8 *alloc_end;

void mem_alloc_init(void *base, size_t size)
{
    alloc_base=(u8 *)base;
    alloc_ptr=(u8 *)base;
    alloc_end=(u8 *)base+size;
}

void *mem_alloc(size_t size)
{
    /*align to 8 bytes*/
    size=(size+7)&~7ULL;
    if (alloc_ptr + size>alloc_end) return NULL;
    void *p = alloc_ptr;
    alloc_ptr+=size;
    memset(p,0,size);
    return p;
}

void mem_free(void *p)
{
    (void)p;
}