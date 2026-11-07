# EepromStoreTest

A self-checking Arduino sketch for Inertia’s AVR EEPROM storage backends. It exercises CRC-checked records, wear leveling, synchronous rotating journals, and cooperative asynchronous journal saves.

## Test coverage

- Verify that a written record can be read back with its fields intact.
- Reject reads from erased EEPROM and leave the caller's output unchanged.
- Verify that overwriting a record returns the newest value.
- Confirm that changing the store version invalidates data written with the previous version.
- Reject a CRC-corrupted record without changing the caller's output.
- Write multiple records through an `x2` wear-level store, reconstruct the store between writes, and verify the latest record after counter rollover.
- Complete cooperative async writes for standard and wear-level stores by repeatedly polling, including reads of an unrelated valid record and rejection of a competing write while the EEPROM is busy.
- Drive an async EEPROM write through the `AsyncWriteTask` TaskScheduler adapter and verify its default reject-while-busy request policy.
- Save versioned records through the typed EEPROM journal task and verify accepted-request, completion, and rejection behavior.
- Rotate asynchronously through journal slots and load the newest committed snapshot.
- Stop an async save before its commit marker and recover the previous committed snapshot.
- Inject an async byte-write failure in a RAM byte-store fake and verify the previous snapshot remains loadable and the operation lock is released.
- Benchmark synchronous reads/writes and cooperative async writes for standard and wear-level stores, reporting minimum, maximum, and median (typical) durations over 16 samples.
- Check at compile time that standard and wear-level stores report their expected storage footprints.

## How the sketch works

Define `EEPROM_MOCK_IN_MEMORY` before the Inertia headers to route EEPROM operations to a RAM array with the same capacity as the target EEPROM. Without the mock, the EEPROM store and journal tests use physical EEPROM. The tests call `EraseEEPROM()`, so run without the mock only when erasing the target EEPROM is acceptable.

The synchronous `StructStore` and EmbeddedStorage code performs CRC calculation, storage selection, reads, writes, and wear-level counter handling. Async use is explicit through `AsyncStructStore`, which selects EmbeddedStorage's async unit variants and adds a copied record buffer. Reported timings depend on the selected backend: mock timings measure software/RAM overhead, while physical EEPROM timings include device latency. The journal interruption check stops after a completed byte operation and before writing the commit marker; it validates recovery logic but does not simulate an actual power cut or measure device endurance. The injected failure check uses a RAM byte-store fake.

For cooperative writes, use `AsyncStructStore` instead of `StructStore`, call `BeginWrite(record)` once, then call `PollWrite()` from a task or loop until it returns `Complete` or `Failed`. Each poll advances at most one byte operation, and the async store copies the record when starting; keep the async store instance alive until polling finishes. Reads remain available during a pending write, though reading the record being updated may return false while its data and CRC are temporarily inconsistent. A read may wait for an EEPROM byte program already in progress. Writes and erase operations remain excluded until completion. No interrupt callback is used. Synchronous-only `StructStore` instances do not carry the async record buffer or polling state.

`AsyncWriteTask<Store>` adapts an `AsyncStructStore` to TaskScheduler. `AsyncVersionedRecordJournalTask<Journal>` accepts typed records and starts one save immediately; `RequestSave` returns false while another save is active or the EEPROM operation lock cannot be acquired. Rejected requests are not queued. Check `IsSaveActive()` to observe whether the accepted save is pending, then `GetLastCompletedStatus()` for its terminal result. The task disables itself when idle and re-enables when a request is accepted.

## Run the sketch

Open `EepromStoreTest.ino` in the Arduino IDE with Inertia, EmbeddedStorage, and CRC available, then build and upload it to a supported AVR board (such as an Uno). Open Serial Monitor at **115200 baud**. The checks and operation benchmark run once from `setup()`; `loop()` is intentionally empty. Each runtime check prints `PASS` or `FAIL`, followed by a compact table that groups minimum, median, and maximum timings for synchronous operations and async writes. Compile-time `static_assert` checks run during the build and do not print to Serial.
