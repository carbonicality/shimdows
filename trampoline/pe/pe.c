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

/*apply base relocs*/
static void pe_apply_relocs(void *load_base, const IMAGE_NT_HEADERS64 *nt, u64 preferred_base)
{
    s64 delta = (s64)(uintptr_t)load_base - (s64)preferred_base;
    if (delta==0) return;

    const IMAGE_DATA_DIRECTORY *reloc_dir=&nt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    
    if (reloc_dir->VirtualAddress==0||reloc_dir->Size==0) return;

    const u8 *reloc_data = (const u8 *)load_base + reloc_dir->VirtualAddress;
    const u8 *reloc_end = reloc_data+reloc_dir->Size;

    while (reloc_data < reloc_end) {
        const IMAGE_BASE_RELOCATION *block=(const IMAGE_BASE_RELOCATION *)reloc_data;

        if (block->SizeOfBlock<sizeof(IMAGE_BASE_RELOCATION)) break;

        u32 num_entries = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/sizeof(u16);
        const u16 *entries=(const u16*)(block+1);

        for (u32 i=0; i<num_entries;i++) {
            u16 entry = entries[i];
            u8 type = entry >> 12;
            u16 offset = entry & 0x0FFF;

            if (type == IMAGE_REL_BASED_ABSOLUTE) continue; /*padding*/

            if (type == IMAGE_REL_BASED_DIR64) {
                u64 *target = (u64 *)((u8 *)load_base + block->VirtualAddress+offset);
                *target+=delta;
            }
        }
        reloc_data+=block->SizeOfBlock;
    }
}