#pragma once
#include "types.h"

/*dos header*/
typedef struct {
    u16 e_magic;
    u8 e_pad[58];
    u32 e_lfanew;
} __attribute__((packed)) IMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550

/*file hdr*/
typedef struct {
    u16 Machine;
    u16 NumberOfSections;
    u32 TimeDateStamp;
    u32 PointerToSymbolTable;
    u32 NumberOfSymbols;
    u16 SizeOfOptionalHeader;
    u16 Characteristics;
} __attribute__((packed)) IMAGE_FILE_HEADER;

#define IMAGE_FILE_MACHINE_AMD64 0x8664

/*hdr for PE32*/
typedef struct {
    /*standard*/
    u16 Magic;
    u8 MajorLinkerVersion;
    u8 MinorLinkerVersion;
    u32 SizeOfCode;
    u32 SizeOfInitializedData;
    u32 SizeOfUninitializedData;
    u32 AddressOfEntryPoint;
    u32 BaseOfCode;

    /*windows specific*/
    u64 ImageBase;
    u32 SectionAlignment;
    u32 FileAlignment;
    u16 MajorOSVersion;
    u16 MinorOSVersion;
    u16 MajorImageVersion;
    u16 MinorImageVersion;
    u16 MajorSubsystemVersion;
    u16 MinorSubsystemVersion;
    u32 Win32VersionValue;
    u32 SizeOfImage;
    u32 SizeOfHeaders;
    u32 CheckSum;
    u16 Subsystem;
    u16 DllCharacteristics;
    u64 SizeOfStackReserve;
    u64 SizeOfStackCommit;
    u64 SizeOfHeapReserve;
    u64 SizeOfHeapCommit;
    u32 LoaderFlags;
    u32 NumberOfRvaAndSizes;
} __attribute__((packed)) IMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20B

/*data dir*/
typedef struct {
    u32 VirtualAddress;
    u32 Size;
} __attribute__((packed)) IMAGE_DATA_DIRECTORY;

#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/*NT headers*/
typedef struct {
    u32 Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} __attribute__((packed)) IMAGE_NT_HEADERS64;

/*section header*/
typedef struct {
    char Name[8];
    u32 VirtualSize;
    u32 VirtualAddress;
    u32 SizeOfRawData;
    u32 PointerToRawData;
    u32 PointerToRelocations;
    u32 PointerToLinenumbers;
    u16 NumberOfRelocations;
    u16 NumberOfLinenumbers;
    u32 Characteristics;
} __attribute__((packed)) IMAGE_SECTION_HEADER;

#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000

/*base relocations*/
typedef struct {
    u32 VirtualAddress;
    u32 SizeOfBlock;
} __attribute__((packed)) IMAGE_BASE_RELOCATION;

#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_DIR64 10
