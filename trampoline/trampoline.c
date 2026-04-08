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
#define MAX_HANDLES 16
#define MAX_PROTOCOLS 8

typedef struct {
    EFI_GUID guid;
    void *interface;
} protocol_entry_t;

typedef struct {
    bool used;
    protocol_entry_t protocols[MAX_PROTOCOLS];
    int num_protocols;
} handle_entry_t;

static handle_entry_t handles[MAX_HANDLES];

static EFI_HANDLE alloc_handle(void)
{
    for (int i=0;i<MAX_HANDLES;i++) {
        if (!handles[i].used) {
            handles[i].used = true;
            handles[i].num_protocols = 0;
            return (EFI_HANDLE)(uintptr_t)(i+1);
        }
    }
    return NULL;
}

static handle_entry_t *get_handle(EFI_HANDLE h)
{
    int idx=(int)(uintptr_t)h-1;
    if (idx<0||idx>=MAX_HANDLES||!handles[idx].used) return NULL;
    return &handles[idx];
}

static EFI_STATUS install_protocol(EFI_HANDLE h, const EFI_GUID *guid, void *iface)
{
    handle_entry_t *he=get_handle(h);
    if (!he) return EFI_INVALID_PARAMETER;
    if (he->num_protocols>=MAX_PROTOCOLS) return EFI_OUT_OF_RESOURCES;

    he->protocols[he->num_protocols].guid=*guid;
    he->protocols[he->num_protocols].interface=iface;
    he->num_protocols++;
    return EFI_SUCCESS;
}

