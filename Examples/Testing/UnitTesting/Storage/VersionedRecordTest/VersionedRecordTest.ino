#include <Arduino.h>
#include <InertiaModel.h>

using namespace Inertia::Components::Storage::VersionedRecord;

// Version 1 stores active time as 32 bits; version 2 widens it and adds a start count.
struct LegacyRecord
{
	HeaderStruct Header;
	uint32_t ActiveSeconds;
};

struct CurrentRecord
{
	HeaderStruct Header;
	uint64_t ActiveSeconds;
	uint16_t StartCount;
};

using CurrentCodec = Codec<CurrentRecord, HeaderStruct, 2>;
using FutureCodec = Codec<CurrentRecord, HeaderStruct, 3>;

struct LegacyCodec : Codec<LegacyRecord, HeaderStruct, 1>
{
	using NextCodec = CurrentCodec;

	static bool MigrateToNext(const StorageType& source, CurrentCodec::StorageType& destination)
	{
		destination.ActiveSeconds = source.ActiveSeconds;
		destination.StartCount = 0;
		return true;
	}
};

struct ChainedV1Record
{
	HeaderStruct Header;
	uint32_t ActiveSeconds;
};

struct ChainedV2Record
{
	HeaderStruct Header;
	uint64_t ActiveSeconds;
	uint16_t StartCount;
	uint8_t Reserved;
};

struct ChainedV3Record
{
	HeaderStruct Header;
	uint64_t ActiveSeconds;
	uint16_t StartCount;
	uint32_t RetiredCounters[4];
};

struct ChainedCurrentRecord
{
	HeaderStruct Header;
	uint64_t ActiveSeconds;
	uint16_t StartCount;
};

using ChainedCurrentCodec = Codec<ChainedCurrentRecord, HeaderStruct, 4>;

struct ChainedV3Codec : Codec<ChainedV3Record, HeaderStruct, 3>
{
	using NextCodec = ChainedCurrentCodec;

	static bool MigrateToNext(const StorageType& source, ChainedCurrentCodec::StorageType& destination)
	{
		destination.ActiveSeconds = source.ActiveSeconds;
		destination.StartCount = source.StartCount;
		return true;
	}
};

struct ChainedV2Codec : Codec<ChainedV2Record, HeaderStruct, 2>
{
	using NextCodec = ChainedV3Codec;

	static bool MigrateToNext(const StorageType& source, ChainedV3Codec::StorageType& destination)
	{
		if (source.Reserved != 0) { return false; }
		destination.ActiveSeconds = source.ActiveSeconds;
		destination.StartCount = source.StartCount;
		return true;
	}
};

struct ChainedV1Codec : Codec<ChainedV1Record, HeaderStruct, 1>
{
	using NextCodec = ChainedV2Codec;

	static bool MigrateToNext(const StorageType& source, ChainedV2Codec::StorageType& destination)
	{
		destination.ActiveSeconds = source.ActiveSeconds;
		destination.StartCount = 0;
		destination.Reserved = 0;
		return true;
	}
};

using ChainedMigrationDispatcher = Internal::CodecDispatcher<
	ChainedCurrentCodec, ChainedV3Codec, ChainedV2Codec, ChainedV1Codec>;

// Dispatch through registered immediate versions so the legacy codec can migrate into the current format.
using MigrationDispatcher = Internal::CodecDispatcher<CurrentCodec, LegacyCodec>;

// Keep the test's format registry well-formed and size its storage for the largest registered record.
static_assert(Internal::VersionsAreUnique<CurrentCodec, LegacyCodec>::Value,
	"Current and legacy record versions must be unique.");
static_assert(Internal::AllMigrationChainsAreValid<CurrentCodec, LegacyCodec>::Value,
	"Legacy record codecs must migrate through registered immediate versions.");
static_assert(Internal::MaximumRecordSize<CurrentCodec, LegacyCodec>::Value == sizeof(CurrentRecord),
	"The codec registry must report its largest record format.");
