
// Using Archipenko's TaskScheduler library for Arduino.
#define _TASK_OO_CALLBACKS
#define _TASK_SLEEP_ON_IDLE_RUN
#include <TScheduler.hpp>

#include <InertiaModel.h>
#include <Components/Storage/FlashJournal/VersionedRecordJournal.h>

#include <stdint.h>
#include <string.h>

#include "RamFlash.h"

constexpr uint32_t SerialBaudRate = 115200;

enum { JournalPageCount = 4, JournalQueueCapacity = 4, JournalEntrySize = 17 };
using FlashBackend = RamFlash;
enum { TargetFlashSizeBytes = RamFlash::PageSizeBytes * RamFlash::PageCount };
constexpr uint32_t JournalSeed = 0x2C5F1414;

struct JournalEntry
{
	uint8_t Bytes[JournalEntrySize];
};

using JournalType = Inertia::Components::Storage::FlashJournal::Journal<
	FlashBackend, TargetFlashSizeBytes, JournalPageCount, JournalEntry>;
using AsyncJournalTask = Inertia::Components::Storage::FlashJournal::AsyncFlashJournalTask<
	JournalType, JournalQueueCapacity>;

using namespace Inertia::Components::Storage::VersionedRecord;

struct JournalVersion1Record
{
	HeaderStruct Header;
	uint32_t Value;
};

struct JournalCurrentRecord
{
	HeaderStruct Header;
	uint32_t Value;
	uint16_t Count;
};

using JournalCurrentCodec = Codec<JournalCurrentRecord, HeaderStruct, 2>;
struct JournalVersion1Codec : Codec<JournalVersion1Record, HeaderStruct, 1>
{
	using NextCodec = JournalCurrentCodec;

	static bool MigrateToNext(const StorageType& source, JournalCurrentCodec::StorageType& destination)
	{
		destination.Value = source.Value;
		destination.Count = 0;
		return true;
	}
};
using JournalMigrationDispatcher = Internal::CodecDispatcher<JournalCurrentCodec, JournalVersion1Codec>;
constexpr size_t JournalVersionedCapacity = Internal::MaximumRecordSize<
	JournalCurrentCodec, JournalVersion1Codec>::Value;
using JournalVersionedEnvelope = Envelope<JournalVersionedCapacity>;
using VersionedJournalType = Inertia::Components::Storage::FlashJournal::Journal<
	FlashBackend, TargetFlashSizeBytes, JournalPageCount, JournalVersionedEnvelope>;
using VersionedJournalAdapter = Inertia::Components::Storage::FlashJournal::VersionedRecordJournal<
	VersionedJournalType, JournalCurrentCodec, JournalMigrationDispatcher, JournalVersionedCapacity>;
using VersionedAsyncTask = Inertia::Components::Storage::FlashJournal::AsyncFlashJournalTask<
	VersionedJournalType, JournalQueueCapacity>;

bool RunVersionedRecordJournalTests()
{
	RamFlash::Reset();
	VersionedJournalType journal(JournalSeed);
	VersionedJournalAdapter storage(journal);
	JournalCurrentRecord expected{};
	expected.Value = 654321;
	expected.Count = 37;
	bool passed = storage.Save(expected);
	auto result = storage.Load();
	passed = result.HasRecord() && result.Record.Value == expected.Value
		&& result.Record.Count == expected.Count && result.Sequence == 1 && passed;

	JournalVersion1Record legacy{};
	legacy.Value = 123456;
	JournalVersionedEnvelope envelope{};
	envelope.Size = static_cast<uint16_t>(JournalVersion1Codec::RecordSize);
	passed = JournalVersion1Codec::Encode(legacy, envelope.Bytes, JournalVersionedCapacity)
		== CodecResultEnum::Success && passed;
	passed = journal.Save(envelope) && passed;
	result = storage.Load();
	passed = result.HasRecord() && result.Record.Value == legacy.Value
		&& result.Record.Count == 0 && result.Sequence == 2 && passed;

	TS::Scheduler versionedScheduler{};
	VersionedAsyncTask asyncTask(versionedScheduler, journal);
	expected.Value = 765432;
	expected.Count = 41;
	passed = storage.QueueSave(asyncTask, expected) && passed;
	uint16_t callbacks = 0;
	while (asyncTask.GetPendingOperationCount() != 0 && callbacks++ < 32)
	{
		versionedScheduler.execute();
		delay(1);
	}
	result = storage.Load();
	passed = asyncTask.GetPendingOperationCount() == 0 && asyncTask.GetLastOperationSucceeded()
		&& result.HasRecord() && result.Record.Value == expected.Value
		&& result.Record.Count == expected.Count && result.Sequence == 3 && passed;
	Serial.println(passed
		? F("PASS: versioned journal current round-trip, legacy migration, and async save.")
		: F("FAIL: versioned journal integration test."));
	return passed;
}

