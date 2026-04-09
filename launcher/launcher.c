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

static int parse_iomem(void)
{
    FILE *f = fopen("/proc/iomem","r");
    if (!f) {
        perror("open /proc/iomem");
        return -1;
    }

    char line[256];
    while (fgets(line,sizeof(line),f)) {
        uint64_t start, end;
        char type[64];

        if (line[0]==' ') continue;
        
        if (sscanf(line, "%1x-%1x : %63[^\n]", &start, &end, type)==3) {
            if (num_regions >= MAX_MEM_REGIONS) break;
            mem_regions[num_regions].start=start;
            mem_regions[num_regions].end=end;
            memcpy(mem_regions[num_regions].type,type,sizeof(mem_regions[0].type)-1);
            num_regions++;
        }
    }
    fclose(f);
    printf("[launcher] parsed %d iomem regions\n",num_regions);
    return 0;
}

static void *load_file(const char *path, size_t *out_size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"[launcher] cannot open %s: %s\n",path,strerror(errno));
        return NULL;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return NULL;
    }

    void *buf = mmap(NULL,st.st_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (buf == MAP_FAILED) {
        buf = malloc(st.st_size);
        if (!buf) {
            fprintf(stderr,"[launcher] OOM loading %s\n",path);
            close(fd);
            return NULL;
        }
    }
    ssize_t n = read(fd,buf,st.st_size);
    close(fd);

    if (n!=st.st_size) {
        fprintf(stderr,"[launcher] short read on %s\n",path);
        free(buf);
        return NULL;
    }
    *out_size = st.st_size;
    printf("[launcher] loaded %s (%zu bytes)\n",path,*out_size);
    return buf;
}