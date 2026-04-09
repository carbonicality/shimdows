LOAD_BASE = 0x100000;

ENTRY(_start);

SECTIONS
{
    . = LOAD_BASE;
    
    .text.entry : {
        *(.text.entry)
    }

    .text : {
        *(.text .text.*)
    }

    . = ALIGN(0x1000);
    .data : {
        *(.data .data.*)
    }

    . = ALIGN(0x1000);
    .rodata : {
        *(.rodata .rodata.*)
    }

    . = ALIGN(0x1000);
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }

    . = ALIGN(0x1000);
    .stack : {
        *(.stack)
    }

    . = LOAD_BASE + 0x80000;
    .memmap : {
        . += 8;
        _memmap_count = . - 8;
        _memmap_entries = .;
        . += (128 * 24);
    }

    . = LOAD_BASE + 0x90000;
    .bootmgr_region : {
        . += 8;
        _bootmgr_size = . - 8;
        _bootmgr_data = .;
        . += (4 * 1024 * 1024);
    }

    . = LOAD_BASE + 0x490000;
    .winload_region : {
        . += 8;
        _winload_size = . - 8;
        _winload_data = .;
        . += (4 * 1024 * 1024);
    }

    .pool : {
        _pool_start = .;
        . += (4 * 1024 * 1024);
        _pool_end = .;
    }

    . = LOAD_BASE + 0xC90000;
    .image_load : {
        _image_load_area = .;
        . += (4 * 1024 * 1024);
    }

    . = LOAD_BASE + 0x1090000;
    .winload_load : {
        _winload_load_area = .;
        . += (4 * 1024 * 1024);
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note*)
        *(.gnu*)
    }
}