//#define EEPROM_MOCK_IN_MEMORY

#define _TASK_OO_CALLBACKS
#include <TScheduler.hpp>

#include <Arduino.h>
#include <string.h>
#include <InertiaDrivers.h>
#include <Drivers/Storage/Eeprom/VersionedRecordStore.h>
#include <InertiaModel.h>

using namespace Inertia::Drivers::Storage::Eeprom;
using namespace Inertia::Components::Storage::VersionedRecord;

struct TestRecord
{
	uint32_t Sequence;
	uint16_t Value;
};

struct VersionedV1Record
{
	HeaderStruct Header;
	uint32_t Value;
};

struct VersionedCurrentRecord
{
	HeaderStruct Header;
	uint32_t Value;
	uint16_t Count;
};

using VersionedCurrentCodec = Codec<VersionedCurrentRecord, HeaderStruct, 2>;
struct VersionedV1Codec : Codec<VersionedV1Record, HeaderStruct, 1>
{
	using NextCodec = VersionedCurrentCodec;

	static bool MigrateToNext(const StorageType& source, VersionedCurrentCodec::StorageType& destination)
	{
		destination.Value = source.Value;
		destination.Count = 0;
		return true;
	}
};
using VersionedDispatcher = Inertia::Components::Storage::VersionedRecord::Internal::CodecDispatcher<
	VersionedCurrentCodec, VersionedV1Codec>;
constexpr size_t VersionedCapacity = Inertia::Components::Storage::VersionedRecord::Internal::MaximumRecordSize<
	VersionedCurrentCodec, VersionedV1Codec>::Value;
using VersionedStore = VersionedRecordStore<VersionedCurrentCodec, VersionedDispatcher,
	VersionedCapacity, false, 1, 320>;
using AsyncVersionedStore = VersionedRecordStore<VersionedCurrentCodec, VersionedDispatcher,
	VersionedCapacity, true, 1, 352>;

using Store = StructStore<TestRecord, 3, 0>;
using VersionChangedStore = StructStore<TestRecord, 4, 0>;
using WearLevelStore = StructStore<TestRecord, 4, 64, WearLevelTiny, WearLevelTiny::x2>;
using AsyncStore = AsyncStructStore<TestRecord, 3, 128>;
using AsyncWearLevelStore = AsyncStructStore<TestRecord, 4, 192, WearLevelTiny, WearLevelTiny::x2>;
using CompetingAsyncStore = AsyncStructStore<TestRecord, 3, 240>;
using ConcurrentReadStore = StructStore<TestRecord, 3, 256>;
using JournalStore = Journal<TestRecord, 300, 4, 0x4A4E4C31UL>;
using VersionedJournalStore = Journal<VersionedStore::EnvelopeType, 400, 4, 0x56524A31UL>;

constexpr uint8_t BenchmarkSamples = 16;
constexpr uint8_t StoreColumnWidth = 14;
constexpr uint8_t MeasurementColumnWidth = 18;
constexpr uint8_t NumericColumnWidth = 8;
constexpr uint8_t NumericGroupWidth = NumericColumnWidth * 3 + 6;
constexpr uint8_t FailureColumnWidth = 4;

static_assert(Store::UsedSize == sizeof(TestRecord) + 1,
	"A standard EEPROM record must include one CRC byte.");
static_assert(WearLevelStore::UsedSize == EmbeddedStorage::GetStorageSize(sizeof(TestRecord), WearLevelTiny::x2),
	"The wear-level store must report its allocated EEPROM footprint.");
static_assert(sizeof(Store) < sizeof(AsyncStore),
	"Synchronous stores must not include asynchronous unit state or record buffers.");

uint8_t FailedChecks = 0;

void Report(const __FlashStringHelper* name, const bool passed)
{
	Serial.print(passed ? F("PASS: ") : F("FAIL: "));
	Serial.println(name);
	if (!passed) { FailedChecks++; }
}

bool TestJournalStartsEmptyAndSavesSnapshot()
{
	EmbeddedEEPROM::EraseEEPROM();
	JournalStore journal;
	if (journal.Load().Status != JournalStore::LoadStatus::NoValidSnapshot) { return false; }

	TestRecord expected{};
	expected.Sequence = 1;
	expected.Value = 23;
	if (!journal.Save(expected)) { return false; }

	const JournalStore::LoadResult loaded = journal.Load();
	return loaded.HasSnapshot() && loaded.Sequence == 1
		&& loaded.Value.Sequence == expected.Sequence && loaded.Value.Value == expected.Value;
}

