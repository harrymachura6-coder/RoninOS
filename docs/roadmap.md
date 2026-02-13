# RōninOS Roadmap

## Done

- **Milestone 1 (teilweise):** Build/CI Fundament vorhanden.
  - Docker Build-Umgebung (`buildenv/Dockerfile`)
  - Build Script (`scripts/build.sh`)
  - GitHub Actions Build + Artefakte (`.github/workflows/ci.yml`)
  - Optionaler manueller QEMU Smoke-Job als Startpunkt

## Next

- [x] VFS-Layer vereinheitlicht (Path-Resolver + cwd + stat/listdir/open/read/write).
- [x] RAMFS als Root-FS (`/`) per VFS gemountet.
- [x] Initrd/Multiboot2-Module als read-only Mount unter `/initrd`.
- [x] Shell-Kommandos `ls/cat/pwd/cd` auf VFS-API umgestellt.

## Later

- ELF32 Loader + Ring3 + `int 0x80` Syscalls.
- Preemptiver Scheduler mit Blocking/Sleep.
- TTY-/Device-Layer und erweiterte Treiberbasis.
- Smoke-Suite mit eindeutigem `SMOKE: PASS` für automatisierten CI-Check.
