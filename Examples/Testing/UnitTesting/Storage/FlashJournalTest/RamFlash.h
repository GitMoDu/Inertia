#pragma once

#include <stddef.h>
#include <stdint.h>

class RamFlash
{
public:
	enum
	{
#if defined(__AVR__)
		PageSizeBytes = 128,
#else
		PageSizeBytes = 1024,
#endif
		PageCount = 4,
		WordCount = (PageSizeBytes * PageCount) / 2
	};

	static constexpr uint32_t GetFlashBaseAddress()
	{
		return 0x08000000UL;
	}

	static bool IsStorageRegionAvailable(const uint32_t startAddress, const uint32_t endAddress)
	{
		return StorageAvailable()
			&& startAddress >= GetFlashBaseAddress()
			&& endAddress > startAddress
			&& endAddress <= GetFlashBaseAddress() + PageSizeBytes * PageCount
			&& (startAddress - GetFlashBaseAddress()) % PageSizeBytes == 0
			&& (endAddress - startAddress) % PageSizeBytes == 0;
	}

	static uint16_t ReadHalfWord(const uint32_t address)
	{
		if (!IsAddressValid(address))
			return 0;
		return Words()[(address - GetFlashBaseAddress()) / 2];
	}

	static bool ProgramHalfWord(const uint32_t address, const uint16_t value)
	{
		++ProgramOperationCount();
		if (!IsAddressValid(address) || WritesUntilFailure() == 0)
			return false;
		if (WritesUntilFailure() > 0)
			--WritesUntilFailure();

		uint16_t& word = Words()[(address - GetFlashBaseAddress()) / 2];
		if ((word & value) != value)
			return false;
		word &= value;
		return true;
	}

	static bool ErasePage(const uint32_t address)
	{
		++EraseOperationCount();
		if (address < GetFlashBaseAddress()
			|| (address - GetFlashBaseAddress()) % PageSizeBytes != 0
			|| address + PageSizeBytes > GetFlashBaseAddress() + PageSizeBytes * PageCount
			|| ErasesUntilFailure() == 0)
			return false;
		if (ErasesUntilFailure() > 0)
			--ErasesUntilFailure();

		const size_t firstWord = (address - GetFlashBaseAddress()) / 2;
		for (size_t index = 0; index < PageSizeBytes / 2; ++index)
			Words()[firstWord + index] = 0xFFFF;
		++PageEraseCount()[(address - GetFlashBaseAddress()) / PageSizeBytes];
		return true;
	}

	static void Reset()
	{
		for (size_t index = 0; index < WordCount; ++index)
			Words()[index] = 0xFFFF;
		WritesUntilFailure() = -1;
		ErasesUntilFailure() = -1;
		StorageAvailable() = true;
		ProgramOperationCount() = 0;
		EraseOperationCount() = 0;
		for (size_t index = 0; index < PageCount; ++index)
			PageEraseCount()[index] = 0;
	}

	static uint32_t GetMutationCount()
	{
		return ProgramOperationCount() + EraseOperationCount();
	}

	static void FailAfterWrites(const int32_t writes)
	{
		WritesUntilFailure() = writes;
	}

	static void FailAfterErases(const int32_t erases)
	{
		ErasesUntilFailure() = erases;
	}

	static uint32_t GetPageEraseCount(const uint16_t pageIndex)
	{
		return pageIndex < PageCount ? PageEraseCount()[pageIndex] : 0;
	}

	static void SetStorageAvailable(const bool available)
	{
		StorageAvailable() = available;
	}

	static void Corrupt(const uint32_t address, const uint16_t value)
	{
		if (IsAddressValid(address))
			Words()[(address - GetFlashBaseAddress()) / 2] = value;
	}

	static int32_t& ErasesUntilFailure()
	{
		static int32_t erases = -1;
		return erases;
	}

	static uint32_t* PageEraseCount()
	{
		static uint32_t counts[PageCount] = {};
		return counts;
	}

	static bool& StorageAvailable()
	{
		static bool available = true;
		return available;
	}

private:
	static uint16_t* Words()
	{
		static uint16_t words[WordCount];
		return words;
	}

	static int32_t& WritesUntilFailure()
	{
		static int32_t writes = -1;
		return writes;
	}

	static uint32_t& ProgramOperationCount()
	{
		static uint32_t count = 0;
		return count;
	}

	static uint32_t& EraseOperationCount()
	{
		static uint32_t count = 0;
		return count;
	}

	static bool IsAddressValid(const uint32_t address)
	{
		return address >= GetFlashBaseAddress()
			&& (address & 1) == 0
			&& address + 2 <= GetFlashBaseAddress() + PageSizeBytes * PageCount;
	}
};