bool TestJournalRotatesAndRecoversPreviousValidSnapshot()
{
	EmbeddedEEPROM::EraseEEPROM();
	JournalStore journal;
	TestRecord expected{};
	for (uint32_t sequence = 1; sequence <= JournalStore::Capacity + 2; ++sequence)
	{
		expected.Sequence = sequence;
		expected.Value = static_cast<uint16_t>(sequence * 7);
		if (!journal.Save(expected)) { return false; }
	}

	JournalStore::LoadResult loaded = journal.Load();
	if (!loaded.HasSnapshot() || loaded.Sequence != JournalStore::Capacity + 2
		|| loaded.Value.Sequence != JournalStore::Capacity + 2)
	{
		return false;
	}

	const uint16_t latestSlot = static_cast<uint16_t>((loaded.Sequence - 1) % JournalStore::Capacity);
	const uint16_t commitAddress = static_cast<uint16_t>(JournalStore::Base
		+ latestSlot * JournalStore::SlotSizeBytes + JournalStore::SlotSizeBytes - 1);
	EmbeddedEEPROM::WriteBlock(commitAddress, 0);
	loaded = journal.Load();
	if (!loaded.HasSnapshot() || loaded.Sequence != JournalStore::Capacity + 1
		|| loaded.Value.Sequence != JournalStore::Capacity + 1)
	{
		return false;
	}

	const uint16_t priorCommitAddress = static_cast<uint16_t>(JournalStore::Base
		+ ((loaded.Sequence - 1) % JournalStore::Capacity) * JournalStore::SlotSizeBytes
		+ JournalStore::SlotSizeBytes - 1);
	const uint16_t priorPayloadAddress = static_cast<uint16_t>(priorCommitAddress
		- JournalStore::EntrySize - sizeof(uint32_t));
	EmbeddedEEPROM::WriteBlock(priorPayloadAddress, EmbeddedEEPROM::ReadBlock(priorPayloadAddress) ^ 1);
	loaded = journal.Load();
	return loaded.HasSnapshot() && loaded.Sequence == JournalStore::Capacity
		&& loaded.Value.Sequence == JournalStore::Capacity;
}

bool TestVersionedEnvelopeInEepromJournal()
{
	EmbeddedEEPROM::EraseEEPROM();
	VersionedJournalStore journal;
	VersionedCurrentRecord expected{};
	expected.Value = 654321;
	expected.Count = 42;
	VersionedJournalStore::EntryType envelope{};
	envelope.Size = static_cast<uint16_t>(VersionedCurrentCodec::RecordSize);
	if (VersionedCurrentCodec::Encode(expected, envelope.Bytes, VersionedCapacity) != CodecResultEnum::Success
		|| !journal.Save(envelope))
	{
		return false;
	}

	const auto loaded = journal.Load();
	VersionedCurrentRecord actual{};
	return loaded.HasSnapshot()
		&& Inertia::Components::Storage::VersionedRecord::Internal::CodecDispatcher<
			VersionedCurrentCodec, VersionedV1Codec>::Decode(loaded.Value.Bytes, loaded.Value.Size, actual)
			== CodecResultEnum::Success
		&& actual.Value == expected.Value && actual.Count == expected.Count;
}

bool TestAsyncVersionedRecordTaskWritesRecord()
{
	EmbeddedEEPROM::EraseEEPROM();
	TS::Scheduler scheduler;
	AsyncVersionedStore store;
	AsyncWriteTask<AsyncVersionedStore> task(scheduler, store);
	VersionedCurrentRecord expected{};
	expected.Value = 789123;
	expected.Count = 31;
	if (!task.RequestWrite(expected)) { return false; }

	while (task.IsWriteActive())
	{
		scheduler.execute();
		if (task.IsWriteActive()) { delay(1); }
	}

	VersionedCurrentRecord actual{};
	return task.GetLastCompletedWriteStatus() == AsyncVersionedStore::AsyncWriteStatus::Complete
		&& store.Read(actual) && actual.Value == expected.Value && actual.Count == expected.Count;
}

bool TestVersionedRecordStoreRoundTrip()
{
	EmbeddedEEPROM::EraseEEPROM();
	VersionedStore store;
	VersionedCurrentRecord expected{};
	expected.Value = 987654;
	expected.Count = 23;
	if (!store.Write(expected)) { return false; }

	VersionedCurrentRecord actual{};
	return store.Read(actual) && actual.Value == expected.Value && actual.Count == expected.Count;
}

