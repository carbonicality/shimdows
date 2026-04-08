#pragma once
#include "types.h"

/*UEFI types*/
typedef u64 EFI_STATUS;
typedef void* EFI_HANDLE;
typedef void* EFI_EVENT;
typedef u64 EFI_LBA;
typedef u64 EFI_PHYSICAL_ADDRESS;
typedef u64 EFI_VIRTUAL_ADDRESS;
typedef u16 CHAR16;

#define EFI_SUCCESS 0ULL
#define EFI_ERROR_BIT (1ULL << 63)
#define EFI_NOT_FOUND (EFI_ERROR_BIT | 14)
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_BIT | 5)
#define EFI_INVALID_PARAMETER (EFI_ERROR_BIT | 2)
#define EFI_UNSUPPORTED (EFI_ERROR_BIT | 3)
#define EFI_OUT_OF_RESOURCES (EFI_ERROR_BIT | 9)


/*GUID*/
typedef struct {
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8 Data4[8];
} __attribute__((packed)) EFI_GUID;

#define GUID(a,b,c,d0,d1,d2,d3,d4,d5,d6,d7) \
    {(a),(b),(c),{(d0),(d1),(d2),(d3),(d4),(d5),(d6),(d7)}}

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    GUID(0x5B1B31A1,0x9562,0x11d2,0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B)
#define EFI_BLOCK_IO_PROTOCOL_GUID \
    GUID(0x964e5b21,0x6459,0x11d2,0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b)
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    GUID(0x964e5b22,0x6459,0x11d2,0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b)
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    GUID(0x9042a9de,0x23dc,0x4a38,0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a)
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    GUID(0x09576e91,0x6d3f,0x11d2,0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b)

typedef struct {
    u64 Signature;
    u32 Revision;
    u32 HeaderSize;
    u32 CRC32;
    u32 Reserved;
} __attribute__((packed)) EFI_TABLE_HEADER;

/*mem map*/
#define EFI_CONVENTIONAL_MEMORY 7
#define EFI_LOADER_CODE 1
#define EFI_LOADER_DATA 2
#define EFI_BOOT_SERVICES_CODE 3
#define EFI_BOOT_SERVICES_DATA 4
#define EFI_RUNTIME_SERVICES_CODE 5
#define EFI_RUNTIME_SERVICES_DATA 6
#define EFI_UNUSABLE_MEMORY 8
#define EFI_ACPI_RECLAIM_MEMORY 9
#define EFI_ACPI_MEMORY_NVS 10
#define EFI_MEMORY_MAPPED_IO 11
#define EFI_RESERVED_MEMORY_TYPE 0

#define EFI_MEMORY_UC 0x0000000000000001ULL
#define EFI_MEMORY_WC 0x0000000000000002ULL
#define EFI_MEMORY_WT 0x0000000000000004ULL
#define EFI_MEMORY_WB 0x0000000000000008ULL
#define EFI_MEMORY_UCE 0x0000000000000010ULL
#define EFI_MEMORY_WP 0x0000000000001000ULL
#define EFI_MEMORY_RP 0x0000000000002000ULL
#define EFI_MEMORY_XP 0x0000000000004000ULL
#define EFI_MEMORY_RUNTIME 0x8000000000000000ULL

typedef struct {
    u32 Type;
    u32 pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    u64 NumberOfPages;
    u64 Attribute;
} __attribute__((packed)) EFI_MEMORY_DESCRIPTOR;

#define EFI_MEMORY_DESCRIPTOR_VERSION 1

/*allo type*/
typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

/*device path*/
typedef struct {
    u8 Type;
    u8 SubType;
    u8 Length[2];
} __attribute__((packed)) EFI_DEVICE_PATH_PROTOCOL;

/*eop node*/
#define END_DEVICE_PATH_TYPE 0x7f
#define END_ENTIRE_DEVICE_PATH_SUBTYPE 0xff

