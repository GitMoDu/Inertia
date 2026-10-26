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
					template<typename TData, uint16_t TVersion>
					struct CrcKey
					{
						static constexpr uint32_t Value = (static_cast<uint32_t>(TVersion) << 16)
							| static_cast<uint32_t>(sizeof(TData));
					};

					template<bool TAsync, typename TSyncUnit, typename TAsyncUnit>
					struct StorageUnitTypeSelector;

					template<typename TSyncUnit, typename TAsyncUnit>
					struct StorageUnitTypeSelector<false, TSyncUnit, TAsyncUnit>
					{
						using Type = TSyncUnit;
					};

					template<typename TSyncUnit, typename TAsyncUnit>
					struct StorageUnitTypeSelector<true, TSyncUnit, TAsyncUnit>
					{
						using Type = TAsyncUnit;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress,
						typename TWearLevelOptionType, TWearLevelOptionType TWearLevelOption, bool TAsync>
					struct StorageUnitSelector;

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, NoWearLevel TWearLevelOption, bool TAsync>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, NoWearLevel, TWearLevelOption, TAsync>
					{
						static_assert(TWearLevelOption == NoWearLevel::x1, "Unsupported EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<TAsync,
							::StorageUnit<TBaseAddress, sizeof(TData), CrcKey<TData, TVersion>::Value>,
							::AsyncStorageUnit<TBaseAddress, sizeof(TData), CrcKey<TData, TVersion>::Value>>::Type;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelTiny TWearLevelOption, bool TAsync>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelTiny, TWearLevelOption, TAsync>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelTiny::x2)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelTiny::x9),
							"Unsupported tiny EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<TAsync,
							::TinyWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>,
							::AsyncTinyWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>>::Type;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelShort TWearLevelOption, bool TAsync>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelShort, TWearLevelOption, TAsync>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelShort::x10)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelShort::x17),
							"Unsupported short EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<TAsync,
							::ShortWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>,
							::AsyncShortWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>>::Type;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelLong TWearLevelOption, bool TAsync>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelLong, TWearLevelOption, TAsync>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelLong::x18)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelLong::x33),
							"Unsupported long EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<TAsync,
							::LongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>,
							::AsyncLongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>>::Type;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelLongLong TWearLevelOption, bool TAsync>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelLongLong, TWearLevelOption, TAsync>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelLongLong::x34)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelLongLong::x65),
							"Unsupported long-long EEPROM wear-level option.");
						using Type = typename StorageUnitTypeSelector<TAsync,
							::LongLongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>,
							::AsyncLongLongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption, CrcKey<TData, TVersion>::Value>>::Type;
					};
				}

				/// <summary>CRC-checked typed storage in AVR EEPROM, optionally using EmbeddedStorage wear leveling.</summary>
				/// <typeparam name="TData">Trivially copyable record type to persist.</typeparam>
				/// <typeparam name="TVersion">Version included in the CRC key; changing it invalidates existing records.</typeparam>
				/// <typeparam name="TBaseAddress">Starting EEPROM address reserved for this store.</typeparam>
				/// <typeparam name="TWearLevelOptionType">EmbeddedStorage wear-level option type.</typeparam>
				/// <typeparam name="TWearLevelOption">Wear-level option, or <c>NoWearLevel::x1</c>.</typeparam>
				template<typename TData, uint16_t TVersion = 1, uint16_t TBaseAddress = 0,
					typename TWearLevelOptionType = NoWearLevel,
					TWearLevelOptionType TWearLevelOption = NoWearLevel::x1, bool TAsync = false>
				class StructStoreBase
				{
				private:
					static_assert(sizeof(TData) <= 65535u, "EEPROM struct size must fit in EmbeddedStorage's uint16_t size.");
				protected:
					using StorageUnitType = typename Internal::StorageUnitSelector<TData, TVersion, TBaseAddress,
						TWearLevelOptionType, TWearLevelOption, TAsync>::Type;
					StorageUnitType Unit{};

				public:
					using DataType = TData;

					static constexpr uint16_t BaseAddress = TBaseAddress;
					static constexpr size_t UsedSize = StorageUnitType::Size();

					static_assert(__is_trivially_copyable(TData),
						"EEPROM struct stores require trivially copyable data.");
					static_assert(StorageUnitType::Address() == TBaseAddress,
						"The EEPROM storage unit must start at its declared base address.");
					static_assert(static_cast<uint32_t>(StorageUnitType::Address()) + StorageUnitType::Size()
						<= EmbeddedEEPROM::Size(), "The EEPROM storage unit exceeds the device capacity.");

					/// <summary>Writes a record synchronously and verifies it by reading it back.</summary>
					/// <param name="data">Record to persist.</param>
					/// <returns>True if the record was written and verified.</returns>
					bool Write(const TData& data)
					{
						if (EmbeddedEEPROM::IsAsyncOperationActive()) { return false; }
						const uint8_t* source = reinterpret_cast<const uint8_t*>(&data);
						Unit.WriteData(source);

						uint8_t verification[sizeof(TData)]{};
						return Unit.ReadData(verification)
							&& memcmp(source, verification, sizeof(TData)) == 0;
					}

					/// <summary>Reads a record only when its CRC is valid; otherwise leaves <paramref name="data"/> unchanged.</summary>
					/// <param name="data">Destination for a valid record.</param>
					/// <returns>True if a valid record was read.</returns>
					bool Read(TData& data)
					{
						uint8_t buffer[sizeof(TData)]{};
						if (!Unit.ReadData(buffer)) { return false; }

						memcpy(&data, buffer, sizeof(TData));
						return true;
					}
				};

				template<typename TData, uint16_t TVersion = 1, uint16_t TBaseAddress = 0,
					typename TWearLevelOptionType = NoWearLevel,
					TWearLevelOptionType TWearLevelOption = NoWearLevel::x1>
				class StructStore : public StructStoreBase<TData, TVersion, TBaseAddress,
					TWearLevelOptionType, TWearLevelOption, false>
				{};

				template<typename TData, uint16_t TVersion = 1, uint16_t TBaseAddress = 0,
					typename TWearLevelOptionType = NoWearLevel,
					TWearLevelOptionType TWearLevelOption = NoWearLevel::x1>
				class AsyncStructStore : public StructStoreBase<TData, TVersion, TBaseAddress,
					TWearLevelOptionType, TWearLevelOption, true>
				{
				private:
					TData AsyncData{};
					EmbeddedEEPROM::AsyncOperationStateEnum AsyncWriteState = EmbeddedEEPROM::AsyncOperationStateEnum::Idle;

				public:
					using AsyncWriteStatus = EmbeddedEEPROM::AsyncOperationStateEnum;

					bool BeginWrite(const TData& data)
					{
						if (AsyncWriteState == AsyncWriteStatus::Pending) { return false; }
						memcpy(&AsyncData, &data, sizeof(TData));
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
							uint8_t verification[sizeof(TData)]{};
							if (!this->Unit.ReadData(verification)
								|| memcmp(&AsyncData, verification, sizeof(TData)) != 0)
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
