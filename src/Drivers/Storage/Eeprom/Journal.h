#ifndef _INERTIA_DRIVERS_STORAGE_EEPROM_JOURNAL_h
#define _INERTIA_DRIVERS_STORAGE_EEPROM_JOURNAL_h

#if defined(ARDUINO_ARCH_AVR) \
    && (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328p__) \
        || defined(__AVR_atmega328p__) || defined(__AVR_ATtiny85__))

#include <EmbeddedStorage.h>
#include <Components/Storage/EepromJournal/Journal.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Eeprom
			{
				using JournalLoadStatusEnum = Inertia::Components::Storage::EepromJournal::LoadStatusEnum;

				struct EmbeddedEepromByteStore
				{
					enum class AsyncByteWriteResult : uint8_t
					{
						Started,
						Unchanged,
						Busy,
						Failed
					};

					static constexpr uint32_t CapacityBytes = EmbeddedEEPROM::Size();

					static uint8_t ReadByte(const uint32_t address)
					{
						return EmbeddedEEPROM::ReadBlock(static_cast<uint16_t>(address));
					}

					static bool WriteByte(const uint32_t address, const uint8_t value)
					{
						if (EmbeddedEEPROM::IsAsyncOperationActive() || address >= CapacityBytes) { return false; }
						EmbeddedEEPROM::WriteBlock(static_cast<uint16_t>(address), value);
						return EmbeddedEEPROM::ReadBlock(static_cast<uint16_t>(address)) == value;
					}

					static bool IsWriteAvailable()
					{
						return !EmbeddedEEPROM::IsAsyncOperationActive();
					}

					static bool TryBeginAsyncWrite()
					{
						return EmbeddedEEPROM::TryBeginAsyncOperation();
					}

					static void EndAsyncWrite()
					{
						EmbeddedEEPROM::EndAsyncOperation();
					}

					static bool IsAsyncWriteBusy()
					{
						return EmbeddedEEPROM::IsAsyncByteBusy();
					}

					static AsyncByteWriteResult TryStartAsyncWriteByte(const uint32_t address, const uint8_t value)
					{
						if (address >= CapacityBytes) { return AsyncByteWriteResult::Failed; }
						switch (EmbeddedEEPROM::TryStartAsyncByte(static_cast<uint16_t>(address), value,
							EmbeddedEEPROM::AsyncWriteModeEnum::Replace))
						{
						case EmbeddedEEPROM::AsyncByteStatusEnum::Started:
							return AsyncByteWriteResult::Started;
						case EmbeddedEEPROM::AsyncByteStatusEnum::Unchanged:
							return AsyncByteWriteResult::Unchanged;
						case EmbeddedEEPROM::AsyncByteStatusEnum::Busy:
							return AsyncByteWriteResult::Busy;
						default:
							return AsyncByteWriteResult::Failed;
						}
					}
				};

				template<typename Entry, uint16_t BaseAddress, uint16_t SlotCount, uint32_t Key = sizeof(Entry)>
				using Journal = Inertia::Components::Storage::EepromJournal::Journal<
					Entry, EmbeddedEepromByteStore, BaseAddress, SlotCount, Key>;

			}
		}
	}
}

#endif
#endif