bool TestVersionedRecordStoreMigratesLegacyEnvelope()
{
	EmbeddedEEPROM::EraseEEPROM();
	using EnvelopeType = VersionedStore::EnvelopeType;
	StructStore<EnvelopeType, 1, 320> rawStore;
	EnvelopeType envelope{};
	VersionedV1Record legacy{};
	legacy.Value = 456789;
	if (VersionedV1Codec::Encode(legacy, envelope.Bytes, VersionedCapacity) != CodecResultEnum::Success)
	{
		return false;
	}
	envelope.Size = static_cast<uint16_t>(VersionedV1Codec::RecordSize);
	if (!rawStore.Write(envelope)) { return false; }

	VersionedStore store;
	VersionedCurrentRecord actual{};
	return store.Read(actual) && actual.Value == legacy.Value && actual.Count == 0;
}

bool TestAsyncVersionedRecordStoreRoundTrip()
{
	EmbeddedEEPROM::EraseEEPROM();
	AsyncVersionedStore store;
	VersionedCurrentRecord expected{};
	expected.Value = 123456;
	expected.Count = 12;
	if (!store.BeginWrite(expected)) { return false; }

	using AsyncStatus = AsyncVersionedStore::AsyncWriteStatus;
	AsyncStatus status = AsyncStatus::Pending;
	while (status == AsyncStatus::Pending)
	{
		status = store.PollWrite();
		if (status == AsyncStatus::Pending) { delay(1); }
	}

	VersionedCurrentRecord actual{};
	return status == AsyncStatus::Complete && store.Read(actual)
		&& actual.Value == expected.Value && actual.Count == expected.Count;
}

bool TestStoreRoundTrip()
{
	EmbeddedEEPROM::EraseEEPROM();
	Store store;
	TestRecord expected{};
	expected.Sequence = 1234;
	expected.Value = 56;
	if (!store.Write(expected)) { return false; }

	TestRecord actual{};
	return store.Read(actual)
		&& actual.Sequence == expected.Sequence
		&& actual.Value == expected.Value;
}

bool TestAsyncWriteTaskRunsThroughScheduler()
{
	EmbeddedEEPROM::EraseEEPROM();
	TS::Scheduler scheduler;
	AsyncStore store;
	AsyncWriteTask<AsyncStore> task(scheduler, store);
	TestRecord expected{};
	expected.Sequence = 2468;
	expected.Value = 135;
	if (!task.RequestWrite(expected) || task.RequestWrite(expected)) { return false; }

	while (task.IsWriteActive())
	{
		scheduler.execute();
		if (task.IsWriteActive()) { delay(1); }
	}

	TestRecord actual{};
	return task.GetCompletedWriteCount() == 1
		&& task.GetLastCompletedWriteStatus() == AsyncStore::AsyncWriteStatus::Complete
		&& store.Read(actual)
		&& actual.Sequence == expected.Sequence
		&& actual.Value == expected.Value;
}

bool TestErasedStoreRejectsReadWithoutChangingOutput()
{
	EmbeddedEEPROM::EraseEEPROM();
	Store store;
	TestRecord actual{};
	actual.Sequence = 111;
	actual.Value = 222;
	return !store.Read(actual)
		&& actual.Sequence == 111
		&& actual.Value == 222;
}

bool TestStoreOverwriteReturnsLatestRecord()
{
	EmbeddedEEPROM::EraseEEPROM();
	Store store;
	TestRecord first{};
	first.Sequence = 1;
	first.Value = 10;
	TestRecord latest{};
	latest.Sequence = 2;
	latest.Value = 20;
	if (!store.Write(first) || !store.Write(latest)) { return false; }

	TestRecord actual{};
	return store.Read(actual)
		&& actual.Sequence == latest.Sequence
		&& actual.Value == latest.Value;
}

bool TestVersionChangeInvalidatesStoredRecord()
{
	EmbeddedEEPROM::EraseEEPROM();
	Store store;
	TestRecord expected{};
	expected.Sequence = 345;
	expected.Value = 67;
	if (!store.Write(expected)) { return false; }

	VersionChangedStore changedVersionStore;
	TestRecord actual{};
	actual.Sequence = 111;
	actual.Value = 222;
	return !changedVersionStore.Read(actual)
		&& actual.Sequence == 111
		&& actual.Value == 222;
}