static_assert(JournalType::GetGeometry().StartAddress == JournalType::GetJournalStartAddress(),
	"Flash-journal geometry must be available at compile time.");
TS::Scheduler Scheduler{};
JournalType JournalStorage(JournalSeed);
AsyncJournalTask JournalTask(Scheduler, JournalStorage);

uint32_t GetFixtureChecksum(const uint32_t sequence, const JournalEntry& entry)
{
	Fletcher32 hasher;
	hasher.begin(uint16_t(JournalSeed), uint16_t(JournalSeed >> 16));
	hasher.add(uint16_t(sequence));
	hasher.add(uint16_t(sequence >> 16));
	for (size_t index = 0; index < JournalEntrySize; index += 2)
	{
		uint16_t word = entry.Bytes[index];
		if (index + 1 < JournalEntrySize)
			word |= uint16_t(entry.Bytes[index + 1]) << 8;
		hasher.add(word);
	}
	return hasher.getFletcher();
}

bool ProgramRecordFixture(const uint32_t address, const uint32_t sequence, const JournalEntry& entry)
{
	if (!RamFlash::ProgramHalfWord(address, uint16_t(sequence))
		|| !RamFlash::ProgramHalfWord(address + 2, uint16_t(sequence >> 16)))
		return false;
	for (uint16_t offset = 0; offset < JournalType::GetPaddedPayloadSize(); offset += 2)
	{
		const uint8_t low = offset < JournalEntrySize ? entry.Bytes[offset] : 0xFF;
		const uint8_t high = offset + 1 < JournalEntrySize ? entry.Bytes[offset + 1] : 0xFF;
		if (!RamFlash::ProgramHalfWord(address + 4 + offset, uint16_t(low | (uint16_t(high) << 8))))
			return false;
	}
	const uint32_t checksumAddress = address + 4 + JournalType::GetPaddedPayloadSize();
	const uint32_t checksum = GetFixtureChecksum(sequence, entry);
	return RamFlash::ProgramHalfWord(checksumAddress, uint16_t(checksum))
		&& RamFlash::ProgramHalfWord(checksumAddress + 2, uint16_t(checksum >> 16))
		&& RamFlash::ProgramHalfWord(checksumAddress + 4,
			Inertia::Components::Storage::FlashJournal::CommitMarker);
}

