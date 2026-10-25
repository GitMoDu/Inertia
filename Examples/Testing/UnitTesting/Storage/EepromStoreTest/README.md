# EepromStoreTest

A self-checking Arduino sketch for `Inertia::Drivers::Storage::Eeprom::StructStore`. It exercises CRC-checked record reads and writes and the optional wear-level storage path using the real EmbeddedStorage algorithms.

## Test coverage

- Verify that a written record can be read back with its fields intact.
- Reject reads from erased EEPROM and leave the caller's output unchanged.
- Verify that overwriting a record returns the newest value.
- Confirm that changing the store version invalidates data written with the previous version.
- Reject a CRC-corrupted record without changing the caller's output.
- Write multiple records through an `x2` wear-level store, reconstruct the store between writes, and verify the latest record after counter rollover.
- Benchmark standard and wear-level store reads and writes, reporting minimum, maximum, and median (typical) durations over 16 samples.
- Check at compile time that standard and wear-level stores report their expected storage footprints.

## How the sketch works

Define `EEPROM_MOCK_IN_MEMORY` before the Inertia headers to route EEPROM operations to a RAM array with the same capacity as the target EEPROM. Without the mock, operations use physical EEPROM. The tests call `EraseEEPROM()`, so run without the mock only when erasing the target EEPROM is acceptable.

The normal `StructStore` and EmbeddedStorage code performs CRC calculation, storage selection, reads, writes, and wear-level counter handling. Reported timings depend on the selected backend: mock timings measure software/RAM overhead, while physical EEPROM timings include device latency. Neither mode validates power-loss behavior or real-device endurance.

## Run the sketch

Open `EepromStoreTest.ino` in the Arduino IDE with Inertia, EmbeddedStorage, and CRC available, then build and upload it to a supported AVR board (such as an Uno). Open Serial Monitor at **115200 baud**. The checks and operation benchmark run once from `setup()`; `loop()` is intentionally empty. Each runtime check prints `PASS` or `FAIL`, followed by min/max/median read and write timings for both store types. Compile-time `static_assert` checks run during the build and do not print to Serial.

A hardware-backed EEPROM test would require a separate test sketch with the mock disabled and deliberate handling of the target's physical EEPROM contents.
