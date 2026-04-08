/*
 * trampoline.c
 * called from entry.S after we have a stack and GDT.
 */

#include "../include/types.h"
#include "../include/efi.h"
#include "../include/pe.h"

/* external syms */
extern void fixup_gdt_base(void);
extern void jump_to_bootmgr(void *entry, void *image_handle, void *system_table);

extern void *pe_load(const void *file_data, size_t file_size, void *load_base);
extern u32 pe_image_size(const void *file_data, size_t file_size);

extern void mem_alloc_init(void *base, size_t size);
extern void *mem_alloc(size_t size);
extern void mem_free(void *p);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);

/*syms from linker*/
extern u64 _memmap_count;
extern u8 _memmap_entries;
extern u64 _bootmgr_size;
extern u8 _bootmgr_data;
extern u64 _winload_size;
extern u8 _winload_data;
extern u8 _pool_start;
extern u8 _pool_end;
extern u8 _image_load_area; /*where we map bootmgfw*/
extern u8 _winload_load_area; /*where we map winload*/

/*launcher mem entry*/
typedef struct __attribute__((packed)) {
    u64 base;
    u64 size;
    u32 type;
    u32 pad;
} launcher_mem_entry_t;

/*mem map state*/
#define MAX_EFI_MEM_DESCRIPTORS 128

static EFI_MEMORY_DESCRIPTOR efi_memmap[MAX_EFI_MEM_DESCRIPTORS];
static u64 efi_memmap_count=0;
static u64 efi_memmap_key=1;

static void build_efi_memmap(void)
{
    u64 n=_memmap_count;
    if (n>MAX_EFI_MEM_DESCRIPTORS) n=MAX_EFI_MEM_DESCRIPTORS;
    
    launcher_mem_entry_t *entries=(launcher_mem_entry_t *)&_memmap_entries;
    for (u64 i=0;i<n;i++) {
        efi_memmap[i].Type = entries[i].type;
        efi_memmap[i].Pad = 0;
        efi_memmap[i].PhysicalStart = entries[i].base;
        efi_memmap[i].VirtualStart = entries[i].base;
        efi_memmap[i].NumberOfPages = entries[i].size/4096;
        efi_memmap[i].Attribute = EFI_MEMORY_WB;
    }
    efi_memmap_count=n;
}

static bool guid_eq(const EFI_GUID *a, const EFI_GUID *b)
{
    return memcmp(a,b,sizeof(EFI_GUID))==0;
}

/*handle database*/
