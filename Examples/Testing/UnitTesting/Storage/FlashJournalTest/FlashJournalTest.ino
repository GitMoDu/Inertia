
// Using Archipenko's TaskScheduler library for Arduino.
#define _TASK_OO_CALLBACKS
#define _TASK_SLEEP_ON_IDLE_RUN
#include <TScheduler.hpp>

#include <InertiaModel.h>

#include <stdint.h>
#include <string.h>

#include "RamFlash.h"

constexpr uint32_t SerialBaudRate = 115200;

enum { JournalPageCount = 4, JournalQueueCapacity = 4, JournalEntrySize = 17 };
using FlashBackend = RamFlash;
enum { TargetFlashSizeBytes = RamFlash::PageSizeBytes * RamFlash::PageCount };

struct JournalEntry
{
	uint8_t Bytes[JournalEntrySize];
};

using JournalType = Inertia::Components::Storage::FlashJournal::FlashJournal<
	FlashBackend, TargetFlashSizeBytes, JournalPageCount, JournalEntry>;
using AsyncJournalTask = Inertia::Components::Storage::FlashJournal::AsyncFlashJournalTask<
	JournalType, JournalQueueCapacity>;

const uint8_t JournalKey[] = { 'F', 'l', 'a', 's', 'h' };
TS::Scheduler Scheduler{};
JournalType JournalStorage(JournalKey, sizeof(JournalKey));
AsyncJournalTask JournalTask(Scheduler, JournalStorage);

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
	JournalEntry loaded{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
	{
		first.Bytes[index] = static_cast<uint8_t>(index + 1);
		second.Bytes[index] = static_cast<uint8_t>(index + 31);
	}

	uint32_t sequence = 0;
	passed = JournalStorage.Save(first) && passed;
	passed = JournalStorage.Save(second) && passed;
	passed = JournalStorage.Load(loaded, sequence) && sequence == 2
		&& memcmp(loaded.Bytes, second.Bytes, JournalEntrySize) == 0 && passed;

	const uint8_t differentKey[] = { 'O', 't', 'h', 'e', 'r' };
	JournalType mismatchedKey(differentKey, sizeof(differentKey));
	JournalType invalidKey(nullptr, 0);
	passed = !mismatchedKey.Load(loaded, sequence) && passed;
	passed = !invalidKey.Save(first) && !invalidKey.Load(loaded, sequence) && passed;

	const uint32_t secondRecord = JournalType::GetJournalStartAddress() + JournalType::GetRecordSize();
	RamFlash::Corrupt(secondRecord + sizeof(uint32_t), 0);
	passed = JournalStorage.Load(loaded, sequence) && sequence == 1
		&& memcmp(loaded.Bytes, first.Bytes, JournalEntrySize) == 0 && passed;

	RamFlash::Reset();
	passed = JournalStorage.Save(first) && passed;
	RamFlash::FailAfterWrites(3);
	passed = !JournalStorage.Save(second) && passed;
	RamFlash::FailAfterWrites(-1);
	passed = JournalStorage.Load(loaded, sequence) && sequence == 1
		&& memcmp(loaded.Bytes, first.Bytes, JournalEntrySize) == 0 && passed;

	RamFlash::Reset();
	for (uint32_t index = 1; index <= 40; ++index)
	{
		for (size_t byte = 0; byte < JournalEntrySize; ++byte)
			first.Bytes[byte] = static_cast<uint8_t>(index + byte);
		passed = JournalStorage.Save(first) && passed;
	}
	passed = JournalStorage.Load(loaded, sequence) && sequence == 40 && passed;
	Serial.println(passed
		? F("PASS: save/load, bad-key rejection, corruption recovery, interrupted writes, page rotation.")
		: F("FAIL: flash-journal runtime test."));
	return passed;
}

bool RunIncrementalSaveTests()
{
	RamFlash::Reset();
	JournalEntry entry{};
	JournalEntry loaded{};
	for (size_t index = 0; index < JournalEntrySize; ++index)
		entry.Bytes[index] = static_cast<uint8_t>(index + 41);

	JournalType::SaveOperation operation{};
	bool passed = JournalStorage.BeginSave(entry, operation)
		&& operation.CurrentPhase == Inertia::Components::Storage::FlashJournal::SavePhaseEnum::Erase;
	bool completed = false;
	uint16_t steps = 0;
	while (passed && !completed && steps < 32)
	{
		const uint32_t before = RamFlash::GetMutationCount();
		const JournalType::SaveStepResult result = JournalStorage.SaveStep(entry, operation);
		passed = RamFlash::GetMutationCount() - before <= 1 && passed;
		completed = result == JournalType::SaveStepResult::Completed;
		passed = result != JournalType::SaveStepResult::Failed && passed;
		++steps;
	}

	uint32_t sequence = 0;
	passed = completed && steps < 32 && operation.CurrentPhase
		== Inertia::Components::Storage::FlashJournal::SavePhaseEnum::Completed && passed;
	passed = JournalStorage.Load(loaded, sequence) && sequence == 1
		&& memcmp(loaded.Bytes, entry.Bytes, JournalEntrySize) == 0 && passed;
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
	JournalEntry loaded{};
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

	uint32_t sequence = 0;
	passed = JournalTask.GetPendingOperationCount() == 0 && passed;
	passed = JournalTask.GetCompletedOperationCount() - completedBefore == 4 && passed;
	passed = JournalTask.GetLastOperationSucceeded() && passed;
	passed = JournalStorage.Load(loaded, sequence) && sequence == 3
		&& memcmp(loaded.Bytes, third.Bytes, JournalEntrySize) == 0 && passed;
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
	Serial.println(passed
		? F("PASS: async FIFO queue, page erase, and one flash mutation per callback.")
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
	Serial.println(F("FlashJournal setup..."));
#if !defined(FLASH_JOURNAL_USE_INTERNAL_FLASH)
	RamFlash::Reset();
#endif
	Serial.println(F("FlashJournal unit tests: send t to run tests."));
}

void loop()
{
	Scheduler.execute();
	if (Serial.available() && Serial.read() == 't')
	{
		const bool journalPassed = RunJournalTests();
		const bool incrementalSavePassed = RunIncrementalSaveTests();
		const bool queuePassed = RunAsyncQueueTests();
		Serial.println(journalPassed && incrementalSavePassed && queuePassed
			? F("ALL TESTS PASS") : F("TESTS FAILED"));
	}
}
