#ifndef _INERTIA_DRIVERS_STORAGE_EEPROM_STRUCT_STORE_h
#define _INERTIA_DRIVERS_STORAGE_EEPROM_STRUCT_STORE_h

#if defined(ARDUINO_ARCH_AVR) \
	&& (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328p__) \
		|| defined(__AVR_atmega328p__) || defined(__AVR_ATtiny85__))

#include <StorageUnit.h>
#include <AsyncStorageUnit.h>
#include <WearLevelUnit.h>

#include <string.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Eeprom
			{
				namespace Internal
				{
					template<typename DataType, uint16_t Version>
					struct CrcKey
					{
						static constexpr uint32_t Value = (static_cast<uint32_t>(Version) << 16)
							| static_cast<uint32_t>(sizeof(DataType));
					};

					template<bool Async, typename SyncUnitType, typename AsyncUnitType>
					struct StorageUnitTypeSelector;

					template<typename SyncUnitType, typename AsyncUnitType>
					struct StorageUnitTypeSelector<false, SyncUnitType, AsyncUnitType>
					{
						using Type = SyncUnitType;
					};

					template<typename SyncUnitType, typename AsyncUnitType>
					struct StorageUnitTypeSelector<true, SyncUnitType, AsyncUnitType>
					{
						using Type = AsyncUnitType;
					};

					template<typename DataType, uint16_t Version, uint16_t BaseAddress,
						typename WearLevelOptionType, WearLevelOptionType WearLevelOption, bool Async>
					struct StorageUnitSelector;

					template<typename DataType, uint16_t Version, uint16_t BaseAddress, NoWearLevel WearLevelOption, bool Async>
					struct StorageUnitSelector<DataType, Version, BaseAddress, NoWearLevel, WearLevelOption, Async>
					{
						static_assert(WearLevelOption == NoWearLevel::x1, "Unsupported EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<Async,
							::StorageUnit<BaseAddress, sizeof(DataType), CrcKey<DataType, Version>::Value>,
							::AsyncStorageUnit<BaseAddress, sizeof(DataType), CrcKey<DataType, Version>::Value>>::Type;
					};

					template<typename DataType, uint16_t Version, uint16_t BaseAddress, WearLevelTiny WearLevelOption, bool Async>
					struct StorageUnitSelector<DataType, Version, BaseAddress, WearLevelTiny, WearLevelOption, Async>
					{
						static_assert(static_cast<uint8_t>(WearLevelOption) >= static_cast<uint8_t>(WearLevelTiny::x2)
							&& static_cast<uint8_t>(WearLevelOption) <= static_cast<uint8_t>(WearLevelTiny::x9),
							"Unsupported tiny EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<Async,
							::TinyWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>,
							::AsyncTinyWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>>::Type;
					};

					template<typename DataType, uint16_t Version, uint16_t BaseAddress, WearLevelShort WearLevelOption, bool Async>
					struct StorageUnitSelector<DataType, Version, BaseAddress, WearLevelShort, WearLevelOption, Async>
					{
						static_assert(static_cast<uint8_t>(WearLevelOption) >= static_cast<uint8_t>(WearLevelShort::x10)
							&& static_cast<uint8_t>(WearLevelOption) <= static_cast<uint8_t>(WearLevelShort::x17),
							"Unsupported short EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<Async,
							::ShortWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>,
							::AsyncShortWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>>::Type;
					};

					template<typename DataType, uint16_t Version, uint16_t BaseAddress, WearLevelLong WearLevelOption, bool Async>
					struct StorageUnitSelector<DataType, Version, BaseAddress, WearLevelLong, WearLevelOption, Async>
					{
						static_assert(static_cast<uint8_t>(WearLevelOption) >= static_cast<uint8_t>(WearLevelLong::x18)
							&& static_cast<uint8_t>(WearLevelOption) <= static_cast<uint8_t>(WearLevelLong::x33),
							"Unsupported long EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<Async,
							::LongWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>,
							::AsyncLongWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>>::Type;
					};

					template<typename DataType, uint16_t Version, uint16_t BaseAddress, WearLevelLongLong WearLevelOption, bool Async>
					struct StorageUnitSelector<DataType, Version, BaseAddress, WearLevelLongLong, WearLevelOption, Async>
					{
						static_assert(static_cast<uint8_t>(WearLevelOption) >= static_cast<uint8_t>(WearLevelLongLong::x34)
							&& static_cast<uint8_t>(WearLevelOption) <= static_cast<uint8_t>(WearLevelLongLong::x65),
							"Unsupported long-long EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<Async,
							::LongLongWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>,
							::AsyncLongLongWearLevelUnit<BaseAddress, sizeof(DataType), WearLevelOption, CrcKey<DataType, Version>::Value>>::Type;
					};
				}

				/// <summary>CRC-checked typed storage in AVR EEPROM, optionally using EmbeddedStorage wear leveling.</summary>
				/// <typeparam name="data_type">Trivially copyable record type to persist.</typeparam>
				/// <typeparam name="Version">Version included in the CRC key; changing it invalidates existing records.</typeparam>
				/// <typeparam name="base_address">Starting EEPROM address reserved for this store.</typeparam>
				/// <typeparam name="WearLevelOptionType">EmbeddedStorage wear-level option type.</typeparam>
				/// <typeparam name="WearLevelOption">Wear-level option, or <c>NoWearLevel::x1</c>.</typeparam>
				template<typename data_type, uint16_t Version = 1, uint16_t base_address = 0,
					typename WearLevelOptionType = NoWearLevel,
					WearLevelOptionType WearLevelOption = NoWearLevel::x1, bool Async = false>
				class StructStoreBase
				{
				private:
					static_assert(sizeof(data_type) <= 65535u, "EEPROM struct size must fit in EmbeddedStorage's uint16_t size.");
				protected:
					using StorageUnitType = typename Internal::StorageUnitSelector<data_type, Version, base_address,
						WearLevelOptionType, WearLevelOption, Async>::Type;
					StorageUnitType Unit{};

				public:
					using DataType = data_type;

					static constexpr uint16_t BaseAddress = base_address;
					static constexpr size_t UsedSize = StorageUnitType::Size();

					static_assert(__is_trivially_copyable(data_type),
						"EEPROM struct stores require trivially copyable data.");
					static_assert(StorageUnitType::Address() == base_address,
						"The EEPROM storage unit must start at its declared base address.");
					static_assert(static_cast<uint32_t>(StorageUnitType::Address()) + StorageUnitType::Size()
						<= EmbeddedEEPROM::Size(), "The EEPROM storage unit exceeds the device capacity.");

					/// <summary>Writes a record synchronously and verifies it by reading it back.</summary>
					/// <param name="data">Record to persist.</param>
					/// <returns>True if the record was written and verified.</returns>
					bool Write(const data_type& data)
					{
						if (EmbeddedEEPROM::IsAsyncOperationActive()) { return false; }
						const uint8_t* source = reinterpret_cast<const uint8_t*>(&data);
						Unit.WriteData(source);

						uint8_t verification[sizeof(data_type)]{};
						return Unit.ReadData(verification)
							&& memcmp(source, verification, sizeof(data_type)) == 0;
					}

					/// <summary>Reads a record only when its CRC is valid; otherwise leaves <paramref name="data"/> unchanged.</summary>
					/// <param name="data">Destination for a valid record.</param>
					/// <returns>True if a valid record was read.</returns>
					bool Read(data_type& data)
					{
						uint8_t buffer[sizeof(data_type)]{};
						if (!Unit.ReadData(buffer)) { return false; }

						memcpy(&data, buffer, sizeof(data_type));
						return true;
					}
				};

				template<typename DataType, uint16_t Version = 1, uint16_t BaseAddress = 0,
					typename WearLevelOptionType = NoWearLevel,
					WearLevelOptionType WearLevelOption = NoWearLevel::x1>
				class StructStore : public StructStoreBase<DataType, Version, BaseAddress,
					WearLevelOptionType, WearLevelOption, false>
				{};

				template<typename DataType, uint16_t Version = 1, uint16_t BaseAddress = 0,
					typename WearLevelOptionType = NoWearLevel,
					WearLevelOptionType WearLevelOption = NoWearLevel::x1>
				class AsyncStructStore : public StructStoreBase<DataType, Version, BaseAddress,
					WearLevelOptionType, WearLevelOption, true>
				{
				private:
					DataType AsyncData{};
					EmbeddedEEPROM::AsyncOperationStateEnum AsyncWriteState = EmbeddedEEPROM::AsyncOperationStateEnum::Idle;

				public:
					using AsyncWriteStatus = EmbeddedEEPROM::AsyncOperationStateEnum;

					bool BeginWrite(const DataType& data)
					{
						if (AsyncWriteState == AsyncWriteStatus::Pending) { return false; }
						memcpy(&AsyncData, &data, sizeof(DataType));
						if (!this->Unit.BeginWriteData(reinterpret_cast<const uint8_t*>(&AsyncData))) { return false; }
						AsyncWriteState = AsyncWriteStatus::Pending;
						return true;
					}

					AsyncWriteStatus PollWrite()
					{
						if (AsyncWriteState != AsyncWriteStatus::Pending) { return AsyncWriteState; }
						AsyncWriteState = this->Unit.PollWriteData();
						if (AsyncWriteState == AsyncWriteStatus::Complete)
						{
							uint8_t verification[sizeof(DataType)]{};
							if (!this->Unit.ReadData(verification)
								|| memcmp(&AsyncData, verification, sizeof(DataType)) != 0)
							{
								AsyncWriteState = AsyncWriteStatus::Failed;
							}
						}
						return AsyncWriteState;
					}
				};
			}
		}
	}
}

#endif
#endif
