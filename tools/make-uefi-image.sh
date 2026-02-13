#!/usr/bin/env bash
set -euo pipefail

IMG="build/uefi/roninos_uefi.img"
EFI_BIN="build/uefi/BOOTX64.EFI"
SIZE_MIB="64"

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

if [[ ! -f "$EFI_BIN" ]]; then
    echo "ERROR: $EFI_BIN nicht gefunden. Bitte zuerst 'make uefi' ausführen." >&2
    exit 1
fi

mkdir -p build/uefi

rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=1M count="$SIZE_MIB" status=none
mkfs.vfat -F 32 "$IMG" >/dev/null

mmd -i "$IMG" ::/EFI
mmd -i "$IMG" ::/EFI/BOOT
mcopy -i "$IMG" "$EFI_BIN" ::/EFI/BOOT/BOOTX64.EFI

echo "UEFI FAT32-Image erstellt: $IMG"
