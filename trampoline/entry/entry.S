/*
 * entry.S
 * kexec purgatory jumps in here in 64bit long mode
 * sonnnnnnnnnnnn im crine
 */

.intel_syntax noprefix
.code64
.section .text.entry, "ax"
.global _start
.global gdt64
.global gdt64_ptr
.global gdt32_ptr

.section .text.entry, "ax"
.global trampoline_header
trampoline_header: 
    .long 0x53484457 /* magic SHDW */
    .long 1
    .quad 0x100000
    .quad 0x1200000
    .quad (_start - 0x100000)
    .quad 0x17F000
    .quad 0x80000
    .quad 128
    .quad 0x90000
    .quad (4 * 1024 * 1024)
    .quad 0x490000
    .quad (4 * 1024 * 1024)

_start:
    cli

    lea rsp, [rip + _stack_top]
    xor rbp, rbp

    /* load our own GDT */
    lea rax, [rip + gdt64_ptr]
    lgdt [rax]

    lea rax, [rip + .Lafter_far_ret]
    push 0x08
    push rax
    lretq

.Lafter_far_ret:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    lea rax, [rip + trampoline_main]
    call rax

    cli
.Lhalt:
    hlt
    jmp .Lhalt

/*
 * GDT
 * 0x00 null descriptor
 * 0x08 64 bit code (ring 0)
 * 0x10 64 bit data (ring 0)
 * 0x18 32 bit code (ring 0) needed for mode switch if required
 * 0x20 32 bit data (ring 0)
 */
.section .data
.align 8

gdt64:
    /* 0x00 null */
    .quad 0x0000000000000000

    /* 0x08 */
    .quad 0x00AF9A000000FFFF

    /* 0x10 */
    .quad 0x00CF92000000FFFF

    /* 0x18 */
    .quad 0x00CF9A000000FFFF

    /* 0x20 */
    .quad 0x00CF92000000FFFF

gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

/*
 * stack 
 * 64KB
 * places in its own section so linker can place it in a known phys addr
 */
.section .stack, "aw", @nobits
.align 4096
.global _stack_bottom
_stack_bottom:
    .space 65536
.global _stack_top
_stack_top:

/*
 * jump_to_bootmgr
 * called from C once we have build fake UEFI tables and loaded bootmgfw
 * we go from sysv abi to ms abi here
 */
.section .text
.global jump_to_bootmgr
jump_to_bootmgr:
    mov r10, rdi 
    mov rcx, rsi
    and rsp, -16
    sub rsp, 32
    jmp r10 

/* 
 * fixup_gdt_base
 * called from C before lgdt
 */
.section .text
.global fixup_gdt_base
fixup_gdt_base:
    lea rax, [rip + gdt64]
    lea rcx, [rip + gdt64_ptr + 2]
    mov [rcx], rax
    ret