bool TestCorruptRecordIsRejectedWithoutChangingOutput()
{
	EmbeddedEEPROM::EraseEEPROM();
	Store store;
	TestRecord expected{};
	expected.Sequence = 5678;
	expected.Value = 90;
	if (!store.Write(expected)) { return false; }

	const uint16_t crcAddress = Store::BaseAddress + sizeof(TestRecord);
	EmbeddedEEPROM::WriteBlock(crcAddress, EmbeddedEEPROM::ReadBlock(crcAddress) ^ 1);

	TestRecord actual{};
	actual.Sequence = 111;
	actual.Value = 222;
	return !store.Read(actual)
		&& actual.Sequence == 111
		&& actual.Value == 222;
}

bool TestWearLevelStoreKeepsLatestRecord()
{
	EmbeddedEEPROM::EraseEEPROM();
	TestRecord latest{};
	for (uint32_t sequence = 1; sequence <= 5; sequence++)
	{
		WearLevelStore store;
		latest.Sequence = sequence;
		latest.Value = static_cast<uint16_t>(sequence * 10);
		if (!store.Write(latest)) { return false; }
	}

	WearLevelStore store;
	TestRecord actual{};
	return store.Read(actual)
		&& actual.Sequence == latest.Sequence
		&& actual.Value == latest.Value;
}

bool TestAsyncStoreWriteCompletesThroughPolling()
{
	EmbeddedEEPROM::EraseEEPROM();
	ConcurrentReadStore readStore;
	TestRecord existing{};
	existing.Sequence = 44;
	existing.Value = 55;
	if (!readStore.Write(existing)) { return false; }

	AsyncStore store;
	TestRecord expected{};
	expected.Sequence = 9876;
	expected.Value = 543;
	if (!store.BeginWrite(expected)) { return false; }

	CompetingAsyncStore competingStore;
	if (competingStore.BeginWrite(expected)) { return false; }
	TestRecord actual{};
	if (!readStore.Read(actual) || actual.Sequence != existing.Sequence || actual.Value != existing.Value)
	{
		return false;
	}

	using AsyncStatus = AsyncStore::AsyncWriteStatus;
	AsyncStatus status = AsyncStatus::Pending;
	uint8_t pollCount = 0;
	while (status == AsyncStatus::Pending)
	{
		status = store.PollWrite();
		pollCount++;
		if (status == AsyncStatus::Pending) { delay(1); }
	}

	actual = {};
	return status == AsyncStatus::Complete
		&& pollCount > 1
		&& store.Read(actual)
		&& actual.Sequence == expected.Sequence
		&& actual.Value == expected.Value;
}

bool TestAsyncWearLevelWriteCompletesThroughPolling()
{
	AsyncWearLevelStore store;
	TestRecord expected{};
	expected.Sequence = 7654;
	expected.Value = 321;
	if (!store.BeginWrite(expected)) { return false; }

	AsyncWearLevelStore::AsyncWriteStatus status = AsyncWearLevelStore::AsyncWriteStatus::Pending;
	while (status == AsyncWearLevelStore::AsyncWriteStatus::Pending)
	{
		status = store.PollWrite();
		if (status == AsyncWearLevelStore::AsyncWriteStatus::Pending) { delay(1); }
	}

	TestRecord actual{};
	return status == AsyncWearLevelStore::AsyncWriteStatus::Complete
		&& store.Read(actual)
		&& actual.Sequence == expected.Sequence
		&& actual.Value == expected.Value;
}

void PrintTableBorder()
{
	Serial.print('+');
	for (uint8_t i = 0; i < StoreColumnWidth + 2; i++) { Serial.print('-'); }
	Serial.print('+');
	for (uint8_t i = 0; i < MeasurementColumnWidth + 2; i++) { Serial.print('-'); }
	Serial.print('+');
	for (uint8_t i = 0; i < NumericGroupWidth + 2; i++) { Serial.print('-'); }
	Serial.print('+');
	for (uint8_t i = 0; i < FailureColumnWidth + 2; i++) { Serial.print('-'); }
	Serial.println('+');
}

void PrintTextCell(const __FlashStringHelper* text, const uint8_t width)
{
	Serial.print(F("| "));
	Serial.print(text);
	const size_t textLength = strlen_P(reinterpret_cast<PGM_P>(text));
	for (size_t i = textLength; i < width; i++) { Serial.print(' '); }
	Serial.print(' ');
}

