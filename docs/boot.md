# Boot / Multiboot2 Leitfaden

RoninOS wird von GRUB als **Multiboot2-Kernel** geladen. Damit das zuverlässig funktioniert,
müssen folgende Bedingungen erfüllt sein:

- Der Header liegt in einer dedizierten `.multiboot2`-Section.
- Der Header ist 8-Byte aligned.
- Das Linkerscript platziert `.multiboot2` als erste relevante Output-Section und schützt sie mit `KEEP(...)`.
- Der Header liegt innerhalb der ersten 32 KiB des Kernel-Files.
- `iso/boot/grub/grub.cfg` lädt genau das finale Kernel-Artefakt (`/boot/roninos.elf`).

## Build-Checks

Der Build enthält `make verify`:

- Wenn vorhanden: `grub-file --is-x86-multiboot2 build/roninos.elf`
- Fallback: `readelf`-Prüfung, dass `.multiboot2` existiert und Section-Offset `< 32768` ist.

Empfohlener Ablauf:

```bash
make clean && make
make verify
```
