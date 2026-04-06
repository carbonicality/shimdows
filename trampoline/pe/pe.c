/*
 * pe.c
 * 
 * PE32+ loader
 * parses, maps and relocates EFI PE images
 */

#include "../../include/pe.h"
#include "../../include/types.h"

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);

/* validation */
static bool pe_validate(const void *data, size_t size)
{
    if (size<sizeof(IMAGE_DOS_HEADER)) return false;
    
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)data;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > size) return false;
    const IMAGE_NT_HEADERS64 *nt =
        (const IMAGE_NT_HEADERS64 *)((const u8 *)data+dos->e_lfanew);
    
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return false;
    if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;

    return true;
}

/*get NT headers*/
static const IMAGE_NT_HEADERS64 *pe_get_nt(const void *data)
{
    const IMAGE_DOS_HEADER  *dos=(const IMAGE_DOS_HEADER *)data;
    return (const IMAGE_NT_HEADERS64 *)((const u8 *)data+dos->e_lfanew);
}

/*map sections*/
static void pe_map_sections(const void *file_data, void *load_base, const IMAGE_NT_HEADERS64 *nt)
{
    u32 num_sections=nt->FileHeader.NumberOfSections;
    
    const IMAGE_SECTION_HEADER *sections =
        (const IMAGE_SECTION_HEADER *)(
            (const u8*)&nt->OptionalHeader +
            nt->FileHeader.SizeOfOptionalHeader +
            sizeof(IMAGE_DATA_DIRECTORY)*IMAGE_NUMBEROF_DIRECTORY_ENTRIES
        );
    
    for (u32 i=0; i<num_sections;i++) {
        const IMAGE_SECTION_HEADER *s = &sections[i];

        void *dest = (u8 *)load_base + s->VirtualAddress;
        u32 raw = s->SizeOfRawData;
        u32 virt = s->VirtualSize;

        /*copy raw data*/
        if (raw > 0 && s->PointerToRawData!=0) {
            const void *src = (const u8 *)file_data + s->PointerToRawData;
            u32 copy_size = raw < virt ? raw : virt;
            memcpy(dest,src,copy_size);
        }

        if (virt > raw) {
            memset((u8 *)dest +raw, 0,virt-raw);
        }
    }
}