bool RunJournalTests()
{
	bool passed = true;
	RamFlash::Reset();
	passed = JournalType::GetPayloadSize() == sizeof(JournalEntry)
		&& JournalType::GetPaddedPayloadSize() == JournalEntrySize + 1
		&& JournalType::GetSlotsPerPage() == FlashBackend::PageSizeBytes / JournalType::GetRecordSize()
		&& passed;
	const uint32_t erasedPage = JournalType::GetJournalStartAddress() + FlashBackend::PageSizeBytes;
	passed = RamFlash::ProgramHalfWord(erasedPage, 0) && passed;
	JournalEntry first{};
	JournalEntry second{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		first.Bytes[index] = static_cast<uint8_t>(index + 1);
		second.Bytes[index] = static_cast<uint8_t>(index + 31);
	}

	passed = JournalStorage.Save(first) && passed;
	passed = JournalStorage.Save(second) && passed;
	JournalType::LoadResult loadResult = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 2
		&& memcmp(snapshot->Value.Bytes, second.Bytes, JournalEntrySize) == 0 && passed;

	JournalType mismatchedSeed(JournalSeed + 1);
	passed = mismatchedSeed.Load().Status() == JournalType::LoadStatus::NoValidSnapshot && passed;

	const uint32_t secondRecord = JournalType::GetJournalStartAddress() + JournalType::GetRecordSize();
	RamFlash::Corrupt(secondRecord + sizeof(uint32_t), 0);
	loadResult = JournalStorage.Load();
	snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 1
		&& memcmp(snapshot->Value.Bytes, first.Bytes, JournalEntrySize) == 0 && passed;

	RamFlash::Reset();
	passed = JournalStorage.Save(first) && passed;
	RamFlash::FailAfterWrites(3);
	passed = !JournalStorage.Save(second) && passed;
	RamFlash::FailAfterWrites(-1);
	loadResult = JournalStorage.Load();
	snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 1
		&& memcmp(snapshot->Value.Bytes, first.Bytes, JournalEntrySize) == 0 && passed;

	RamFlash::Reset();
	for (uint32_t index = 1; index <= 320; ++index)
	{
		for (size_t byte = 0; byte < JournalEntrySize; ++byte)
			first.Bytes[byte] = static_cast<uint8_t>(index + byte);
		passed = JournalStorage.Save(first) && passed;
	}
	loadResult = JournalStorage.Load();
	snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 320 && passed;
	uint32_t minimumEraseCount = RamFlash::GetPageEraseCount(0);
	uint32_t maximumEraseCount = minimumEraseCount;
	for (uint16_t page = 1; page < JournalPageCount; ++page)
	{
		minimumEraseCount = minimumEraseCount < RamFlash::GetPageEraseCount(page)
			? minimumEraseCount : RamFlash::GetPageEraseCount(page);
		maximumEraseCount = maximumEraseCount > RamFlash::GetPageEraseCount(page)
			? maximumEraseCount : RamFlash::GetPageEraseCount(page);
	}
	passed = maximumEraseCount - minimumEraseCount <= 1 && passed;
	Serial.println(passed
		? F("PASS: save/load, bad-key rejection, corruption recovery, interrupted writes, page rotation.")
		: F("FAIL: flash-journal runtime test."));
	return passed;
}

