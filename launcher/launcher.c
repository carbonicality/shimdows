/*
 * launcher.c
 * reads from /proc/iomem, loads trampoline, kexec_loads then triggers the jump
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <errno.h>
#include <sys/syscall.h>
#include <sys/reboot.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/reboot.h>

/*kexec syscall defs*/
#define __NR_kexec_load 246

#define KEXEC_ARCH_x86_64 (62 << 16)
#define KEXEC_SEGMENT_MAX 16

struct kexec_segment {
    const void *buf;
    size_t bufsz;
    const void *mem;
    size_t memsz;
};

static long kexec_load(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags)
{
    return syscall(__NR_kexec_load, entry, nr_segments, segments, flags);
}

/*physical mem map*/
#define MAX_MEM_REGIONS 64

typedef struct {
    uint64_t start;
    uint64_t end;
    char type[32];
} mem_region_t;

static mem_region_t mem_regions[MAX_MEM_REGIONS];
static int num_regions=0;

