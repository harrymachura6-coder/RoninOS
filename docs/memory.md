# Memory Architecture (PMM -> Paging -> Heap)

## Überblick

RoninOS initialisiert Speicher jetzt in drei Schritten:

1. **PMM (Physical Memory Manager)** liest die Multiboot2-Memory-Map, verwaltet 4-KiB-Frames per Bitmap und reserviert kritische Bereiche.
2. **Paging Bootstrap** baut ein Page Directory + Page Tables auf und aktiviert Paging mit Identity-Mapping bis zur vom PMM verwalteten Obergrenze.
3. **Kernel Heap** verwendet `kmalloc/kfree` auf einem virtuellen Heap-Bereich und erweitert ihn bei Bedarf mit neuen Seiten aus dem PMM.

## 1) PMM

Datei: `kernel/mem/pmm.c`

- Framegröße: 4096 Byte.
- Verwalteter Bereich: bis max. 512 MiB (`PMM_MAX_PHYS_ADDR`), effektiv begrenzt durch Multiboot2-Infos.
- Belegte Bereiche nach Init:
  - Niedrigspeicher `< 1 MiB`
  - Kernel-Image (`_kernel_start`..`_kernel_end`)
  - Multiboot2-Infoblock
  - Module + Framebuffer (falls vorhanden)

API:

- `pmm_init(mb_magic, mb_info_addr)`
- `pmm_alloc_frame()` -> physische 4-KiB-Adresse oder `0`
- `pmm_free_frame(phys)`
- `pmm_dump_stats()` / `pmm_get_stats()`

## 2) Paging

Datei: `kernel/mem/paging.c`

- Nutzt 32-bit Paging (4 KiB Seiten, kein PAE).
- Baut dynamisch Page Tables aus einem Bootstrap-Pool (`MAX_BOOTSTRAP_TABLES`).
- Identity-Mapping von `0` bis `phys_limit` (aus PMM).
- Aktivierung:
  - `mov cr3, page_directory`
  - `cr0 |= PG`
- API:
  - `paging_init(phys_limit)`
  - `map_page(virt, phys, flags)`
  - `unmap_page(virt)`
  - `translate(virt)`
- TLB-Invalidierung bei Änderungen via `invlpg`.

## 3) Kernel Heap

Datei: `kernel/heap.c`

- Virtueller Heap-Bereich beginnt bei `0x18000000`.
- Initial werden 4 Seiten gemappt.
- Allocator: first-fit Freiliste mit Split/Merge.
- Wenn kein Block passt: Heap erweitert sich per PMM-Frames + `map_page`.
- Thread-Safety:
  - einfacher Spinlock (`__sync_lock_test_and_set`)
  - Interrupts werden im kritischen Abschnitt per `cli` gesperrt und Flags restauriert.

## Boot-Reihenfolge

`kernel/kernel.c`:

1. `pmm_init(...)`
2. `paging_init(...)`
3. `heap_init(...)`
4. Speicher-Smoke-Test (`kmalloc(16)`, `kmalloc(4096)`, `kmalloc(8192)`, `kfree`, Re-Alloc)
5. danach IDT/ISR/Keyboard/Scheduler

## Testen

1. Bauen:

```bash
make clean && make
```

2. Lauf starten:

```bash
make run
```

3. Erwartete Boot-Ausgaben:

- Memory map summary
- PMM stats
- Paging enabled confirmation
- Memory smoke test `PASS`

4. Shell-Checks:

- `meminfo` zeigt Heap- und PMM-Werte.
- `heap_test` sollte weiterhin PASS liefern.
