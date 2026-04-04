#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef u64 uintptr_t;
typedef u64 size_t;
typedef s64 ssize_t;

#define NULL ((void*)0)
#define true 1
#define false 0
typedef u8 bool8;

#define PAGE_SIZE 4096ULL
#define PAGE_ALIGN(x) (((x)+PAGE_SIZE-1)&~(PAGE_SIZE-1))
#define PAGE_ALIGN_DOWN(x) ((x)&~(PAGE_SIZE-1))