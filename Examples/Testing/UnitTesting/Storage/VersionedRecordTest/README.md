# VersionedRecordTest

A self-checking Arduino sketch for the `Inertia::Components::Storage::VersionedRecord` codecs. It checks record encoding, validation, and version dispatch without requiring a storage device.

## Test coverage

- Encode and decode a current version-2 record, verifying both payload fields.
- Reject too-small encode buffers, null encode buffers, and undersized or oversized records.
- Detect modified payloads in both current and legacy records; failed decoding leaves the output record unchanged.
- Decode a version-1 record through its registered migration to version 2. The legacy `uint32_t` active-time value is retained and the new `StartCount` field defaults to zero.
- Follow a multi-step version-1 → version-2 → version-3 → version-4 migration chain.
- Migrate a larger version-3 record to the smaller current version-4 format while preserving active time and start count and dropping retired counters.
- Reject a legacy record when its migration fails, without changing the destination record.
- Reject a record carrying an unregistered future version.
- Check at compile time that codec versions are unique, migration chains are valid, and each registry's maximum record size is correct.

## Version dispatch

```text
+-------------------------------+
| Stored record                 |
| Header + versioned payload    |
+---------------+---------------+
				|
				v
	   +------------------+
	   | Read record      |
	   | version from     |
	   | Header           |
	   +--------+---------+
				|
		+-------+-------+
		|               |
		v               v
+---------------+  +----------------+
| Version 1     |  | Version 2      |
| LegacyCodec   |  | CurrentCodec   |
+-------+-------+  +--------+-------+
		|                   |
		v                   v
+------------------+  +------------------+
| Migrate fields   |  | Decode current   |
| ActiveSeconds    |  | record directly  |
| StartCount = 0   |  +--------+---------+
+--------+---------+           |
		 |                     |
		 +----------+----------+
					v
		  +-------------------+
		  | Current version-2 |
		  | record in memory  |
		  +-------------------+
```

```text
Unregistered version (for example, version 3)
					|
					v
		UnsupportedVersion result
```

The extended fixture verifies multi-step conversion and dropping fields that are no longer stored:

```text
Stored V1 record
	  |
	  v
  migrate V1 -> V2
	  |
	  v
  migrate V2 -> V3
	  |
	  v
  migrate V3 -> V4; discard retired counters
	  |
	  v
Current V4 record

V3 record (larger)  -------------------->  V4 record (smaller)
```

An invalid reserved value makes the V2 migration fail; a checksum-corrupt V1 record is rejected before migration. In both cases the dispatcher leaves the caller's destination unchanged.

## Run the sketch

Open `VersionedRecordTest.ino` in the Arduino IDE with the Inertia library available, then build and upload it to a supported board. Open Serial Monitor at **115200 baud** to see one `PASS` or `FAIL` line for each runtime check. The checks run once from `setup()`; `loop()` is intentionally empty.

The sketch uses in-memory record buffers only. It exercises codec and migration behavior, not EEPROM, filesystem, or storage-device durability. Compile-time `static_assert` checks run during the build and do not print to Serial.
