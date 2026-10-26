# EepromStoreTest

A self-checking Arduino sketch for `Inertia::Drivers::Storage::Eeprom::StructStore`. It exercises CRC-checked record reads and writes and the optional wear-level storage path using the real EmbeddedStorage algorithms.

## Test coverage

- Verify that a written record can be read back with its fields intact.
- Reject reads from erased EEPROM and leave the caller's output unchanged.
- Verify that overwriting a record returns the newest value.
- Confirm that changing the store version invalidates data written with the previous version.
- Reject a CRC-corrupted record without changing the caller's output.
- Write multiple records through an `x2` wear-level store, reconstruct the store between writes, and verify the latest record after counter rollover.
- Complete cooperative async writes for standard and wear-level stores by repeatedly polling, including reads of an unrelated valid record and rejection of a competing write while the EEPROM is busy.
- Benchmark synchronous reads/writes and cooperative async writes for standard and wear-level stores, reporting minimum, maximum, and median (typical) durations over 16 samples.
- Check at compile time that standard and wear-level stores report their expected storage footprints.

## How the sketch works

Define `EEPROM_MOCK_IN_MEMORY` before the Inertia headers to route EEPROM operations to a RAM array with the same capacity as the target EEPROM. Without the mock, operations use physical EEPROM. The tests call `EraseEEPROM()`, so run without the mock only when erasing the target EEPROM is acceptable.

The synchronous `StructStore` and EmbeddedStorage code performs CRC calculation, storage selection, reads, writes, and wear-level counter handling. Async use is explicit through `AsyncStructStore`, which selects EmbeddedStorage's async unit variants and adds a copied record buffer. Reported timings depend on the selected backend: mock timings measure software/RAM overhead, while physical EEPROM timings include device latency. Neither mode validates power-loss behavior or real-device endurance.

For cooperative writes, use `AsyncStructStore` instead of `StructStore`, call `BeginWrite(record)` once, then call `PollWrite()` from a task or loop until it returns `Complete` or `Failed`. Each poll advances at most one byte operation, and the async store copies the record when starting; keep the async store instance alive until polling finishes. Reads remain available during a pending write, though reading the record being updated may return false while its data and CRC are temporarily inconsistent. A read may wait for an EEPROM byte program already in progress. Writes and erase operations remain excluded until completion. No interrupt callback is used. Synchronous-only `StructStore` instances do not carry the async record buffer or polling state.

`AsyncWriteTask<Store>` adapts an `AsyncStructStore` to TaskScheduler: construct it with a scheduler and store, then call `RequestWrite(record)`. The default `RejectWhileBusy` policy rejects requests while a write is active. `ReplacePending` keeps one copied pending record and replaces it with each newer request while the active write finishes. Check `GetLastCompletedWriteStatus()` and `GetCompletedWriteCount()` for the last result and total completions. The task disables itself when idle and re-enables when a request is accepted.

## Run the sketch

Open `EepromStoreTest.ino` in the Arduino IDE with Inertia, EmbeddedStorage, and CRC available, then build and upload it to a supported AVR board (such as an Uno). Open Serial Monitor at **115200 baud**. The checks and operation benchmark run once from `setup()`; `loop()` is intentionally empty. Each runtime check prints `PASS` or `FAIL`, followed by a compact table that groups minimum, median, and maximum timings for synchronous operations and async `BeginWrite`, worst single `PollWrite`, and completion latency at a 1 ms poll cadence. Compile-time `static_assert` checks run during the build and do not print to Serial.

A hardware-backed EEPROM test would require a separate test sketch with the mock disabled and deliberate handling of the target's physical EEPROM contents.
