#ifndef _INERTIA_EEPROM_JOURNAL_FAILURE_BYTE_STORE_h
#define _INERTIA_EEPROM_JOURNAL_FAILURE_BYTE_STORE_h

#include <stdint.h>
#include <string.h>

struct FailingJournalByteStore
{
	enum class AsyncByteWriteResult : uint8_t { Started, Unchanged, Busy, Failed };
	static constexpr uint32_t CapacityBytes = 64;
	static uint8_t Bytes[CapacityBytes];
	static bool OperationActive;
	static int16_t WritesBeforeFailure;

	static void Reset()
	{
		memset(Bytes, UINT8_MAX, sizeof(Bytes));
		OperationActive = false;
		WritesBeforeFailure = -1;
	}

	static uint8_t ReadByte(const uint32_t address) { return Bytes[address]; }

	static bool WriteByte(const uint32_t address, const uint8_t value)
	{
		if (OperationActive || address >= CapacityBytes) { return false; }
		Bytes[address] = value;
		return ReadByte(address) == value;
	}

	static bool IsWriteAvailable() { return !OperationActive; }

	static bool TryBeginAsyncWrite()
	{
		if (OperationActive) { return false; }
		OperationActive = true;
		return true;
	}

	static void EndAsyncWrite() { OperationActive = false; }
	static bool IsAsyncWriteBusy() { return !OperationActive; }

	static AsyncByteWriteResult TryStartAsyncWriteByte(const uint32_t address, const uint8_t value)
	{
		if (address >= CapacityBytes) { return AsyncByteWriteResult::Failed; }
		if (!OperationActive) { return AsyncByteWriteResult::Busy; }
		if (WritesBeforeFailure == 0) { return AsyncByteWriteResult::Failed; }
		if (Bytes[address] == value) { return AsyncByteWriteResult::Unchanged; }
		Bytes[address] = value;
		if (WritesBeforeFailure > 0) { --WritesBeforeFailure; }
		return AsyncByteWriteResult::Started;
	}
};

#endif