void PrintPaddedNumber(const uint32_t value, const uint8_t width)
{
	uint32_t remaining = value;
	uint8_t digits = 1;
	while (remaining >= 10)
	{
		remaining /= 10;
		digits++;
	}
	for (uint8_t i = digits; i < width; i++) { Serial.print(' '); }
	Serial.print(value);
}

void PrintTimingHeader()
{
	PrintTableBorder();
	PrintTextCell(F("Store"), StoreColumnWidth);
	PrintTextCell(F("Measurement"), MeasurementColumnWidth);
	PrintTextCell(F("Min / Median / Max (us)"), NumericGroupWidth);
	PrintTextCell(F("Fail"), FailureColumnWidth);
	Serial.println('|');
	PrintTableBorder();
}

void PrintTimingRow(const __FlashStringHelper* storeName, const __FlashStringHelper* measurement, const uint32_t minimum,
	const uint32_t median, const uint32_t maximum, const uint8_t failures)
{
	PrintTextCell(storeName, StoreColumnWidth);
	PrintTextCell(measurement, MeasurementColumnWidth);
	Serial.print(F("| "));
	PrintPaddedNumber(minimum, NumericColumnWidth);
	Serial.print(F(" / "));
	PrintPaddedNumber(median, NumericColumnWidth);
	Serial.print(F(" / "));
	PrintPaddedNumber(maximum, NumericColumnWidth);
	Serial.print(' ');
	Serial.print(F("| "));
	PrintPaddedNumber(failures, FailureColumnWidth);
	Serial.print(' ');
	Serial.println('|');
}

void PrintTimingSummary(const __FlashStringHelper* storeName, const __FlashStringHelper* operationName, uint32_t* samples,
	const uint8_t sampleCount, const uint8_t failures, const bool countFailures = true)
{
	for (uint8_t i = 1; i < sampleCount; i++)
	{
		const uint32_t sample = samples[i];
		uint8_t j = i;
		while (j > 0 && samples[j - 1] > sample)
		{
			samples[j] = samples[j - 1];
			j--;
		}
		samples[j] = sample;
	}

	const uint32_t typical = (samples[(sampleCount - 1) / 2] + samples[sampleCount / 2]) / 2;
	PrintTimingRow(storeName, operationName, samples[0], typical, samples[sampleCount - 1], failures);
	if (countFailures) { FailedChecks += failures; }
}

template<typename TStore>
void BenchmarkStore(const __FlashStringHelper* name, TStore& store)
{
	uint32_t writeSamples[BenchmarkSamples]{};
	uint32_t readSamples[BenchmarkSamples]{};
	uint8_t writeFailures = 0;
	uint8_t readFailures = 0;
	TestRecord expected{};

	for (uint8_t i = 0; i < BenchmarkSamples; i++)
	{
		expected.Sequence = static_cast<uint32_t>(i) + 1;
		expected.Value = static_cast<uint16_t>(expected.Sequence * 10);
		const uint32_t start = micros();
		const bool succeeded = store.Write(expected);
		writeSamples[i] = micros() - start;
		if (!succeeded) { writeFailures++; }
	}

	for (uint8_t i = 0; i < BenchmarkSamples; i++)
	{
		TestRecord actual{};
		const uint32_t start = micros();
		const bool succeeded = store.Read(actual);
		readSamples[i] = micros() - start;
		if (!succeeded || actual.Sequence != expected.Sequence || actual.Value != expected.Value)
		{
			readFailures++;
		}
	}

	PrintTimingSummary(name, F("Write"), writeSamples, BenchmarkSamples, writeFailures);
	PrintTimingSummary(name, F("Read"), readSamples, BenchmarkSamples, readFailures);
}