bool RunLoadAndFormatTests()
{
	RamFlash::Reset();
	bool passed = true;
	const bool erasedLoadPassed = JournalStorage.Load().Status() == JournalType::LoadStatus::NoValidSnapshot;
	passed = erasedLoadPassed && passed;
	RamFlash::SetStorageAvailable(false);
	const bool unavailableLoadPassed = JournalStorage.Load().Status() == JournalType::LoadStatus::StorageUnavailable;
	passed = unavailableLoadPassed && passed;
	RamFlash::SetStorageAvailable(true);

	const uint16_t fixedRecordWords[] =
	{
		0x0001, 0x0000, 0x0201, 0x0403, 0x0605, 0x0807, 0x0A09,
		0x0C0B, 0x0E0D, 0x100F, 0xFF11, 0x5C66, 0x4265, 0xAA55
	};
	const uint32_t recordAddress = JournalType::GetJournalStartAddress();
	for (size_t index = 0; index < sizeof(fixedRecordWords) / sizeof(fixedRecordWords[0]); ++index)
		passed = RamFlash::ProgramHalfWord(recordAddress + uint32_t(index * 2), fixedRecordWords[index]) && passed;
	JournalType::LoadResult result = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = result.GetSnapshot();
	const bool fixedRecordLoadPassed = snapshot != nullptr && snapshot->Sequence == 1;
	passed = fixedRecordLoadPassed && passed;
	bool fixedPayloadPassed = true;
	for (size_t index = 0; index < JournalEntrySize; ++index)
		fixedPayloadPassed = snapshot != nullptr && snapshot->Value.Bytes[index] == index + 1 && fixedPayloadPassed;
	passed = fixedPayloadPassed && passed;

	RamFlash::Corrupt(recordAddress + 22, 0);
	const bool corruptionRejected = JournalStorage.Load().Status() == JournalType::LoadStatus::NoValidSnapshot;
	passed = corruptionRejected && passed;

	RamFlash::Reset();
	JournalEntry older{};
	JournalEntry newer{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		older.Bytes[index] = static_cast<uint8_t>(index + 3);
		newer.Bytes[index] = static_cast<uint8_t>(index + 73);
	}
	passed = ProgramRecordFixture(recordAddress, 0xFFFFFFFEUL, older) && passed;
	passed = ProgramRecordFixture(recordAddress + JournalType::GetRecordSize(), 1, newer) && passed;
	result = JournalStorage.Load();
	snapshot = result.GetSnapshot();
	const bool wrappedSequencePassed = snapshot != nullptr && snapshot->Sequence == 1
		&& memcmp(snapshot->Value.Bytes, newer.Bytes, JournalEntrySize) == 0;
	passed = wrappedSequencePassed && passed;
	JournalEntry temporaryEntry{};
	JournalType temporarySeedJournal(JournalSeed + 2);
	const bool temporarySeedSavePassed = temporarySeedJournal.Save(temporaryEntry);
	const bool temporarySeedLoadPassed = temporarySeedJournal.Load().Status() == JournalType::LoadStatus::Loaded;
	passed = temporarySeedSavePassed && temporarySeedLoadPassed && passed;
	Serial.println(erasedLoadPassed ? F("PASS: erased-region status.") : F("FAIL: erased-region status."));
	Serial.println(unavailableLoadPassed ? F("PASS: unavailable-region status.") : F("FAIL: unavailable-region status."));
	Serial.println(fixedRecordLoadPassed
		? F("PASS: fixed record checksum and sequence.") : F("FAIL: fixed record checksum and sequence."));
	Serial.println(fixedPayloadPassed
		? F("PASS: fixed record payload bytes.") : F("FAIL: fixed record payload bytes."));
	Serial.println(corruptionRejected ? F("PASS: corrupted record rejection.") : F("FAIL: corrupted record rejection."));
	Serial.println(wrappedSequencePassed ? F("PASS: sequence wrap ordering.") : F("FAIL: sequence wrap ordering."));
	Serial.println(temporarySeedSavePassed && temporarySeedLoadPassed
		? F("PASS: independent seed record.") : F("FAIL: independent seed record."));
	Serial.println(passed
		? F("PASS: typed load statuses, known Fletcher32 record bytes, checksum rejection, sequence wrap.")
		: F("FAIL: load and on-flash compatibility test."));
	return passed;
}

bool RunInterruptedWriteTests()
{
	bool passed = true;
	JournalEntry previous{};
	JournalEntry next{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		previous.Bytes[index] = static_cast<uint8_t>(index + 9);
		next.Bytes[index] = static_cast<uint8_t>(index + 91);
	}

	for (int32_t successfulWrites = 0; successfulWrites < 14; ++successfulWrites)
	{
		RamFlash::Reset();
		passed = JournalStorage.Save(previous) && passed;
		RamFlash::FailAfterWrites(successfulWrites);
		passed = !JournalStorage.Save(next) && passed;
		RamFlash::FailAfterWrites(-1);
		JournalType::LoadResult result = JournalStorage.Load();
		const JournalType::Snapshot* snapshot = result.GetSnapshot();
		passed = snapshot != nullptr && snapshot->Sequence == 1
			&& memcmp(snapshot->Value.Bytes, previous.Bytes, JournalEntrySize) == 0 && passed;

		passed = JournalStorage.Save(next) && passed;
		result = JournalStorage.Load();
		snapshot = result.GetSnapshot();
		passed = snapshot != nullptr && snapshot->Sequence == 2
			&& memcmp(snapshot->Value.Bytes, next.Bytes, JournalEntrySize) == 0 && passed;
		const uint32_t expectedSlot = successfulWrites == 0 ? 1 : 2;
		const uint32_t savedAddress = JournalType::GetJournalStartAddress()
			+ expectedSlot * JournalType::GetRecordSize();
		passed = uint32_t(RamFlash::ReadHalfWord(savedAddress)) == 2 && passed;
		if (successfulWrites != 0)
			passed = RamFlash::ReadHalfWord(JournalType::GetJournalStartAddress()
				+ JournalType::GetRecordSize()) != 0xFFFF && passed;
	}

	RamFlash::Reset();
	for (uint32_t sequence = 1; sequence <= JournalType::GetSlotsPerPage(); ++sequence)
		passed = JournalStorage.Save(previous) && passed;
	JournalType::SaveOperation rotation;
	JournalStorage.BeginSave(next, rotation);
	RamFlash::FailAfterErases(0);
	passed = rotation.Step() == JournalType::SaveStatus::Failed
		&& rotation.Error() == JournalType::SaveError::FlashFailure && passed;
	RamFlash::FailAfterErases(-1);
	JournalType::LoadResult result = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = result.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == JournalType::GetSlotsPerPage() && passed;
	passed = JournalStorage.Save(next) && passed;
	passed = RamFlash::GetPageEraseCount(1) == 1 && passed;
	Serial.println(passed
		? F("PASS: programmed-boundary recovery, failed-slot skipping, and rotation erase failure.")
		: F("FAIL: interrupted write boundary test."));
	return passed;
}