/*loaded image protocol*/
typedef struct {
    u32 Revision;
    EFI_HANDLE ParentHandle;
    void* SystemTable;
    EFI_HANDLE DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL* FilePath;
    void* Reserved;
    u32 LoadOptionsSize;
    void* LoadOptions;
    void* ImageBase;
    u64 ImageSize;
    u32 ImageCodeType;
    u32 ImageDataType;
    void* Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

/*block io protocol*/
typedef struct {
    u32 MediaId;
    bool RemovableMedia;
    bool MediaPresent;
    bool LogicalPartition;
    bool ReadOnly;
    bool WriteCaching;
    u32 BlockSize;
    u32 IoAlign;
    u64 LastBlock;
} EFI_BLOCK_IO_MEDIA;

typedef struct _EFI_BLOCK_IO_PROTOCOL {
    u64 Revision;
    EFI_BLOCK_IO_MEDIA* Media;
    EFI_STATUS (*Reset)(struct _EFI_BLOCK_IO_PROTOCOL*,bool);
    EFI_STATUS (*ReadBlocks)(struct _EFI_BLOCK_IO_PROTOCOL*,u32,EFI_LBA,u64,void*);
    EFI_STATUS (*WriteBlocks)(struct _EFI_BLOCK_IO_PROTOCOL*,u32,EFI_LBA,u64,void*);
    EFI_STATUS (*FlushBlocks)(struct _EFI_BLOCK_IO_PROTOCOL*);
} EFI_BLOCK_IO_PROTOCOL;

/*gfx output protocol*/
typedef struct {
    u32 RedMask, GreenMask, BlueMask, ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMask
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    u32 Version;
    u32 HorizontalResolution;
    u32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    u32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    u32 MaxMode;
    u32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    u64 SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    u64 FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
} _EFI_GRAPHICS_OUTPUT_PROTOCOL;

/*simple text out*/
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (*OutputString)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*,CHAR16*);
    void* TestString;
    void* QueryMode;
    void* SetMode;
    void* SetAttribute;
    void* ClearScreen;
    void* SetCursorPosition;
    void* EnableCursor;
    void* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/*simple text in*/
typedef struct {
    u16 ScanCode;
    u16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (*ReadKeyStroke)(struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL*,EFI_INPUT_KEY*);
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

/*cfg table*/
typedef struct {
    EFI_GUID VendorGuid;
    void* VendorTable;
} EFI_CONFIGURATION_TABLE;

/*boot services*/
typedef struct {
    EFI_TABLE_HEADER Hdr;

    /*task priority*/
    void* RaiseTPL;
    void* RestoreTPL;

    /*mem*/
    EFI_STATUS (*AllocatePages)(EFI_ALLOCATE_TYPE,u32,u64,EFI_PHYSICAL_ADDRESS*);
    EFI_STATUS (*FreePages)(EFI_PHYSICAL_ADDRESS,u64);
    EFI_STATUS (*GetMemoryMap)(u64*,EFI_MEMORY_DESCRIPTOR*,u64*,u64*,u32*);
    EFI_STATUS (*AllocatePool)(u32,u64,void**);
    EFI_STATUS (*FreePool)(void*);

    /*events*/
    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;

    /*protocol handlers*/
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    EFI_STATUS (*HandleProtocol)(EFI_HANDLE,EFI_GUID*,void**);
    void* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;

    /*image*/
    EFI_STATUS (*LoadImage)(bool,EFI_HANDLE,EFI_DEVICE_PATH_PROTOCOL*,void*,u64,EFI_HANDLE*);
    EFI_STATUS (*StartImage)(EFI_HANDLE,u64*,CHAR16**);
    void* Exit;
    void* UnloadImage;
    EFI_STATUS (*ExitBootServices)(EFI_HANDLE,u64);

    /*misc*/
    void* GetNextMonotonicCount;
    void* Stall;
    void* SetWatchdogTimer;

    /*drivers*/
    void* ConnectController;
    void* DisconnectController;

    /*proto open/close*/
    EFI_STATUS (*OpenProtocol)(EFI_HANDLE,EFI_GUID*,void**,EFI_HANDLE,EFI_HANDLE,u32);
    void* CloseProtocol;
    void* OpenProtocolInformation;

    /*library*/
    void* ProtocolsPerHandle;
    void* LocateHandleBuffer;
    EFI_STATUS (*LocateProtocol)(EFI_GUID*,void*,void**);
    void* InstallMultipleProtocolInterfaces;
    void* UninstallMultipleProtocolInterfaces;

    /*crc*/
    void* CalculateCrc32;

    /*misc*/
    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
} EFI_BOOT_SERVICES;

/*runtime services*/
typedef struct {
    EFI_TABLE_HEADER Hdr;

    /*time*/
    void* GetTime;
    void* SetTime;
    void* GetWakeupTime;
    void* SetWakeupTime;

    /*virt mem*/
    void* SetVirtualAddressMap;
    void* ConvertPointer;

    /*vars*/
    EFI_STATUS (*GetVariable)(CHAR16*,EFI_GUID*,u32*,u64*,void*);
    void* GetNextVariableName;
    EFI_STATUS (*SetVariable)(CHAR16*,EFI_GUID*,u32,u64,void*);

    /*misc*/
    void* GetNextHighMonotonicCount;
    void* ResetSystem;
    void* UpdateCapsule;
    void* QueryCapsuleCapabilities;
    void* QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

/*sys table*/
#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2 << 16) | 70)

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    u32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE StdErrHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    EFI_RUNTIME_SERVICES* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
    u64 NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE* ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef EFI_STATUS (*EFI_IMAGE_ENTRY_POINT)(EFI_HANDLE,EFI_SYSTEM_TABLE*);