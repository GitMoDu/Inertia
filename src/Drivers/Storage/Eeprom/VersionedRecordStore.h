#ifndef _INERTIA_DRIVERS_STORAGE_EEPROM_VERSIONED_RECORD_STORE_h
#define _INERTIA_DRIVERS_STORAGE_EEPROM_VERSIONED_RECORD_STORE_h

#if defined(ARDUINO_ARCH_AVR) \
	&& (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328p__) \
		|| defined(__AVR_atmega328p__) || defined(__AVR_ATtiny85__))

#include "StructStore.h"
#include "AsyncWriteTask.h"
#include <Components/Storage/VersionedRecord/Codec.h>
#include <limits.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Eeprom
			{
				template<typename CurrentCodec, typename Dispatcher, size_t Capacity, bool Async = false,
					uint16_t StorageVersion = 1, uint16_t BaseAddress = 0,
					typename WearLevelOptionType = NoWearLevel,
					WearLevelOptionType WearLevelOption = NoWearLevel::x1>
				class VersionedRecordStore
				{
				public:
					using RecordType = typename CurrentCodec::StorageType;
					using DataType = RecordType;
					using EnvelopeType = Components::Storage::VersionedRecord::Envelope<Capacity>;

				private:
					using StoreType = typename Internal::StorageUnitTypeSelector<Async,
						StructStore<EnvelopeType, StorageVersion, BaseAddress, WearLevelOptionType, WearLevelOption>,
						AsyncStructStore<EnvelopeType, StorageVersion, BaseAddress, WearLevelOptionType, WearLevelOption>>::Type;
					StoreType Store;

					static_assert(Capacity >= CurrentCodec::RecordSize,
						"EEPROM versioned-record capacity must fit the current codec record.");
					static_assert(Capacity <= UINT16_MAX,
						"EEPROM versioned-record capacity must fit the envelope size field.");
					static_assert(__is_trivially_copyable(EnvelopeType),
						"EEPROM versioned-record envelopes must be trivially copyable.");

				public:
					static constexpr uint16_t BaseAddressValue = BaseAddress;
					static constexpr size_t UsedSize = StoreType::UsedSize;
					using AsyncWriteStatus = EmbeddedEEPROM::AsyncOperationStateEnum;

					bool Write(const RecordType& record)
					{
						EnvelopeType envelope{};
						envelope.Size = static_cast<uint16_t>(CurrentCodec::RecordSize);
						if (CurrentCodec::Encode(record, envelope.Bytes, Capacity)
							!= Components::Storage::VersionedRecord::CodecResultEnum::Success)
						{
							return false;
						}
						return Store.Write(envelope);
					}

					bool BeginWrite(const RecordType& record)
					{
						static_assert(Async, "BeginWrite requires an asynchronous EEPROM versioned-record store.");
						EnvelopeType envelope{};
						envelope.Size = static_cast<uint16_t>(CurrentCodec::RecordSize);
						if (CurrentCodec::Encode(record, envelope.Bytes, Capacity)
							!= Components::Storage::VersionedRecord::CodecResultEnum::Success)
						{
							return false;
						}
						return Store.BeginWrite(envelope);
					}

					AsyncWriteStatus PollWrite()
					{
						static_assert(Async, "PollWrite requires an asynchronous EEPROM versioned-record store.");
						return Store.PollWrite();
					}

					bool Read(RecordType& record)
					{
						EnvelopeType envelope{};
						if (!Store.Read(envelope) || envelope.Size == 0 || envelope.Size > Capacity)
						{
							return false;
						}

						return Dispatcher::Decode(envelope.Bytes, envelope.Size, record)
							== Components::Storage::VersionedRecord::CodecResultEnum::Success;
					}

				};
			}
		}
	}
}
#endif

#endif