bool RunOperationOwnershipTests()
{
	RamFlash::Reset();
	JournalEntry first{};
	JournalEntry second{};
	JournalEntry third{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		first.Bytes[index] = static_cast<uint8_t>(index + 2);
		second.Bytes[index] = static_cast<uint8_t>(index + 42);
		third.Bytes[index] = static_cast<uint8_t>(index + 82);
	}
	const uint8_t expectedThirdFirstByte = third.Bytes[0];
	bool passed = JournalStorage.Save(first);
	JournalType::SaveOperation firstOperation;
	JournalType::SaveOperation secondOperation;
	JournalStorage.BeginSave(second, firstOperation);
	JournalStorage.BeginSave(third, secondOperation);
	second.Bytes[0] = 0xFE;
	third.Bytes[0] = 0xFD;
	passed = firstOperation.Step() == JournalType::SaveStatus::InProgress && passed;
	const uint32_t beforeBlockedStep = RamFlash::GetMutationCount();
	passed = secondOperation.Step() == JournalType::SaveStatus::InProgress && passed;
	passed = RamFlash::GetMutationCount() == beforeBlockedStep && passed;
	passed = !JournalStorage.Save(first) && !JournalStorage.ErasePageForMaintenance(0) && passed;
	uint16_t steps = 0;
	while (firstOperation.Status() == JournalType::SaveStatus::InProgress && steps++ < 32)
		firstOperation.Step();
	steps = 0;
	while (secondOperation.Status() == JournalType::SaveStatus::InProgress && steps++ < 32)
		secondOperation.Step();
	passed = firstOperation.Status() == JournalType::SaveStatus::Completed
		&& secondOperation.Status() == JournalType::SaveStatus::Completed && passed;
	passed = firstOperation.Step() == JournalType::SaveStatus::Completed && passed;
	JournalType::LoadResult result = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = result.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 3
		&& snapshot->Value.Bytes[0] == expectedThirdFirstByte && passed;

	JournalType::SaveOperation abandoned;
	JournalStorage.BeginSave(second, abandoned);
	passed = abandoned.Status() == JournalType::SaveStatus::InProgress && passed;
	passed = abandoned.Step() == JournalType::SaveStatus::InProgress && passed;
	abandoned.Abandon();
	passed = abandoned.Status() == JournalType::SaveStatus::Failed
		&& abandoned.Error() == JournalType::SaveError::Abandoned && passed;
	const uint32_t beforeTerminalStep = RamFlash::GetMutationCount();
	passed = abandoned.Step() == JournalType::SaveStatus::Failed
		&& abandoned.Error() == JournalType::SaveError::Abandoned
		&& RamFlash::GetMutationCount() == beforeTerminalStep && passed;
	result = JournalStorage.Load();
	snapshot = result.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 3 && passed;
	passed = JournalStorage.Save(first) && passed;
	result = JournalStorage.Load();
	snapshot = result.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 4
		&& memcmp(snapshot->Value.Bytes, first.Bytes, JournalEntrySize) == 0 && passed;
	const uint32_t abandonedSlot = JournalType::GetJournalStartAddress()
		+ 3 * JournalType::GetRecordSize();
	passed = RamFlash::ReadHalfWord(abandonedSlot) != 0xFFFF && passed;
	Serial.println(passed
		? F("PASS: operation snapshot ownership, writer serialization, terminal steps, abandonment.")
		: F("FAIL: operation ownership and serialization test."));
	return passed;
}

