# Scheduler

## Phase A: kooperatives Round-Robin

- `sched_init()` legt den Main-Thread (`tid=0`) an.
- `thread_create()` erzeugt Kernel-Threads mit eigenem Stack aus `kmalloc`.
- `thread_yield()` wechselt auf den nächsten `RUNNABLE` Thread.
- `thread_exit()` markiert den aktuellen Thread als `ZOMBIE` und schaltet auf den nächsten Thread.
- `ps` zeigt die Thread-Tabelle (`tid`, Name, State, `esp`, Stack-Bereich).

## Phase B: optional preemptive über PIT

- PIT wird auf 100 Hz initialisiert (`pit_init(100)`).
- IRQ0 ruft `thread_yield()` nur, wenn `preempt on` gesetzt ist.
- Standard ist `preempt off`, damit kooperatives Debuggen einfach bleibt.

## Shell-Tests

1. `spawn 3`
   - Erzeugt drei Worker-Threads.
2. `ps`
   - Prüft, ob Threads mit separaten Stackbereichen sichtbar sind.
3. `yield`
   - Lässt die Threads kooperativ rotieren.
4. `preempt on`
   - Aktiviert Timer-basiertes Umschalten ohne manuelles `yield`.
5. `preempt off`
   - Deaktiviert automatisches Umschalten wieder.