template<typename TStore>
void BenchmarkAsyncStore(const __FlashStringHelper* name, TStore& store)
{
	uint32_t beginSamples[BenchmarkSamples]{};
	uint32_t maxPollSamples[BenchmarkSamples]{};
	uint32_t completionSamples[BenchmarkSamples]{};
	uint8_t failures = 0;

	for (uint8_t i = 0; i < BenchmarkSamples; i++)
	{
		TestRecord expected{};
		expected.Sequence = static_cast<uint32_t>(200) + i;
		expected.Value = static_cast<uint16_t>(expected.Sequence * 13);

		const uint32_t operationStart = micros();
		const uint32_t beginStart = micros();
		const bool accepted = store.BeginWrite(expected);
		beginSamples[i] = micros() - beginStart;
		if (!accepted)
		{
			completionSamples[i] = micros() - operationStart;
			failures++;
			continue;
		}

		uint32_t maxPollDuration = 0;
		using AsyncStatus = typename TStore::AsyncWriteStatus;
		AsyncStatus status = AsyncStatus::Pending;
		while (status == AsyncStatus::Pending)
		{
			const uint32_t pollStart = micros();
			status = store.PollWrite();
			const uint32_t pollDuration = micros() - pollStart;
			if (pollDuration > maxPollDuration) { maxPollDuration = pollDuration; }
			if (status == AsyncStatus::Pending) { delay(1); }
		}

		maxPollSamples[i] = maxPollDuration;
		completionSamples[i] = micros() - operationStart;
		TestRecord actual{};
		if (status != AsyncStatus::Complete
			|| !store.Read(actual)
			|| actual.Sequence != expected.Sequence
			|| actual.Value != expected.Value)
		{
			failures++;
		}
	}

	PrintTimingSummary(name, F("Async Begin"), beginSamples, BenchmarkSamples, failures, false);
	PrintTimingSummary(name, F("Async max Poll"), maxPollSamples, BenchmarkSamples, failures, false);
	PrintTimingSummary(name, F("Async total @1ms"), completionSamples, BenchmarkSamples, failures);
}

void BenchmarkStorageOperations()
{
	Serial.println(F("EEPROM timings in microseconds (16 samples; typical is median):"));
	PrintTimingHeader();
	Store store;
	BenchmarkStore(F("StructStore"), store);
	WearLevelStore wearLevelStore;
	BenchmarkStore(F("WearLevelStore"), wearLevelStore);
	AsyncStore asyncStore;
	BenchmarkAsyncStore(F("StructStore"), asyncStore);
	AsyncWearLevelStore asyncWearLevelStore;
	BenchmarkAsyncStore(F("WearLevelStore"), asyncWearLevelStore);
	PrintTableBorder();
}

void setup()
{
	Serial.begin(115200);
	Report(F("CRC-checked EEPROM store reads back written data"), TestStoreRoundTrip());
	Report(F("EEPROM snapshot journal starts empty and saves a snapshot"), TestJournalStartsEmptyAndSavesSnapshot());
	Report(F("EEPROM snapshot journal rotates and recovers the previous valid snapshot"),
		TestJournalRotatesAndRecoversPreviousValidSnapshot());
	Report(F("EEPROM snapshot journal persists a versioned-record envelope"), TestVersionedEnvelopeInEepromJournal());
	Report(F("versioned EEPROM envelope round-trips the current codec"), TestVersionedRecordStoreRoundTrip());
	Report(F("versioned EEPROM envelope migrates a legacy codec"), TestVersionedRecordStoreMigratesLegacyEnvelope());
	Report(F("async versioned EEPROM envelope completes and reads back"), TestAsyncVersionedRecordStoreRoundTrip());
	Report(F("async EEPROM task writes a versioned record"), TestAsyncVersionedRecordTaskWritesRecord());
	Report(F("erased EEPROM rejects reads without changing output"), TestErasedStoreRejectsReadWithoutChangingOutput());
	Report(F("overwriting a record returns the latest data"), TestStoreOverwriteReturnsLatestRecord());
	Report(F("changing the record version invalidates stored data"), TestVersionChangeInvalidatesStoredRecord());
	Report(F("corrupt EEPROM records are rejected without changing output"), TestCorruptRecordIsRejectedWithoutChangingOutput());
	Report(F("wear-level counter survives reconstruction and rollover"), TestWearLevelStoreKeepsLatestRecord());
	Report(F("async EEPROM writes complete through task polling"), TestAsyncStoreWriteCompletesThroughPolling());
	Report(F("async wear-level writes complete through task polling"), TestAsyncWearLevelWriteCompletesThroughPolling());
	Report(F("async EEPROM task drives writes through TaskScheduler"), TestAsyncWriteTaskRunsThroughScheduler());
	BenchmarkStorageOperations();
	if (FailedChecks == 0)
	{
		Serial.println(F("All EEPROM storage checks passed."));
	}
	else
	{
		Serial.print(F("EEPROM storage checks failed: "));
		Serial.println(FailedChecks);
	}
	Serial.flush();
}

void loop()
{}
