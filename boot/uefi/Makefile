UEFI_BUILD_DIR := build/uefi
UEFI_APP_SRC := boot/uefi/main.c
UEFI_OBJ := $(UEFI_BUILD_DIR)/main.o
UEFI_SO := $(UEFI_BUILD_DIR)/main.so
UEFI_EFI := $(UEFI_BUILD_DIR)/BOOTX64.EFI

UEFI_CFLAGS := -I/usr/include/efi -I/usr/include/efi/x86_64 -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -O2
UEFI_LDFLAGS := -nostdlib -znocombreloc -shared -Bsymbolic

UEFI_LDS := $(shell \
	for p in \
		/usr/lib/elf_x86_64_efi.lds \
		/usr/lib64/elf_x86_64_efi.lds \
		/usr/lib/x86_64-linux-gnu/elf_x86_64_efi.lds \
		/usr/lib/gnuefi/elf_x86_64_efi.lds; do \
		[ -f "$$p" ] && { echo "$$p"; exit 0; }; \
	done; \
	if command -v dpkg >/dev/null 2>&1; then \
		dpkg -L gnu-efi 2>/dev/null | awk '/elf_x86_64_efi\.lds$$/ { print; exit }'; \
	else \
		find /usr -name elf_x86_64_efi.lds 2>/dev/null | head -n1; \
	fi)

UEFI_LIBDIR := $(shell \
	for p in /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/gnuefi; do \
		if [ -f "$$p/libefi.a" ] && [ -f "$$p/libgnuefi.a" ]; then \
			echo "$$p"; \
			exit 0; \
		fi; \
	done)

UEFI_CRT := $(firstword \
	$(wildcard $(UEFI_LIBDIR)/crt0-efi-x86_64.o) \
	$(wildcard /usr/lib/crt0-efi-x86_64.o) \
	$(wildcard /usr/lib64/crt0-efi-x86_64.o) \
	$(wildcard /usr/lib/x86_64-linux-gnu/crt0-efi-x86_64.o) \
	$(wildcard /usr/lib/gnuefi/crt0-efi-x86_64.o))

.PHONY: uefi
uefi: $(UEFI_EFI)

$(UEFI_BUILD_DIR):
	mkdir -p $@

$(UEFI_OBJ): $(UEFI_APP_SRC) | $(UEFI_BUILD_DIR)
	@if [ ! -f /usr/include/efi/efi.h ]; then \
		echo "ERROR: gnu-efi headers fehlen. Installiere: sudo apt install gnu-efi"; \
		exit 1; \
	fi
	gcc $(UEFI_CFLAGS) -c -o $@ $<

$(UEFI_SO): $(UEFI_OBJ)
	@if [ -z "$(UEFI_LDS)" ] || [ ! -f "$(UEFI_LDS)" ]; then \
		echo "ERROR: elf_x86_64_efi.lds nicht gefunden (gnu-efi Linker-Script)."; \
		exit 1; \
	fi
	@if [ -z "$(UEFI_LIBDIR)" ]; then \
		echo "ERROR: gnu-efi Libdir nicht gefunden (libefi.a/libgnuefi.a)."; \
		exit 1; \
	fi
	@if [ -z "$(UEFI_CRT)" ] || [ ! -f "$(UEFI_CRT)" ]; then \
		echo "ERROR: crt0-efi-x86_64.o nicht gefunden."; \
		exit 1; \
	fi
	ld $(UEFI_LDFLAGS) -T $(UEFI_LDS) -L$(UEFI_LIBDIR) $(UEFI_CRT) $< -o $@ -lefi -lgnuefi

$(UEFI_EFI): $(UEFI_SO)
	objcopy \
		-j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-x86_64 \
		$< $@
