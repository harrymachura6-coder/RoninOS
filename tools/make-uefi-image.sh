#!/usr/bin/env bash
set -euo pipefail

IMG="build/uefi/roninos_uefi.img"
EFI_BIN="build/uefi/BOOTX64.EFI"
KERNEL_ELF="build/roninos.elf"
SIZE_MIB="64"
TMP_DIR="build/uefi/tmp"
GRUB_CFG="$TMP_DIR/grub.cfg"

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: benötigtes Tool fehlt: $1" >&2
        exit 1
    fi
}

need_cmd dd
need_cmd mkfs.vfat
need_cmd mmd
need_cmd mcopy
need_cmd grub-mkstandalone

if [[ ! -f "$KERNEL_ELF" ]]; then
    echo "ERROR: $KERNEL_ELF nicht gefunden. Bitte zuerst 'make' ausführen." >&2
    exit 1
fi

mkdir -p build/uefi "$TMP_DIR"

cat > "$GRUB_CFG" <<'CFG'
set timeout=0
set default=0

menuentry "RoninOS" {
    search --no-floppy --file --set=root /boot/roninos.elf
    multiboot2 ($root)/boot/roninos.elf
    boot
}
CFG

grub-mkstandalone \
    -O x86_64-efi \
    -o "$EFI_BIN" \
    "boot/grub/grub.cfg=$GRUB_CFG"

rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=1M count="$SIZE_MIB" status=none
mkfs.vfat -F 32 "$IMG" >/dev/null

mmd -i "$IMG" ::/EFI
mmd -i "$IMG" ::/EFI/BOOT
mmd -i "$IMG" ::/boot
mcopy -i "$IMG" "$EFI_BIN" ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$IMG" "$KERNEL_ELF" ::/boot/roninos.elf

echo "UEFI FAT32-Image erstellt: $IMG"
echo "Enthält GRUB EFI-Bootloader + RoninOS Kernel (Multiboot2)."
