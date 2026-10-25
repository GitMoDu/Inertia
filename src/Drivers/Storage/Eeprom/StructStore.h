#ifndef _INERTIA_DRIVERS_STORAGE_EEPROM_STRUCT_STORE_h
#define _INERTIA_DRIVERS_STORAGE_EEPROM_STRUCT_STORE_h

#if defined(ARDUINO_ARCH_AVR) \
	&& (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328p__) \
		|| defined(__AVR_atmega328p__) || defined(__AVR_ATtiny85__))

#include <StorageUnit.h>
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

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress,
						typename TWearLevelOptionType, TWearLevelOptionType TWearLevelOption>
					struct StorageUnitSelector;

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, NoWearLevel TWearLevelOption>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, NoWearLevel, TWearLevelOption>
					{
						static_assert(TWearLevelOption == NoWearLevel::x1, "Unsupported EEPROM wear-level option.");
						using Type = ::StorageUnit<TBaseAddress, sizeof(TData), CrcKey<TData, TVersion>::Value>;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelTiny TWearLevelOption>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelTiny, TWearLevelOption>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelTiny::x2)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelTiny::x9),
							"Unsupported tiny EEPROM wear-level option.");
						using Type = ::TinyWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption,
							CrcKey<TData, TVersion>::Value>;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelShort TWearLevelOption>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelShort, TWearLevelOption>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelShort::x10)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelShort::x17),
							"Unsupported short EEPROM wear-level option.");
						using Type = ::ShortWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption,
							CrcKey<TData, TVersion>::Value>;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelLong TWearLevelOption>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelLong, TWearLevelOption>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelLong::x18)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelLong::x33),
							"Unsupported long EEPROM wear-level option.");
						using Type = ::LongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption,
							CrcKey<TData, TVersion>::Value>;
					};

					template<typename TData, uint16_t TVersion, uint16_t TBaseAddress, WearLevelLongLong TWearLevelOption>
					struct StorageUnitSelector<TData, TVersion, TBaseAddress, WearLevelLongLong, TWearLevelOption>
					{
						static_assert(static_cast<uint8_t>(TWearLevelOption) >= static_cast<uint8_t>(WearLevelLongLong::x34)
							&& static_cast<uint8_t>(TWearLevelOption) <= static_cast<uint8_t>(WearLevelLongLong::x65),
							"Unsupported long-long EEPROM wear-level option.");
						using Type = ::LongLongWearLevelUnit<TBaseAddress, sizeof(TData), TWearLevelOption,
							CrcKey<TData, TVersion>::Value>;
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
					TWearLevelOptionType TWearLevelOption = NoWearLevel::x1>
				class StructStore
				{
				private:
					static_assert(sizeof(TData) <= 65535u, "EEPROM struct size must fit in EmbeddedStorage's uint16_t size.");
					using StorageUnitType = typename Internal::StorageUnitSelector<TData, TVersion, TBaseAddress,
						TWearLevelOptionType, TWearLevelOption>::Type;

					StorageUnitType Unit{};

				public:
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
			}
		}
	}
}

#endif
#endif