using MigrationEnvelope = Envelope<Internal::MaximumRecordSize<CurrentCodec, LegacyCodec>::Value>;
static_assert(sizeof(MigrationEnvelope) == sizeof(uint16_t) + Internal::MaximumRecordSize<CurrentCodec, LegacyCodec>::Value,
	"The versioned-record envelope must reserve a fixed byte capacity plus its encoded-size field.");
static_assert(Internal::VersionsAreUnique<ChainedCurrentCodec,
	ChainedV3Codec, ChainedV2Codec, ChainedV1Codec>::Value,
	"The chained migration versions must be unique.");
static_assert(Internal::AllMigrationChainsAreValid<ChainedCurrentCodec,
	ChainedV3Codec, ChainedV2Codec, ChainedV1Codec>::Value,
	"The chained migration codecs must link through immediate versions.");
static_assert(Internal::MaximumRecordSize<ChainedCurrentCodec,
	ChainedV3Codec, ChainedV2Codec, ChainedV1Codec>::Value == sizeof(ChainedV3Record),
	"The largest chained migration record must determine the registry size.");
static_assert(sizeof(ChainedCurrentRecord) < sizeof(ChainedV3Record),
	"The version-3 record must be larger than the current record.");

uint8_t FailedChecks = 0;

void Report(const char* name, const bool passed)
{
	Serial.print(passed ? F("PASS: ") : F("FAIL: "));
	Serial.println(name);
	if (!passed) { FailedChecks++; }
}

bool TestCurrentRecordRoundTrip()
{
	CurrentRecord input{};
	input.ActiveSeconds = 987654;
	input.StartCount = 17;
	uint8_t bytes[CurrentCodec::RecordSize]{};
	if (CurrentCodec::Encode(input, bytes, sizeof(bytes)) != CodecResultEnum::Success) { return false; }

	CurrentRecord output{};
	return CurrentCodec::Decode(bytes, sizeof(bytes), output) == CodecResultEnum::Success
		&& output.ActiveSeconds == input.ActiveSeconds
		&& output.StartCount == input.StartCount;
}

bool TestBufferAndSizeValidation()
{
	CurrentRecord record{};
	uint8_t bytes[CurrentCodec::RecordSize]{};
	uint8_t oversizedBytes[CurrentCodec::RecordSize + 1]{};
	return CurrentCodec::Encode(record, bytes, sizeof(bytes) - 1) == CodecResultEnum::BufferTooSmall
		&& CurrentCodec::Encode(record, nullptr, sizeof(bytes)) == CodecResultEnum::InvalidArgument
		&& CurrentCodec::Decode(bytes, sizeof(bytes) - 1, record) == CodecResultEnum::InvalidSize
		&& CurrentCodec::Encode(record, oversizedBytes, sizeof(oversizedBytes)) == CodecResultEnum::Success
		&& CurrentCodec::Decode(oversizedBytes, sizeof(oversizedBytes), record) == CodecResultEnum::InvalidSize;
}

bool TestChecksumRejection()
{
	CurrentRecord record{};
	uint8_t bytes[CurrentCodec::RecordSize]{};
	if (CurrentCodec::Encode(record, bytes, sizeof(bytes)) != CodecResultEnum::Success) { return false; }

	bytes[offsetof(CurrentRecord, StartCount)] ^= 1;
	CurrentRecord output{};
	return CurrentCodec::Decode(bytes, sizeof(bytes), output) == CodecResultEnum::ChecksumMismatch
		&& output.ActiveSeconds == 0
		&& output.StartCount == 0;
}

bool TestLegacyMigration()
{
	LegacyRecord legacy{};
	legacy.ActiveSeconds = 12345;
	if (LegacyCodec::EncodeInPlace(legacy) != CodecResultEnum::Success) { return false; }

	CurrentRecord migrated{};
	return MigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&legacy), sizeof(legacy), migrated)
			== CodecResultEnum::Success
		&& migrated.ActiveSeconds == legacy.ActiveSeconds
		&& migrated.StartCount == 0;
}

