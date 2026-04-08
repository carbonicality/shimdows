/*
 * entry.S
 * kexec purgatory jumps in here in 64bit long mode
 * sonnnnnnnnnnnn im crine
 */

.code64
.section .text.entry, "ax"
.global _start
.global gdt64
.global gdt64_ptr
.global gdt32_ptr