bool RunJournalCopyOwnershipTests()
{
	RamFlash::Reset();
	JournalType original(JournalSeed);
	JournalType copied(original);
	JournalEntry first{};
	JournalEntry second{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		first.Bytes[index] = static_cast<uint8_t>(index + 12);
		second.Bytes[index] = static_cast<uint8_t>(index + 62);
	}

	JournalType::SaveOperation operation;
	original.BeginSave(first, operation);
	bool passed = operation.Step() == JournalType::SaveStatus::InProgress;
	copied = original;
	uint16_t steps = 0;
	while (operation.Status() == JournalType::SaveStatus::InProgress && steps++ < 32)
		operation.Step();
	passed = operation.Status() == JournalType::SaveStatus::Completed && passed;

	const bool copiedSaveSucceeded = copied.Save(second);
	const bool copiedEraseSucceeded = copied.ErasePageForMaintenance(JournalPageCount - 1);
	JournalType::SaveOperation copiedOperation;
	copied.BeginSave(second, copiedOperation);
	const uint32_t beforeStep = RamFlash::GetMutationCount();
	const JournalType::SaveStatus firstStep = copiedOperation.Step();
	const uint32_t afterFirstStep = RamFlash::GetMutationCount();
	const JournalType::SaveStatus secondStep = copiedOperation.Step();
	const bool incrementalSaveStuck = firstStep == JournalType::SaveStatus::InProgress
		&& secondStep == JournalType::SaveStatus::InProgress
		&& beforeStep == afterFirstStep && afterFirstStep == RamFlash::GetMutationCount();

	passed = copiedSaveSucceeded && copiedEraseSucceeded && !incrementalSaveStuck && passed;
	Serial.println(copiedSaveSucceeded
		? F("PASS: copied journal synchronous save.")
		: F("FAIL: copied journal synchronous save remains blocked."));
	Serial.println(copiedEraseSucceeded
		? F("PASS: copied journal maintenance erase.")
		: F("FAIL: copied journal maintenance erase remains blocked."));
	Serial.println(incrementalSaveStuck
		? F("FAIL: copied journal incremental save remains stuck InProgress.")
		: F("PASS: copied journal incremental save advances."));
	Serial.println(passed
		? F("PASS: journal copy ownership.")
		: F("FAIL: journal copy ownership."));
	return passed;
}

