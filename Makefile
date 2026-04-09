CROSS ?= x86_64-elf-
CC := gcc
CROSS_CC := $(CROSS)gcc
CROSS_AS := $(CROSS)gcc
CROSS_LD := $(CROSS)ld
CROSS_OBJCOPY := $(CROSS)objcopy

LAUNCHER_CFLAGS := \
	-O2 \
	-Wall -Wextra \
	-std=c11 \
	-I include

LAUNCHER_LDFLAGS :=

TRAMP_CFLAGS := \
	-ffreestanding \
	-nostdlib \
	-nostdinc \
	-fno-stack-protector \
    -fno-pic \
    -fno-pie \
    -mno-red-zone \
    -mcmodel=kernel \
    -O2 \
    -Wall -Wextra \
    -std=c11 \
    -I include

TRAMP_ASFLAGS := \
	-ffreestanding \
    -nostdlib \
    -mno-red-zone \
    -mcmodel=kernel

TRAMP_LDFLAGS := \
	-nostdlib \
    -static \
    -T trampoline/trampoline.ld \
    --no-dynamic-linker

LAUNCHER_SRCS := launcher/launcher.c

TRAMP_SRCS := \
	trampoline/trampoline.c \
	trampoline/pe/pe.c \
	trampoline/mem/mem.c

TRAMP_ASM := \
	trampoline/entry/entry.S

.PHONY: all clean launcher trampoline

all: launcher trampoline

launcher: ${LAUNCHER_SRCS}
	$(CC) $(LAUNCHER_CFLAGS) -o launcher.elf $^ $(LAUNCHER_LDFLAGS)

TRAMP_OBJS := $(TRAMP_SRCS:.c=.o) $(TRAMP_ASM:.S=.o)

%.o: %.c
	@echo "  CC    $<"
	$(CROSS_CC) $(TRAMP_CFLAGS) -c -o $@ $<

%.o: %.S
	@echo "  AS    $<"
	$(CROSS_CC) $(TRAMP_CFLAGS) -c -o $@ $<

trampoline/trampoline.elf: $(TRAMP_OBJS)
	@echo "  LD    trampoline.elf"
	$(CROSS_LD) $(TRAMP_LDFLAGS) -o $@ $^

trampoline.bin: trampoline/trampoline.elf
	@echo "  BIN   trampoline.bin"
	$(CROSS_OBJCOPY) -O binary $< $@
	@echo "  SIZE  $$(wc -c < $@) bytes"

trampoline: trampoline.bin

clean:
	rm -f launcher
	rm -f trampoline.bin trampoline/trampoline.elf
	find . -name '*.o' -delete

help:
	@echo "Targets:"
	@echo "all	builds both the launcher and trampoline.bin"
	@echo "launcher builds the launcher executable"
	@echo "trampoline builds the trampoline binary"
	@echo "clean removes all built files"
	@echo "help displays this help message"
	@echo "Usage on shim: ./launcher trampoline.bin /path/to/bootmgfw.efi /path/to/winload.efi"
