# RōninOS ⚔️(RoninOS)

RōninOS ist ein kompakter, freestanding Hobby-Kernel in C mit Fokus auf **Nachvollziehbarkeit**, **einfacher Erweiterbarkeit** und **hands-on OS-Lernen**. Das Projekt enthält eine eigene Shell, mehrere Kernel-Apps, einen RAM-basierten Dateispeicher, eine einfache FAT32-Implementierung auf dem generischen Blockdevice-Layer sowie grundlegende Scheduler-/Thread-Mechanik.

> Ziel dieses Repositories: Eine schlanke, verständliche Codebasis, mit der man Kernel-Basics praktisch erkunden kann, ohne von komplexer Toolchain- oder Framework-Logik erschlagen zu werden.

---

## Inhaltsverzeichnis

1. [Projektziele](#projektziele)
2. [Feature-Überblick](#feature-überblick)
3. [Repository-Struktur](#repository-struktur)
4. [Build & Run](#build--run)
5. [Shell-Kommandos](#shell-kommandos)
6. [Dateisysteme](#dateisysteme)
   - [RAMFS (`fs`)](#ramfs-fs)
   - [FAT32 (`fat32`)](#fat32-fat32)
7. [FAT32-Workflow (Create/Edit/Read/Delete)](#fat32-workflow-createeditreaddelete)
8. [Technische Architektur](#technische-architektur)
9. [Entwicklungsleitlinien](#entwicklungsleitlinien)
10. [Troubleshooting](#troubleshooting)
11. [UEFI build & run](#uefi-build--run)
12. [Roadmap-Ideen](#roadmap-ideen)
13. [Beitrag leisten](#beitrag-leisten)

---

## Projektziele

- **Didaktischer Kernel:** Kernkonzepte (Boot, Speicher, Scheduling, I/O, Dateisysteme) kompakt vermitteln.
- **Freestanding Ansatz:** Keine Abhängigkeit von libc im Kernelpfad (außer eigenen Komponenten in `kernel/lib/`).
- **Kleine, überprüfbare Änderungen:** Features inkrementell hinzufügen und einfach testen.
- **Praktische Shell:** Funktionen direkt aus der Kernel-Shell erkunden.

---

## Feature-Überblick

- Textkonsole inkl. Shell mit Kommando-Parser.
- Basis-Apps (`echo`, `clear`, `about`, `meminfo`, Scheduler-Tools etc.).
- Heap-Allocator mit Selbsttest.
- Einfaches RAM-Dateisystem (`fs`).
- FAT32 auf auswaehlbarem Blockdevice mit Format/Mount/List/Write/Read/Delete.
- Block-Device Discovery (ATA PIO) inkl. `disk` Kommando fuer echte/virtuelle HDDs.
- Preemption-Schalter und Thread-Introspektion (`ps`, `spawn`, `yield`, `preempt`).

---

## Repository-Struktur

```text
RoninOS/
├── kernel/
│   ├── apps/           # Shell-Apps (z. B. fs, fat32, scheduler tools)
│   ├── fs/             # Dateisysteme (ramfs, fat32)
│   ├── lib/            # Eigene freestanding Hilfsbibliothek
│   ├── mem/            # Paging/PMM etc.
│   ├── sched/          # Thread/Scheduler-Logik
│   ├── shell.c         # Shell-Parser + built-in help
│   └── ...
├── Makefile
└── README.md
```

---

## Build & Run

### Voraussetzungen

- `make`
- GCC-Cross-Toolchain (abhängig vom Setup des Projekts)
- Emulator/Runner laut Makefile (typischerweise QEMU)

### Build lokal

```bash
make clean && make
```

### Multiboot2-Header verifizieren

```bash
make verify
```

Details zu Boot-Constraints und Checks: `docs/boot.md`.

### Build via Docker (reproduzierbar)

```bash
scripts/build.sh
```

Das Script baut zuerst das Build-Image aus `buildenv/Dockerfile` und führt danach den Projekt-Build im Container aus.

### Starten

```bash
make run
```

```bash
# Beispiel mit zusaetzlicher 1GiB Raw-HDD fuer Blockdevice-Tests
qemu-img create -f raw build/disk.img 1G
qemu-system-i386 -cdrom build/roninos.iso -m 256M \
  -drive file=build/disk.img,format=raw,if=ide,index=0,media=disk
```

Smoke-Check in der Shell:

```bash
disk
disk info hd0
disk read hd0 0 1
```

> Empfehlung: Nach jeder funktionalen Änderung mindestens einmal komplett mit `make clean && make` neu bauen.

---

## UEFI build, ISO erstellen, testen, USB flashen

### Dependencies (Ubuntu/Debian)

```bash
sudo apt install gnu-efi mtools dosfstools ovmf qemu-system-x86
```

### UEFI-Binary bauen

```bash
make uefi
```

Erzeugt:

- `build/uefi/BOOTX64.EFI`

### UEFI-FAT-Image bauen (für `make uefi-run`)

```bash
make uefi-image
```

Erzeugt:

- `build/uefi/roninos_uefi.img` (mit `/EFI/BOOT/BOOTX64.EFI`)

### UEFI in QEMU starten

```bash
make uefi-run
```

Das Run-Script sucht OVMF robust über mehrere typische Pfade auf Ubuntu/Debian.

> Falls vorher der Fehler `error: file '/boot/roninos.elf' not found` erschien: Das war ein Root-Setup-Problem in der eingebetteten GRUB-Konfiguration. Das Image sucht jetzt aktiv die Kernel-Datei auf dem FAT-Image, bevor `multiboot2` ausgeführt wird.

### UEFI-ISO bauen

```bash
make clean && make
```

Erzeugt:

- `build/roninos.iso`

Die ISO kann sowohl für BIOS als auch (mit OVMF) für UEFI-Tests genutzt werden.

### UEFI-ISO in QEMU testen

```bash
OVMF_CODE=/usr/share/OVMF/OVMF_CODE.fd
qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
  -cdrom build/roninos.iso \
  -m 256M
```

Alternativ bleibt `make uefi-run` für den Test des FAT-UEFI-Images erhalten.

### USB-Stick flashen (UEFI)

#### Variante A: UEFI-FAT-Image direkt schreiben

```bash
make uefi-image
sudo dd if=build/uefi/roninos_uefi.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

`/dev/sdX` durch das **gesamte** USB-Device ersetzen (z. B. `/dev/sdb`, nicht `/dev/sdb1`).

#### Variante B: UEFI-Dateien auf bereits FAT32-formatierten USB-Stick kopieren

```bash
make uefi
sudo mkdir -p /mnt/usb
sudo mount /dev/sdX1 /mnt/usb
sudo mkdir -p /mnt/usb/EFI/BOOT /mnt/usb/boot
sudo cp build/uefi/BOOTX64.EFI /mnt/usb/EFI/BOOT/BOOTX64.EFI
sudo cp build/roninos.elf /mnt/usb/boot/roninos.elf
sync
sudo umount /mnt/usb
```

### Echte Hardware (UEFI)

1. Secure Boot deaktivieren (oder signierten Loader verwenden).
2. Entweder Variante A (dd) oder Variante B (kopieren) nutzen.
3. Im UEFI-Bootmenü den USB-Stick als UEFI-Gerät auswählen.

---

## CI

GitHub Actions nutzt die gleiche Container-Strategie wie der lokale Docker-Build:

- Build auf `push` (main) und `pull_request`
- Artefakt-Upload (`build/roninos.iso`, `build/roninos.elf`)
- Optionaler manueller `qemu-smoke` Job (`workflow_dispatch`)

## Shell-Kommandos

Die Shell bietet via `help` eine Übersicht aller verfügbaren Apps. Typische Kommandos:

- `help` – Zeigt alle Apps plus FAT32 Quick Guide.
- `echo <text>` – Gibt Text aus.
- `clear` – Bildschirm leeren.
- `about` – Kurzinformationen zum Kernel.
- `meminfo` – Heap-/Speicherinformationen.
- `heap_test` – Allokator-Selbsttest.
- `spawn <n>` – Worker-Threads erzeugen.
- `yield` – Freiwilliger Thread-Wechsel.
- `ps` – Scheduler-Thread-Tabelle ausgeben.
- `preempt on|off` – Timer-basiertes Scheduling aktivieren/deaktivieren.
- `fs <cmd>` – Legacy-RAMFS-Dateioperationen (direkter RAMFS-Zugriff).
- `ls [path]` – Verzeichnis über VFS auflisten.
- `cat <path>` – Datei über VFS ausgeben.
- `pwd` – Aktuelles VFS-Arbeitsverzeichnis.
- `cd [path]` – VFS-Arbeitsverzeichnis ändern.
- `fat32 <cmd>` – FAT32 auf dem selektierten Blockdevice.
- `disk` – erkannte Blockdevices anzeigen.
- `disk info <name>` – Details zu einem Blockdevice.
- `disk read <name> <lba> <count>` – sektorweiser Lese-Smoke-Test (Hexdump).

---

## Dateisysteme

### RAMFS (`fs`)

Für schnelle Tests im Speicher:

- `fs ls`
- `fs touch <name>`
- `fs write <name> <text>`
- `fs append <name> <text>`
- `fs cat <name>`
- `fs cp <src> <dst>`
- `fs mv <src> <dst>`
- `fs rm <name>`

### Blockdevices (`disk`)

Beim Boot versucht RōninOS aktuell ATA-Disks per PIO zu erkennen (Primary/Secondary, Master/Slave).

- `disk`
- `disk info <name>`
- `disk read <name> <lba> <count>`

Beispielausgabe:

```text
hd0  ATA  512B/sector  2097152 sectors  1.00 GiB
```

`disk read hd0 0 1` zeigt die ersten 256 Bytes des angeforderten Sektors als Hexdump (z. B. MBR/Bootsektor).

### FAT32 (`fat32`)

Die FAT32-Funktionen laufen auf einem ausgewaehlten Blockdevice (Whole-Disk-Volume, 512B/Sektor). Unterstützte Kommandos:

- `fat32 select <disk>`
- `fat32 format [disk] --yes`
- `fat32 mount`
- `fat32 info`
- `fat32 ls`
- `fat32 write <NAME.EXT> <text>`
- `fat32 edit <NAME.EXT> <text>`
- `fat32 cat <NAME.EXT>`
- `fat32 rm <NAME.EXT>`

**Hinweise zu FAT32:**

- Dateinamen müssen im **8.3-Format** sein (`NAME.EXT`).
- `write` erstellt neue Dateien oder überschreibt bestehende Inhalte.
- `edit` ist ein expliziter Alias für „bearbeiten/überschreiben“.
- `rm` entfernt den Directory-Eintrag und gibt den Daten-Cluster frei.
- Pro Datei gilt derzeit: Daten bis zur Clustergröße (aktuell 512 Bytes).

**Smoke-Test auf QEMU-Disk-Image (nicht Host-Disk formatieren):**

```bash
qemu-img create -f raw build/disk.img 1G
qemu-system-i386 -cdrom build/roninos.iso -m 256M \
  -drive file=build/disk.img,format=raw,if=ide,index=0,media=disk
```

Dann in der RoninOS-Shell:

```text
disk
fat32 format hd0 --yes
disk read hd0 0 1
fat32 info
fat32 ls
```

---

### VFS + Initrd (Multiboot2 Module)

- RAMFS wird beim Boot als Root-FS (`/`) via VFS gemountet.
- Multiboot2-Module werden als read-only Dateisystem unter `/initrd` gemountet.
- Beispiel in `iso/boot/grub/grub.cfg`:
  - `module2 /initrd/banner.txt banner.txt`

Smoke-Test nach dem Boot:

```bash
ls /
pwd
cd /initrd
ls /initrd
cat /initrd/banner.txt
```

Wenn keine Module vorhanden sind, bleibt `/initrd` als leeres Verzeichnis vorhanden.

---

## FAT32-Workflow (Create/Edit/Read/Delete)

Das ist die empfohlene Kommando-Reihenfolge für Nutzer:

1. **Dateisystem initialisieren**
   ```bash
   disk
   fat32 format hd0 --yes
   ```
2. **Datei anlegen (create)**
   ```bash
   fat32 write NOTE.TXT "Hallo RoninOS"
   ```
3. **Datei lesen (read)**
   ```bash
   fat32 cat NOTE.TXT
   ```
4. **Datei bearbeiten (edit/overwrite)**
   ```bash
   fat32 edit NOTE.TXT "Neue Version vom Inhalt"
   ```
5. **Datei erneut lesen (read)**
   ```bash
   fat32 cat NOTE.TXT
   ```
6. **Datei löschen (delete)**
   ```bash
   fat32 rm NOTE.TXT
   ```
7. **Bestätigen via Listing**
   ```bash
   fat32 ls
   ```

---

## Technische Architektur

### Shell

- Zerlegt Eingaben in `argv`-Token (inkl. Unterstützung für Anführungszeichen).
- Führt entweder built-ins (`help`) oder registrierte Apps aus.
- App-Registry wird zentral über `kernel/apps/apps.c` gepflegt.

### App-System

- Jede App exportiert einen `app_<name>_main(int argc, char** argv)` Einstiegspunkt.
- Apps stehen als Tabelle mit Name, Hilfe-Text und Funktion in `apps.c`.
- Vorteil: sehr einfache Erweiterung um neue Shell-Programme.

### FAT32-Design (vereinfacht)

- Backend: generischer Blockdevice-Layer (`blockdev`).
- `fat32_format()` legt Bootsektor, FSInfo, FAT und Root-Cluster an.
- `fat32_mount()` liest Metadaten ein und validiert Grundparameter.
- `fat32_write_file()` schreibt/verändert 8.3-Dateien im Root-Verzeichnis.
- `fat32_read_file()` liest Dateiinhalt anhand Directory-Eintrag und Cluster.
- `fat32_delete_file()` markiert Eintrag als gelöscht und gibt den Cluster frei.

---

## Entwicklungsleitlinien

- Kernel bleibt **freestanding**.
- Keine unnötigen Abhängigkeiten hinzufügen.
- Änderungen klein und reviewbar halten.
- Bei neuen Shell-Programmen: `kernel/apps/app_*.c` + Registrierung in `apps.c`.
- Nach Änderungen bauen mit:

```bash
make clean && make
```

### Multiboot2-Header verifizieren

```bash
make verify
```

Details zu Boot-Constraints und Checks: `docs/boot.md`.

---

## Troubleshooting

### `Unknown command. Type 'help'.`

- Kommando falsch geschrieben oder App nicht registriert.
- Mit `help` verfügbare Befehle prüfen.

### `mount failed`

- Auf dem selektierten Device ist kein gueltiges FAT32 vorhanden.
- Mit `fat32 format <disk> --yes` initialisieren.

### `write failed (name must be 8.3, data <= 512 bytes)`

- Dateiname verletzt 8.3-Regel oder Inhalt ist zu groß.
- Beispiel eines gültigen Namens: `TEST.TXT`.

### Build-Probleme

- Vollständigen Rebuild ausführen:
  ```bash
  make clean && make
  ```
- Toolchain-/Emulator-Setup prüfen.

---

## Roadmap-Ideen

- Unterverzeichnisse und Multi-Cluster-Dateien in FAT32.
- Verbesserte Text-Editor-App auf Basis FAT32.
- Partitionstabellen (MBR/GPT) und Partition-Mounts.
- Erweiterte Debug-Ausgaben für Scheduler/Memory.
- Testsuite für Dateisystem-Funktionen.

---

## Beitrag leisten

Pull Requests sind willkommen. Bitte:

1. Kleine, fokussierte Commits erstellen.
2. Build lokal verifizieren (`make clean && make`).
3. In der PR klar beschreiben:
   - Was geändert wurde,
   - warum es geändert wurde,
   - wie es getestet wurde.

Wenn du einen größeren Umbau planst, gerne vorab ein kurzes Issue eröffnen.

---

Viel Spaß beim Hacken an RōninOS ⚔️
