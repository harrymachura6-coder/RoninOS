CROSS   ?= x86_64-elf-
CC      := $(CROSS)gcc
AS      := $(CROSS)as
LD      := $(CROSS)ld

CFLAGS := -std=c11 -ffreestanding -O2 -Wall -Wextra -fno-pic -fno-pie -m32 -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -mno-sse -mno-sse2 -mno-mmx -mno-80387
LDFLAGS := -T kernel/linker.ld -nostdlib -m elf_i386
ASFLAGS := --32

FB_FONT ?= 16x32
DEBUG ?= 0

ifeq ($(FB_FONT),8x16)
  CFLAGS += -DFB_FONT=816
else
  CFLAGS += -DFB_FONT=1632
endif

ifeq ($(DEBUG),1)
  CFLAGS += -DFB_CONSOLE_DEBUG=1
endif

KERNEL_ELF := build/roninos.elf
ISO_KERNEL := iso/boot/roninos.elf
ISO_IMG    := build/roninos.iso
ISO_LABEL  := RONINOS

OBJ := \
build/boot.o \
build/idt_load.o \
build/exc_stubs.o \
build/idt.o \
build/pic.o \
build/pit.o \
build/isr.o \
build/isr_stubs.o \
build/console.o \
build/fb_console.o \
build/serial.o \
build/keyboard_input.o \
build/string.o \
build/heap.o \
build/panic.o \
build/pmm.o \
build/paging.o \
build/apps.o \
build/app_echo.o \
build/app_clear.o \
build/app_about.o \
build/app_meminfo.o \
build/app_heap_test.o \
build/app_sched.o \
build/app_fs.o \
build/app_fat32.o \
build/app_ls.o \
build/app_cat.o \
build/app_pwd.o \
build/app_cd.o \
build/app_disk.o \
build/ramfs.o \
build/fat32.o \
build/blockdev.o \
build/ata_pio.o \
build/vfs.o \
build/initrd.o \
build/thread.o \
build/switch.o \
build/shell_core.o \
build/shell_commands.o \
build/terminal.o \
build/exceptions.o \
build/kernel.o

all: $(ISO_IMG) verify

include boot/uefi/Makefile

build:
	mkdir -p build

build/boot.o: kernel/boot.s | build
	$(AS) $(ASFLAGS) -o $@ $<

build/idt_load.o: kernel/idt_load.s | build
	$(AS) $(ASFLAGS) -o $@ $<

build/exc_stubs.o: kernel/exc_stubs.s | build
	$(AS) $(ASFLAGS) -o $@ $<

build/isr_stubs.o: kernel/isr_stubs.s | build
	$(AS) $(ASFLAGS) -o $@ $<

build/idt.o: kernel/idt.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/pic.o: kernel/pic.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/isr.o: kernel/isr.c | build
	$(CC) $(CFLAGS) -c -o $@ $<


build/pit.o: kernel/pit.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/console.o: kernel/console.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/fb_console.o: kernel/fb_console.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/serial.o: kernel/serial.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/keyboard_input.o: kernel/input/keyboard.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/string.o: kernel/lib/string.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/heap.o: kernel/heap.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/panic.o: kernel/panic.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/pmm.o: kernel/mem/pmm.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/paging.o: kernel/mem/paging.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/apps.o: kernel/apps/apps.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_echo.o: kernel/apps/app_echo.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_clear.o: kernel/apps/app_clear.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_about.o: kernel/apps/app_about.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_meminfo.o: kernel/apps/app_meminfo.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_heap_test.o: kernel/apps/app_heap_test.c | build
	$(CC) $(CFLAGS) -c -o $@ $<


build/app_sched.o: kernel/apps/app_sched.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_fs.o: kernel/apps/app_fs.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_fat32.o: kernel/apps/app_fat32.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/ramfs.o: kernel/fs/ramfs.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/fat32.o: kernel/fs/fat32.c | build
	$(CC) $(CFLAGS) -c -o $@ $<


build/blockdev.o: kernel/block/blockdev.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/ata_pio.o: kernel/drivers/ata_pio.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_ls.o: kernel/apps/app_ls.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_cat.o: kernel/apps/app_cat.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_pwd.o: kernel/apps/app_pwd.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/app_cd.o: kernel/apps/app_cd.c | build
	$(CC) $(CFLAGS) -c -o $@ $<


build/app_disk.o: kernel/apps/app_disk.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/vfs.o: kernel/fs/vfs.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/initrd.o: kernel/fs/initrd.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/thread.o: kernel/sched/thread.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/switch.o: kernel/sched/switch.S | build
	$(AS) $(ASFLAGS) -o $@ $<

build/shell_core.o: kernel/shell/shell.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/shell_commands.o: kernel/shell/commands.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/terminal.o: kernel/terminal/terminal.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/exceptions.o: kernel/exceptions.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/kernel.o: kernel/kernel.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERNEL_ELF): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

verify: $(KERNEL_ELF)
	@if command -v grub-file >/dev/null 2>&1; then \
		grub-file --is-x86-multiboot2 $(KERNEL_ELF) || { \
			echo "ERROR: $(KERNEL_ELF) is not a valid Multiboot2 kernel"; \
			exit 1; \
		}; \
	else \
		offset_hex=$$(readelf -SW $(KERNEL_ELF) | awk '$$3 == ".multiboot2" { print $$6; exit }'); \
		if [ -z "$$offset_hex" ]; then \
			echo "ERROR: .multiboot2 section missing in $(KERNEL_ELF)"; \
			exit 1; \
		fi; \
		offset_dec=$$((16#$$offset_hex)); \
		if [ $$offset_dec -ge 32768 ]; then \
			echo "ERROR: .multiboot2 section offset $$offset_dec is outside first 32KiB"; \
			exit 1; \
		fi; \
		echo "Verified Multiboot2 header at file offset $$offset_dec"; \
	fi

$(ISO_KERNEL): $(KERNEL_ELF)
	mkdir -p iso/boot
	cp $(KERNEL_ELF) $(ISO_KERNEL)

$(ISO_IMG): $(ISO_KERNEL) verify
	mkdir -p iso/boot
	grub-mkrescue -o $(ISO_IMG) iso -- -volid $(ISO_LABEL) >/dev/null 2>&1

run: $(ISO_IMG)
	qemu-system-i386 -cdrom $(ISO_IMG) -m 256M

run-serial: $(ISO_IMG)
	qemu-system-i386 -cdrom $(ISO_IMG) -m 256M -serial stdio

uefi-image: $(KERNEL_ELF) verify
	./tools/make-uefi-image.sh

uefi-run: uefi-image
	./tools/run-uefi-qemu.sh

clean:
	rm -rf build $(ISO_KERNEL)

.PHONY: all run run-serial clean verify uefi-image uefi-run
