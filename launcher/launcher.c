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

/*trampoline payload hdr*/
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t load_base;
    uint64_t load_size;
    uint64_t entry_offset;
    uint64_t stack_phys;
    uint64_t memmap_offset;
    uint64_t memmap_max_entries;
    uint64_t bootmgr_offset;
    uint64_t bootmgr_size;
    uint64_t winload_offset;
    uint64_t winload_size;
} trampoline_header_t;

#define TRAMPOLINE_MAGIC 0x53484457 /*SHDW (shimdows)*/

/*embed mem map into payload*/
typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t size;
    uint32_t type; /*efi mem type*/
    uint32_t pad;
} launcher_mem_entry_t;

static int build_memmap(trampoline_header_t *hdr, void *payload)
{
    launcher_mem_entry_t *entries = (launcher_mem_entry_t *)((uint8_t *)payload+hdr->memmap_offset);
    uint64_t max = hdr->memmap_max_entries;
    uint64_t n = 0;
    
    for (int i=0; i<num_regions && n<max; i++) {
        mem_region_t *r = &mem_regions[i];
        if (strncmp(r->type,"System RAM",10)!=0) continue;
        uint64_t base = (r->start+0xFFF)&~0xFFFULL;
        uint64_t end = r->end & ~0xFFFULL;
        if (end <= base) continue;
        entries[n].base=base;
        entries[n].size=end-base;
        entries[n].type=7; /*EFI_CONVENTIONAL_MEMORY*/
        entries[n].pad=0;
        n++;
    }

    /*mark trampoline region as EFI_LOADER_CODE so winload doesnt stomp it*/
    if (n<max) {
        entries[n].base=hdr->load_base&~0xFFFULL;
        entries[n].size=(hdr->load_size+0xFFF)&~0xFFFULL;
        entries[n].type=1;
        entries[n].pad=0;
        n++;
    }

    printf("[launcher] built memory map %llu entries\n",(unsigned long long)n);
    
    uint64_t *count_ptr=(uint64_t *)((uint8_t *)payload + hdr->memmap_offset-sizeof(uint64_t));
    *count_ptr = n;
    return 0;
}

static int embed_windows_binaries(trampoline_header_t *hdr, void *payload, const char *bootmgr_path, const char *winload_path)
{
    size_t sz;
    void *data;
    /*bootmgfw*/
    data = load_file(bootmgr_path, &sz);
    if (!data) return -1;
    if (sz > hdr->bootmgr_size) {
        fprintf(stderr,"[launcher] bootmgfw.efi too large (%zu > %llu)\n",sz,(unsigned long long)hdr->bootmgr_size);
        free(data);
        return -1;
    }
    memcpy((uint8_t *)payload+hdr->bootmgr_offset,data,sz);
    /*write actual size just before the blob*/
    *(uint64_t *)((uint8_t *)payload+hdr->bootmgr_offset-8)=sz;
    free(data);
    
    /*winload*/
    data = load_file(winload_path,&sz);
    if (!data) return -1;
    if (sz > hdr->winload_size) {
        fprintf(stderr,"[launcher] winload.efi too large (%zu > %llu)\n",sz,(unsigned long long)hdr->winload_size);
        free(data);
        return -1;
    }
    memcpy((uint8_t *)payload+hdr->winload_offset,data,sz);
    *(uint64_t *)((uint8_t *)payload+hdr->winload_offset-8)=sz;
    free(data);
    printf("[launcher] embedded Windows binaries into payload\n");
    return 0;
}