bool TestMultiStepMigration()
{
	ChainedV1Record legacy{};
	legacy.ActiveSeconds = 765432;
	if (ChainedV1Codec::EncodeInPlace(legacy) != CodecResultEnum::Success) { return false; }

	ChainedCurrentRecord migrated{};
	return ChainedMigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&legacy), sizeof(legacy), migrated)
			== CodecResultEnum::Success
		&& migrated.ActiveSeconds == legacy.ActiveSeconds
		&& migrated.StartCount == 0;
}

bool TestMigrationShrinksRecord()
{
	ChainedV3Record legacy{};
	legacy.ActiveSeconds = 876543;
	legacy.StartCount = 23;
	legacy.RetiredCounters[0] = 10;
	legacy.RetiredCounters[1] = 20;
	legacy.RetiredCounters[2] = 30;
	legacy.RetiredCounters[3] = 40;
	if (ChainedV3Codec::EncodeInPlace(legacy) != CodecResultEnum::Success) { return false; }

	ChainedCurrentRecord migrated{};
	return ChainedMigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&legacy), sizeof(legacy), migrated)
			== CodecResultEnum::Success
		&& sizeof(migrated) < sizeof(legacy)
		&& migrated.ActiveSeconds == legacy.ActiveSeconds
		&& migrated.StartCount == legacy.StartCount;
}

bool TestFailedMigrationLeavesOutputUnchanged()
{
	ChainedV2Record legacy{};
	legacy.ActiveSeconds = 765;
	legacy.StartCount = 9;
	legacy.Reserved = 1;
	if (ChainedV2Codec::EncodeInPlace(legacy) != CodecResultEnum::Success) { return false; }

	ChainedCurrentRecord output{};
	output.ActiveSeconds = 321;
	output.StartCount = 7;
	return ChainedMigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&legacy), sizeof(legacy), output)
			== CodecResultEnum::MigrationFailed
		&& output.ActiveSeconds == 321
		&& output.StartCount == 7;
}

bool TestCorruptLegacyRecordRejection()
{
	LegacyRecord legacy{};
	legacy.ActiveSeconds = 12345;
	if (LegacyCodec::EncodeInPlace(legacy) != CodecResultEnum::Success) { return false; }

	reinterpret_cast<uint8_t*>(&legacy)[offsetof(LegacyRecord, ActiveSeconds)] ^= 1;
	CurrentRecord output{};
	output.ActiveSeconds = 321;
	output.StartCount = 7;
	return MigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&legacy), sizeof(legacy), output)
			== CodecResultEnum::ChecksumMismatch
		&& output.ActiveSeconds == 321
		&& output.StartCount == 7;
}

bool TestUnknownVersionRejection()
{
	CurrentRecord future{};
	if (FutureCodec::EncodeInPlace(future) != CodecResultEnum::Success) { return false; }

	CurrentRecord output{};
	return MigrationDispatcher::Decode(reinterpret_cast<const uint8_t*>(&future), sizeof(future), output)
		== CodecResultEnum::UnsupportedVersion;
}

void setup()
{
	Serial.begin(115200);
	Report("current record encodes and decodes", TestCurrentRecordRoundTrip());
	Report("invalid buffer and record sizes are rejected", TestBufferAndSizeValidation());
	Report("corrupt payload is rejected without modifying output", TestChecksumRejection());
	Report("legacy record dispatches through its migration", TestLegacyMigration());
	Report("legacy records migrate through multiple versions", TestMultiStepMigration());
	Report("migration drops retired fields from larger records", TestMigrationShrinksRecord());
	Report("failed migration is rejected without modifying output", TestFailedMigrationLeavesOutputUnchanged());
	Report("corrupt legacy records are rejected before migration", TestCorruptLegacyRecordRejection());
	Report("unknown record version is rejected", TestUnknownVersionRejection());
	Serial.flush();
}

void loop()
{}