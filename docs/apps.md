# Kernel Apps erweitern

RoninOS nutzt eine einfache Kernel-App-Tabelle (kein Userspace, kein ELF-Loader).

## Neue App hinzufügen

1. Neue Datei anlegen: `kernel/apps/app_<name>.c`
2. Einstiegspunkt implementieren:

```c
int app_<name>_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
}
```

3. In `kernel/apps/apps.c` deklarieren und in `g_apps` registrieren:

```c
int app_<name>_main(int argc, char** argv);

{ "<name>", "<name> - kurze Hilfe", app_<name>_main },
```

4. Im `Makefile` die neue Objektdatei ergänzen:

```make
build/app_<name>.o
```

und Build-Regel ergänzen.

Danach bauen mit:

```sh
make clean && make
```

In der Shell listet `help` alle registrierten Apps auf.

## Neue eingebaute Shell-App: `fs`

Als Grundlage fuer ein Linux-aehnliches CLI gibt es jetzt eine kleine RAM-Dateiverwaltung:

```sh
fs ls
fs touch notes
fs write notes "hello ronin"
fs append notes " + more"
fs cat notes
fs cp notes notes.bak
fs mv notes.bak archive
fs rm notes
```

Hinweise:
- rein im RAM (nicht persistent nach Neustart)
- begrenzte Anzahl Dateien und Dateigroesse (MVP)

## FAT32 (Blockdevice-Backend)

Kernel-App-Befehl `fat32` nutzt jetzt ein ausgewaehltes Blockdevice (`disk`/`disk info`):

```sh
disk
fat32 select hd0
fat32 format --yes
fat32 info
fat32 write NOTE.TXT "hello"
fat32 ls
fat32 cat NOTE.TXT
```

Status:
- formatiert ein FAT32-Volume direkt auf dem gewaehlten Blockdevice (Whole Disk)
- mountet und nutzt Root-Directory (8.3-Dateinamen)
- liest/schreibt einzelne Dateien (aktuell max. 1 Cluster pro Datei)
