#!/usr/bin/env bash
set -euo pipefail

IMG="build/uefi/roninos_uefi.img"

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: benötigtes Tool fehlt: $1" >&2
        exit 1
    fi
}

need_cmd qemu-system-x86_64

if [[ ! -f "$IMG" ]]; then
    echo "ERROR: $IMG fehlt. Bitte zuerst 'make uefi-image' ausführen." >&2
    exit 1
fi

find_ovmf_code() {
    local candidates=(
        "/usr/share/OVMF/OVMF_CODE.fd"
        "/usr/share/edk2-ovmf/OVMF_CODE.fd"
        "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd"
        "/usr/share/OVMF/OVMF_CODE_4M.fd"
        "/usr/share/edk2-ovmf/x64/OVMF_CODE_4M.fd"
    )

    local path
    for path in "${candidates[@]}"; do
        if [[ -f "$path" ]]; then
            echo "$path"
            return 0
        fi
    done

    find /usr/share -name 'OVMF_CODE*.fd' 2>/dev/null | head -n1
}

OVMF_CODE="$(find_ovmf_code)"

if [[ -z "$OVMF_CODE" || ! -f "$OVMF_CODE" ]]; then
    echo "ERROR: OVMF_CODE nicht gefunden. Installiere ggf. Paket 'ovmf'." >&2
    exit 1
fi

QEMU_ARGS=(
    -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE"
    -drive "format=raw,file=$IMG"
)

if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo "Hinweis: kein grafisches Display erkannt, starte QEMU im Headless-Modus (-nographic)."
    QEMU_ARGS+=( -nographic )
fi

echo "Starte QEMU mit OVMF: $OVMF_CODE"
exec qemu-system-x86_64 "${QEMU_ARGS[@]}"
