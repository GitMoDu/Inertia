//#define EEPROM_MOCK_IN_MEMORY

#include <Arduino.h>
#include <string.h>
#include <InertiaDrivers.h>

using namespace Inertia::Drivers::Storage::Eeprom;

struct TestRecord
{
	uint32_t Sequence;
	uint16_t Value;
};

using Store = StructStore<TestRecord, 3, 0>;
using VersionChangedStore = StructStore<TestRecord, 4, 0>;
using WearLevelStore = StructStore<TestRecord, 4, 64, WearLevelTiny, WearLevelTiny::x2>;

constexpr uint8_t BenchmarkSamples = 16;

static_assert(Store::UsedSize == sizeof(TestRecord) + 1,
	"A standard EEPROM record must include one CRC byte.");
static_assert(WearLevelStore::UsedSize == EmbeddedStorage::GetStorageSize(sizeof(TestRecord), WearLevelTiny::x2),
	"The wear-level store must report its allocated EEPROM footprint.");

uint8_t FailedChecks = 0;

void Report(const char* name, const bool passed)
{
	Serial.print(passed ? F("PASS: ") : F("FAIL: "));
	Serial.println(name);
	if (!passed) { FailedChecks++; }
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

void PrintTimingSummary(const char* storeName, const char* operationName, uint32_t* samples,
	const uint8_t sampleCount, const uint8_t failures)
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
	Serial.print(storeName);
	Serial.print(' ');
	Serial.print(operationName);
	Serial.print(F(": min="));
	Serial.print(samples[0]);
	Serial.print(F(" us, max="));
	Serial.print(samples[sampleCount - 1]);
	Serial.print(F(" us, typical(median)="));
	Serial.print(typical);
	Serial.print(F(" us, failures="));
	Serial.println(failures);
	FailedChecks += failures;
}

template<typename TStore>
void BenchmarkStore(const char* name, TStore& store)
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

	PrintTimingSummary(name, "Write", writeSamples, BenchmarkSamples, writeFailures);
	PrintTimingSummary(name, "Read", readSamples, BenchmarkSamples, readFailures);
}

void BenchmarkStorageOperations()
{
	Serial.println(F("EEPROM operation timings (16 samples, microseconds per operation):"));
	Store store;
	BenchmarkStore("StructStore", store);
	WearLevelStore wearLevelStore;
	BenchmarkStore("WearLevelStore", wearLevelStore);
}

void setup()
{
	Serial.begin(115200);
	Report("CRC-checked EEPROM store reads back written data", TestStoreRoundTrip());
	Report("erased EEPROM rejects reads without changing output", TestErasedStoreRejectsReadWithoutChangingOutput());
	Report("overwriting a record returns the latest data", TestStoreOverwriteReturnsLatestRecord());
	Report("changing the record version invalidates stored data", TestVersionChangeInvalidatesStoredRecord());
	Report("corrupt EEPROM records are rejected without changing output", TestCorruptRecordIsRejectedWithoutChangingOutput());
	Report("wear-level counter survives reconstruction and rollover", TestWearLevelStoreKeepsLatestRecord());
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
