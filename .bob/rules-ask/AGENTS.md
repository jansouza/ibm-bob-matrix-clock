# AGENTS.md — Ask mode

This file provides guidance to agents answering questions about this repository.

## Key Context for Answering Questions

- **Phase 1 only is implemented** — `text_encoding`, `locale_data`, `persistence`, `web_page`, `web_routes`, `data_fetcher` modules exist in the plan but not yet in the file tree.
- **Spec and roadmap are in Portuguese** — [`docs/project-spec.md`](docs/project-spec.md) and [`docs/implementation-plan.md`](docs/implementation-plan.md).
- **No RTOS** — the firmware is cooperative; everything runs in a single `loop()` with `millis()` timers. Questions about "threading" or "tasks" don't apply.
- **`configTime(0,0,server)` sets UTC** — timezone is handled entirely by a POSIX TZ string via `setenv("TZ",...)`. The UTC offsets in `configTime` are always 0.
- **Clock validity heuristic**: year ≥ 2020 means NTP-synced; epoch (year < 2020) means not yet synced.
- **`alertMessage[]` is Latin-1 encoded** (not UTF-8) — HTTP input must be converted before storage.
- **`slotIntervalMs[0]` = 0** — the clock slot has no display interval because it is the permanent base, not a rotating slot.
- **`DISPLAY_HARDWARE = MD_MAX72XX::FC16_HW`** — this must match the physical module type or the column mapping will be wrong.