bool RunIncrementalSaveTests()
{
	RamFlash::Reset();
	JournalEntry entry{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
		entry.Bytes[index] = static_cast<uint8_t>(index + 41);

	JournalType::SaveOperation operation;
	JournalStorage.BeginSave(entry, operation);
	const JournalEntry expected = entry;
	entry.Bytes[0] = 0xFE;
	bool passed = operation.Status() == JournalType::SaveStatus::InProgress;
	bool completed = false;
	uint16_t steps = 0;
	while (passed && !completed && steps < 32)
	{
		const uint32_t before = RamFlash::GetMutationCount();
		const JournalType::SaveStatus result = operation.Step();
		passed = RamFlash::GetMutationCount() - before <= 1 && passed;
		completed = result == JournalType::SaveStatus::Completed;
		passed = result != JournalType::SaveStatus::Failed && passed;
		++steps;
	}

	passed = completed && steps < 32 && operation.Status() == JournalType::SaveStatus::Completed && passed;
	JournalType::LoadResult loadResult = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 1
		&& memcmp(snapshot->Value.Bytes, expected.Bytes, JournalEntrySize) == 0 && passed;
	Serial.println(passed
		? F("PASS: incremental save state advances by at most one flash mutation per step.")
		: F("FAIL: incremental flash-journal save test."));
	return passed;
}

bool RunAsyncQueueTests()
{
	if (JournalTask.GetPendingOperationCount() != 0)
		return false;

	RamFlash::Reset();
	const uint32_t erasedPage = JournalType::GetJournalStartAddress() + FlashBackend::PageSizeBytes;
	if (!RamFlash::ProgramHalfWord(erasedPage, 0))
		return false;
	JournalEntry first{};
	JournalEntry second{};
	JournalEntry third{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		first.Bytes[index] = static_cast<uint8_t>(index + 1);
		second.Bytes[index] = static_cast<uint8_t>(index + 11);
		third.Bytes[index] = static_cast<uint8_t>(index + 21);
	}
	const JournalEntry queuedFirst = first;
	bool passed = true;
	const uint32_t completedBefore = JournalTask.GetCompletedOperationCount();
	passed = JournalTask.QueueSave(first) && passed;
	first.Bytes[0] = 0xFF;
	passed = JournalTask.QueuePageErase(1) && passed;
	passed = JournalTask.QueueSave(second) && passed;
	passed = JournalTask.QueueSave(third) && passed;
	passed = !JournalTask.QueueSave(first) && passed;
	passed = JournalTask.GetPendingOperationCount() == JournalQueueCapacity && passed;
	passed = !JournalTask.QueuePageErase(JournalPageCount) && passed;

	uint16_t callbacks = 0;
	while (JournalTask.GetPendingOperationCount() != 0 && callbacks < 128)
	{
		const uint32_t before = RamFlash::GetMutationCount();
		Scheduler.execute();
		const uint32_t mutations = RamFlash::GetMutationCount() - before;
		passed = mutations <= 1 && passed;
		++callbacks;
		delay(1);
	}

	passed = JournalTask.GetPendingOperationCount() == 0 && passed;
	passed = JournalTask.GetCompletedOperationCount() - completedBefore == 4 && passed;
	passed = JournalTask.GetLastOperationSucceeded() && passed;
	JournalType::LoadResult loadResult = JournalStorage.Load();
	const JournalType::Snapshot* snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 3
		&& memcmp(snapshot->Value.Bytes, third.Bytes, JournalEntrySize) == 0 && passed;
	const uint32_t firstRecord = JournalType::GetJournalStartAddress();
	const uint32_t recordSize = JournalType::GetRecordSize();
	const uint32_t firstSequence = RamFlash::ReadHalfWord(firstRecord)
		| (uint32_t(RamFlash::ReadHalfWord(firstRecord + sizeof(uint16_t))) << 16);
	const uint32_t secondSequence = RamFlash::ReadHalfWord(firstRecord + recordSize)
		| (uint32_t(RamFlash::ReadHalfWord(firstRecord + recordSize + sizeof(uint16_t))) << 16);
	passed = firstSequence == 1 && secondSequence == 2 && passed;
	passed = RamFlash::ReadHalfWord(erasedPage) == 0xFFFF && passed;
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		const uint32_t address = firstRecord + sizeof(uint32_t) + uint32_t(index);
		const uint16_t word = RamFlash::ReadHalfWord(address & ~uint32_t(1));
		const uint8_t byte = uint8_t(word >> ((address & 1) * 8));
		passed = byte == queuedFirst.Bytes[index] && passed;
	}
	uint32_t wraparoundCompletedBefore = JournalTask.GetCompletedOperationCount();
	for (uint16_t operationIndex = 0; operationIndex <= JournalQueueCapacity; ++operationIndex)
	{
		JournalEntry wrappedEntry{};
		for (size_t index = 0; index < JournalEntrySize; ++index)
			wrappedEntry.Bytes[index] = static_cast<uint8_t>(operationIndex + index + 111);
		const JournalEntry expected = wrappedEntry;
		passed = JournalTask.QueueSave(wrappedEntry) && passed;
		callbacks = 0;
		while (JournalTask.GetPendingOperationCount() != 0 && callbacks < 128)
		{
			const uint32_t before = RamFlash::GetMutationCount();
			Scheduler.execute();
			passed = RamFlash::GetMutationCount() - before <= 1 && passed;
			++callbacks;
			delay(1);
		}
		passed = JournalTask.GetPendingOperationCount() == 0
			&& JournalTask.GetCompletedOperationCount() == ++wraparoundCompletedBefore
			&& JournalTask.GetLastOperationSucceeded() && passed;
		loadResult = JournalStorage.Load();
		snapshot = loadResult.GetSnapshot();
		passed = snapshot != nullptr && snapshot->Sequence == uint32_t(operationIndex + 4)
			&& memcmp(snapshot->Value.Bytes, expected.Bytes, JournalEntrySize) == 0 && passed;
	}

	RamFlash::Reset();
	passed = JournalStorage.Save(first) && passed;
	RamFlash::FailAfterWrites(0);
	const bool accepted = JournalTask.QueueSave(second);
	callbacks = 0;
	while (JournalTask.GetPendingOperationCount() != 0 && callbacks++ < 16)
	{
		Scheduler.execute();
		delay(1);
	}
	RamFlash::FailAfterWrites(-1);
	passed = accepted && JournalTask.GetPendingOperationCount() == 0
		&& !JournalTask.GetLastOperationSucceeded()
		&& JournalTask.GetLastOperationError() == JournalType::SaveError::FlashFailure && passed;
	loadResult = JournalStorage.Load();
	snapshot = loadResult.GetSnapshot();
	passed = snapshot != nullptr && snapshot->Sequence == 1 && passed;

	RamFlash::Reset();
	passed = RamFlash::ProgramHalfWord(erasedPage, 0) && passed;
	const uint32_t eraseCompletedBefore = JournalTask.GetCompletedOperationCount();
	RamFlash::FailAfterErases(0);
	passed = JournalTask.QueuePageErase(1) && passed;
	callbacks = 0;
	while (JournalTask.GetPendingOperationCount() != 0 && callbacks++ < 16)
	{
		Scheduler.execute();
		delay(1);
	}
	RamFlash::FailAfterErases(-1);
	passed = JournalTask.GetPendingOperationCount() == 0
		&& JournalTask.GetCompletedOperationCount() == eraseCompletedBefore + 1
		&& !JournalTask.GetLastOperationSucceeded()
		&& JournalTask.GetLastOperationError() == JournalType::SaveError::FlashFailure
		&& RamFlash::ReadHalfWord(erasedPage) == 0 && passed;
	Serial.println(passed
		? F("PASS: async FIFO queue, wraparound, page-erase failure, and callback mutation limit.")
		: F("FAIL: asynchronous flash-journal queue test."));
	return passed;
}

void setup()
{
	delay(1000);
	Serial.begin(SerialBaudRate);
	delay(1000);
	while (!Serial)
		;
	delay(1000);
	Serial.println(F("FlashJournal unit tests"));

#if !defined(FLASH_JOURNAL_USE_INTERNAL_FLASH)
	RamFlash::Reset();
#endif
	const bool journalPassed = RunJournalTests();
	const bool incrementalSavePassed = RunIncrementalSaveTests();
	const bool loadAndFormatPassed = RunLoadAndFormatTests();
	const bool interruptedWritePassed = RunInterruptedWriteTests();
	const bool operationOwnershipPassed = RunOperationOwnershipTests();
	const bool journalCopyOwnershipPassed = RunJournalCopyOwnershipTests();
	const bool queuePassed = RunAsyncQueueTests();
	const bool versionedRecordPassed = RunVersionedRecordJournalTests();
	Serial.println(journalPassed && incrementalSavePassed && loadAndFormatPassed
		&& interruptedWritePassed && operationOwnershipPassed && journalCopyOwnershipPassed && queuePassed
		&& versionedRecordPassed
		? F("ALL TESTS PASS") : F("TESTS FAILED"));
}

void loop()
{}