static EFI_STATUS lookup_protocol(EFI_HANDLE h, const EFI_GUID *guid, void **iface)
{
    handle_entry_t *he=get_handle(h);
    if (!he) return EFI_NOT_FOUND;

    for (int i=0;i<he->num_protocols;i++) {
        if (guid_eq(&he->protocols[i].guid,guid)) {
            if (iface) *iface=he->protocols[i].interface;
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}

/*
 * boot services stubs
 * all functions use the ms x64 ABI (__attribute__((ms_abi)))
 */

static EFI_STATUS __attribute__((ms_abi))
stub_AllocatePages(EFI_ALLOCATE_TYPE type, u32 mem_type, u64 pages, EFI_PHYSICAL_ADDRESS *addr)
{
    size_t size=pages*4096;
    void *p = mem_alloc(size);
    if (!p) return EFI_OUT_OF_RESOURCES;

    *addr = (EFI_PHYSICAL_ADDRESS)(uintptr_t)p;
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_FreePages(EFI_PHYSICAL_ADDRESS addr, u64 pages)
{
    (void)addr; (void)pages;
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_GetMemoryMap(u64 *map_size, EFI_MEMORY_DESCRIPTOR *map, u64 *map_key, u64 *desc_size, u32 *desc_version)
{
    u64 needed = efi_memmap_count*sizeof(EFI_MEMORY_DESCRIPTOR);
    *desc_size=sizeof(EFI_MEMORY_DESCRIPTOR);
    *desc_version=EFI_MEMORY_DESCRIPTOR_VERSION;
    *map_key=efi_memmap_key;

    if (*map_size<needed) {
        *map_size=needed;
        return EFI_BUFFER_TOO_SMALL;
    }

    memcpy(map,efi_memmap,needed);
    *map_size=needed;
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_AllocatePool(u32 pool_type, u64 size, void **buf)
{
    (void)pool_type;
    void *p=mem_alloc(size);
    if (!p) return EFI_OUT_OF_RESOURCES;
    *buf = p;
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_FreePool(void *buf)
{
    mem_free(buf);
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_HandleProtocol(EFI_HANDLE handle, EFI_GUID *guid, void **iface)
{
    return lookup_protocol(handle,guid,iface);
}

static EFI_STATUS __attribute__((ms_abi))
stub_OpenProtocol(EFI_HANDLE handle, EFI_GUID *guid, void **iface, EFI_HANDLE agent, EFI_HANDLE controller, u32 attributes)
{
    (void)agent; (void)controller; (void)attributes;
    return lookup_protocol(handle,guid,iface);
}

static EFI_STATUS __attribute__((ms_abi))
stub_LocateProtocol(EFI_GUID *guid, void *registration, void **iface)
{
    (void)registration;
    for (int i=0;i<MAX_HANDLES;i++) {
        if (!handles[i].used) continue;
        for (int j=0;j<handles[i].num_protocols;j++) {
            if (guid_eq(&handles[i].protocols[j].guid,guid)) {
                if (iface) *iface=handles[i].protocols[j].interface;
                return EFI_SUCCESS;
            }
        }
    }
    return EFI_NOT_FOUND;
}

/*
 * stub_LoadImage
 * bootmgfw calls this to load winload.efi
 */

static EFI_STATUS __attribute__((ms_abi))
stub_LoadImage(bool boot_policy,EFI_HANDLE parent, EFI_DEVICE_PATH_PROTOCOL *path, void *src, u64 src_size, EFI_HANDLE *image_handle)
{
    (void)boot_policy; (void)parent; (void)path;

    const void *wl_data = (const void *)&_winload_data;
    u64 wl_size=_winload_size;

    u32 image_bytes=pe_image_size(wl_data,wl_size);
    if (image_bytes==0) return EFI_INVALID_PARAMETER;

    void *load_at = (void *)&_winload_load_area;
    void *entry = pe_load(wl_data, wl_size, load_at);
    if (!entry) return EFI_INVALID_PARAMETER;

    EFI_LOADED_IMAGE_PROTOCOL *li = mem_alloc(sizeof(*li));
    if (!li) return EFI_OUT_OF_RESOURCES;

    static const EFI_GUID loaded_image_guid=EFI_LOADED_IMAGE_PROTOCOL_GUID;

    li->Revision = 0x1000;
    li->ParentHandle = parent;
    li->ImageBase = load_at;
    li->ImageSize = image_bytes;
    li->ImageCodeType = EFI_LOADER_CODE;
    li->ImageDataType = EFI_LOADER_DATA;

    EFI_HANDLE h = alloc_handle();
    if (!h) return EFI_OUT_OF_RESOURCES;

    install_protocol(h,&loaded_image_guid,li);

    li->Reserved=entry;

    *image_handle=h;
    return EFI_SUCCESS;
}

static EFI_STATUS __attribute__((ms_abi))
stub_StartImage(EFI_HANDLE image_handle, u64 *exit_data_size, CHAR16 **exit_data)
{
    (void)exit_data_size; (void)exit_data;
    static const EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_LOADED_IMAGE_PROTOCOL *li=NULL;
    EFI_STATUS st = lookup_protocol(image_handle, &loaded_image_guid, (void**)&li);
    if (st != EFI_SUCCESS || !li) return EFI_INVALID_PARAMETER;

    void *entry = li->Reserved;
    if (!entry) return EFI_INVALID_PARAMETER;

    extern EFI_SYSTEM_TABLE g_system_table;
    
    EFI_IMAGE_ENTRY_POINT ep = (EFI_IMAGE_ENTRY_POINT)entry;
    return ep(image_handle, &g_system_table);
}

static EFI_STATUS __attribute__((ms_abi))
stub_ExitBootServices(EFI_HANDLE image_handle, u64 map_key)
{
    (void)image_handle;
    /* we accept any key because we dont actually have live boot services */
    (void)map_key;
    return EFI_SUCCESS;
}

/*runtime services stubs*/
static EFI_STATUS __attribute__((ms_abi))
stub_GetVariable(CHAR16 *name, EFI_GUID *guid, u32 *attrs, u64 *data_size, void *data)
{
    (void)name; (void)guid; (void)attrs; (void)data; (void)data_size;
    return EFI_NOT_FOUND;
}

static EFI_STATUS __attribute__((ms_abi))
stub_SetVariable(CHAR16 *name, EFI_GUID *guid, u32 attrs, u64 data_size, void *data)
{
    (void)name;(void)guid;(void)attrs;(void)data_size;(void)data;
    return EFI_SUCCESS;
}

/*console output stub*/
static EFI_STATUS __attribute__((ms_abi))
stub_OutputString(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *this, CHAR16 *str)
{
    (void)this;
    (void)str;
    return EFI_SUCCESS;
}

/*global tables*/
EFI_SYSTEM_TABLE g_system_table;
EFI_BOOT_SERVICES g_boot_services;
EFI_RUNTIME_SERVICES g_runtime_services;

static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL g_conout;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL g_conin;

/* fake UEFI environment */
static void build_boot_services(void)
{
    EFI_BOOT_SERVICES *bs = &g_boot_services;
    memset(bs,0,sizeof(*bs));

    bs->Hdr.Signature=0x56524553544f4f42ULL; /*BOOTSERV*/
    bs->Hdr.Revision=EFI_2_70_SYSTEM_TABLE_REVISION;
    bs->Hdr.HeaderSize=sizeof(EFI_BOOT_SERVICES);

    bs->AllocatePages = stub_AllocatePages;
    bs->FreePages = stub_FreePages;
    bs->GetMemoryMap = stub_GetMemoryMap;
    bs->AllocatePool = stub_AllocatePool;
    bs->FreePool = stub_FreePool;
    bs->HandleProtocol = stub_HandleProtocol;
    bs->OpenProtocol = stub_OpenProtocol;
    bs->LocateProtocol = stub_LocateProtocol;
    bs->LoadImage = stub_LoadImage;
    bs->StartImage = stub_StartImage;
    bs->ExitBootServices = stub_ExitBootServices;

    /*stubs for things we dont implement but cant be NULL*/
    bs->RaiseTPL = (void*)stub_FreePool;
    bs->RestoreTPL = (void*)stub_FreePool;
    bs->CreateEvent = (void*)stub_FreePool;
    bs->SetTimer = (void*)stub_FreePool;
    bs->WaitForEvent = (void*)stub_FreePool;
    bs->SignalEvent = (void*)stub_FreePool;
    bs->CloseEvent = (void*)stub_FreePool;
    bs->CheckEvent = (void*)stub_FreePool;
    bs->InstallProtocolInterface = (void*)stub_FreePool;
    bs->ReinstallProtocolInterface = (void*)stub_FreePool;
    bs->RegisterProtocolNotify = (void*)stub_FreePool;
    bs->LocateHandle = (void*)stub_FreePool;
    bs->LocateDevicePath = (void*)stub_FreePool;
    bs->InstallConfigurationTable = (void*)stub_FreePool;
    bs->Exit = (void*)stub_FreePool;
    bs->UnloadImage = (void*)stub_FreePool;
    bs->GetNextMonotonicCount = (void*)stub_FreePool;
    bs->Stall = (void*)stub_FreePool;
    bs->SetWatchdogTimer = (void*)stub_FreePool;
    bs->ConnectController = (void*)stub_FreePool;
    bs->DisconnectController = (void*)stub_FreePool;
    bs->CloseProtocol = (void*)stub_FreePool;
    bs->OpenProtocolInformation = (void*)stub_FreePool;
    bs->ProtocolsPerHandle = (void*)stub_FreePool;
    bs->LocateHandleBuffer = (void*)stub_FreePool;
    bs->InstallMultipleProtocolInterfaces = (void*)stub_FreePool;
    bs->UninstallMultipleProtocolInterfaces = (void*)stub_FreePool;
    bs->CalculateCrc32 = (void*)stub_FreePool;
    bs->CopyMem = (void*)stub_FreePool;
    bs->SetMem = (void*)stub_FreePool;
    bs->CreateEventEx = (void*)stub_FreePool;
}

static void build_runtime_services(void)
{
    EFI_RUNTIME_SERVICES *rs = &g_runtime_services;
    memset(rs,0,sizeof(*rs));

    rs->Hdr.Signature = 0x56524553544e5552ULL; /*RUNTSERV*/
    rs->Hdr.Revision = EFI_2_70_SYSTEM_TABLE_REVISION;
    rs->Hdr.HeaderSize = sizeof(EFI_RUNTIME_SERVICES);

    rs->GetVariable=stub_GetVariable;
    rs->SetVariable=stub_SetVariable;

    rs->GetTime = (void*)stub_FreePool;
    rs->SetTime = (void*)stub_FreePool;
    rs->GetWakeupTime = (void*)stub_FreePool;
    rs->SetWakeupTime = (void*)stub_FreePool;
    rs->SetVirtualAddressMap = (void*)stub_FreePool;
    rs->ConvertPointer = (void*)stub_FreePool;
    rs->GetNextVariableName = (void*)stub_FreePool;
    rs->GetNextHighMonotonicCount = (void*)stub_FreePool;
    rs->ResetSystem = (void*)stub_FreePool;
    rs->UpdateCapsule = (void*)stub_FreePool;
    rs->QueryCapsuleCapabilities = (void*)stub_FreePool;
    rs->QueryVariableInfo = (void*)stub_FreePool;
}

static void build_system_table(void)
{
    g_conout.OutputString = stub_OutputString;

    EFI_SYSTEM_TABLE *st = &g_system_table;
    memset(st,0,sizeof(*st));

    st->Hdr.Signature = EFI_SYSTEM_TABLE_SIGNATURE;
    st->Hdr.Revision = EFI_2_70_SYSTEM_TABLE_REVISION;
    st->Hdr.HeaderSize=sizeof(EFI_SYSTEM_TABLE);

    static CHAR16 vendor[] = {
        's','h','i','m','d','o','w','s',0
    };
    st->FirmwareVendor=vendor;
    st->FirmwareRevision=1;

    st->ConsoleOutHandle=alloc_handle();
    st->ConOut = &g_conout;
    st->ConsoleInHandle = alloc_handle();
    st->ConIn = &g_conin;
    st->StdErrHandle=st->ConsoleOutHandle;
    st->StdErr=&g_conout;

    st->RuntimeServices=&g_runtime_services;
    st->BootServices=&g_boot_services;

    st->NumberOfTableEntries = 0;
    st->ConfigurationTable = NULL;
}