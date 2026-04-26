#ifndef _INERTIA_COMPONENTS_STORAGE_FRAM_STRUCT_STORE_h
#define _INERTIA_COMPONENTS_STORAGE_FRAM_STRUCT_STORE_h

#include "Model.h"

#include <Fletcher16.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace Fram
			{
				template<typename TData, uint16_t TVersion = 1, uint16_t TBaseAddress = 0>
				class StructStore
				{
				private:
					static constexpr size_t   VersionOffset = 0;
					static constexpr size_t   CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t   DataOffset = CrcOffset + sizeof(uint16_t);
					static constexpr size_t   StoreSize = DataOffset + sizeof(TData);

					static constexpr uint32_t ChecksumSeed =
						(static_cast<uint32_t>(TVersion) << 16) ^ static_cast<uint32_t>(sizeof(TData)) ^ 3862492193;

				public:
					static constexpr uint16_t BaseAddress = TBaseAddress;
					static constexpr size_t UsedSize = StoreSize;

                private:
					IFramDriver& Driver;
					Fletcher16 Hasher{};

				public:
                    StructStore(IFramDriver& driver)
						: Driver(driver)
					{}

					bool Write(const TData& data)
					{
						uint8_t buf[StoreSize]{};
						WriteUint16(buf + VersionOffset, TVersion);
						WriteUint16(buf + CrcOffset, ComputeCrc(data));
						memcpy(buf + DataOffset, &data, sizeof(TData));

						return Driver.Write(BaseAddress, buf, StoreSize);
					}

					bool Read(TData& data)
					{
						uint8_t buf[StoreSize]{};
						if (!Driver.Read(BaseAddress, buf, StoreSize)) { return false; }

						if (ReadUint16(buf + VersionOffset) != TVersion) { return false; }

						TData candidate{};
						memcpy(&candidate, buf + DataOffset, sizeof(TData));

						if (ReadUint16(buf + CrcOffset) != ComputeCrc(candidate)) { return false; }

						data = candidate;
						return true;
					}

					static constexpr size_t GetStoreSize() { return UsedSize; }

				private:
					uint16_t ComputeCrc(const TData& data)
					{
						uint8_t seed[sizeof(uint32_t)]{};
						WriteUint32(seed, ChecksumSeed);

						Hasher.begin();
						Hasher.add(seed, sizeof(seed));
						Hasher.add(reinterpret_cast<const uint8_t*>(&data), sizeof(TData));
						return Hasher.getFletcher();
					}

					static void WriteUint16(uint8_t* dst, const uint16_t v)
					{
						dst[0] = static_cast<uint8_t>(v & 0xFFu);
						dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
					}

					static void WriteUint32(uint8_t* dst, const uint32_t v)
					{
						dst[0] = static_cast<uint8_t>(v & 0xFFu);
						dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
						dst[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
						dst[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
					}

					static uint16_t ReadUint16(const uint8_t* src)
					{
						return static_cast<uint16_t>(src[0])
							| static_cast<uint16_t>(static_cast<uint16_t>(src[1]) << 8);
					}
				};
			}
		}
	}
}

#endif