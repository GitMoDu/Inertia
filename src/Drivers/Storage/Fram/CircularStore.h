#ifndef _INERTIA_DRIVERS_STORAGE_FRAM_CIRCULAR_STORE_h
#define _INERTIA_DRIVERS_STORAGE_FRAM_CIRCULAR_STORE_h

#include "../../../Components/Storage/Fram/Model.h"
#include <Fletcher16.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Fram
			{
				using namespace Inertia::Components::Storage::Fram;

				template<typename TRecord, uint32_t TCapacity, uint16_t TVersion = 1, uint16_t TBaseAddress = 0>
				class CircularStore
				{
				private:
					static_assert(TCapacity > 0, "TCapacity must be greater than zero.");

					static constexpr size_t RecordSize = sizeof(TRecord);
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t CapacityOffset = CrcOffset + sizeof(uint16_t);
					static constexpr size_t HeadOffset = CapacityOffset + sizeof(uint32_t);
					static constexpr size_t CountOffset = HeadOffset + sizeof(uint32_t);
					static constexpr size_t HeaderSize = CountOffset + sizeof(uint32_t);
					static constexpr size_t HeaderPayloadSize = sizeof(uint16_t) + sizeof(uint32_t) * 3;

					static constexpr uint32_t ChecksumSeed =
						(static_cast<uint32_t>(TVersion) << 16) ^ TCapacity ^ 1898275824;

					static constexpr size_t   StoreSize = HeaderSize + static_cast<size_t>(TCapacity) * RecordSize;

				public:
					static constexpr uint16_t BaseAddress = TBaseAddress;
					static constexpr size_t UsedSize = StoreSize;

				private:
					IFramDriver& Driver;
					Fletcher16 Hasher{};
					uint32_t HeadIndex = 0;
					uint32_t Count = 0;
					bool Started = false;

             public:
					CircularStore(IFramDriver& driver)
						: Driver(driver)
					{}

					bool Start()
					{
						if (Started) { return true; }

						uint16_t version = 0;
						uint32_t capacity = 0;
						uint32_t head = 0;
						uint32_t count = 0;

						if (!TryReadHeader(version, capacity, head, count)
							|| version != TVersion
							|| capacity != TCapacity
							|| count > TCapacity
							|| (count > 0 && head >= TCapacity))
						{
							return ResetStorage();
						}

						HeadIndex = head;
						Count = count;
						Started = true;
						return true;
					}

					void Stop()
					{
						HeadIndex = 0;
						Count = 0;
						Started = false;
					}

					bool Write(const TRecord& record)
					{
						if (!Started) { return false; }

						const uint32_t writeIndex = (Count < TCapacity)
							? (HeadIndex + Count) % TCapacity
							: HeadIndex;
						const uint32_t newHead = (Count < TCapacity)
							? HeadIndex
							: (HeadIndex + 1) % TCapacity;
						const uint32_t newCount = (Count < TCapacity)
							? Count + 1
							: TCapacity;

						if (!WriteRecordAtPhysical(writeIndex, record)) { return false; }
						if (!WriteHeader(newHead, newCount)) { return false; }

						HeadIndex = newHead;
						Count = newCount;
						return true;
					}

					bool Read(const uint32_t logicalIndex, TRecord& record) const
					{
						if (!Started || logicalIndex >= Count) { return false; }
						return ReadRecordAtPhysical(LogicalToPhysical(logicalIndex), record);
					}

					bool TrimFront(uint32_t trimCount)
					{
						if (!Started) { return false; }
						if (trimCount == 0) { return true; }
						if (trimCount > Count) { trimCount = Count; }

						const uint32_t newCount = Count - trimCount;
						const uint32_t newHead = (newCount == 0)
							? 0
							: (HeadIndex + trimCount) % TCapacity;

						if (!WriteHeader(newHead, newCount)) { return false; }

						HeadIndex = newHead;
						Count = newCount;
						return true;
					}

					bool Reset()
					{
						return ResetStorage();
					}

					uint32_t GetCount()    const { return Started ? Count : 0; }
					uint32_t GetCapacity() const { return TCapacity; }
					bool     IsFull()      const { return Started && Count >= TCapacity; }
					bool     IsStarted()   const { return Started; }

					// Total bytes this store occupies in FRAM — useful for packing multiple stores.
                 static constexpr size_t GetStoreSize() { return UsedSize; }

				private:
					uint32_t LogicalToPhysical(const uint32_t logicalIndex) const
					{
						return (HeadIndex + logicalIndex) % TCapacity;
					}

					uint16_t RecordAddress(const uint32_t physicalIndex) const
					{
						return static_cast<uint16_t>(BaseAddress + HeaderSize + physicalIndex * RecordSize);
					}

					bool ResetStorage()
					{
						if (!WriteHeader(0, 0)) { return false; }
						HeadIndex = 0;
						Count = 0;
						Started = true;
						return true;
					}

					bool TryReadHeader(uint16_t& version,
						uint32_t& capacity,
						uint32_t& headIndex,
						uint32_t& count)
					{
						uint8_t buf[HeaderSize]{};
						if (!Driver.Read(BaseAddress, buf, HeaderSize)) { return false; }

						version = ReadUint16(buf + VersionOffset);
						const uint16_t crc = ReadUint16(buf + CrcOffset);
						capacity = ReadUint32(buf + CapacityOffset);
						headIndex = ReadUint32(buf + HeadOffset);
						count = ReadUint32(buf + CountOffset);

						return crc == ComputeHeaderCrc(version, capacity, headIndex, count);
					}

					bool WriteHeader(const uint32_t headIndex, const uint32_t count)
					{
						uint8_t buf[HeaderSize]{};
						WriteUint16(buf + VersionOffset, TVersion);
						WriteUint16(buf + CrcOffset, ComputeHeaderCrc(TVersion, TCapacity, headIndex, count));
						WriteUint32(buf + CapacityOffset, TCapacity);
						WriteUint32(buf + HeadOffset, headIndex);
						WriteUint32(buf + CountOffset, count);

						return Driver.Write(BaseAddress, buf, HeaderSize);
					}

					bool ReadRecordAtPhysical(const uint32_t physicalIndex, TRecord& record) const
					{
						return Driver.Read(
							RecordAddress(physicalIndex),
							reinterpret_cast<uint8_t*>(&record),
							RecordSize);
					}

					bool WriteRecordAtPhysical(const uint32_t physicalIndex, const TRecord& record)
					{
						return Driver.Write(
							RecordAddress(physicalIndex),
							reinterpret_cast<const uint8_t*>(&record),
							RecordSize);
					}

					uint16_t ComputeHeaderCrc(const uint16_t version,
						const uint32_t capacity,
						const uint32_t headIndex,
						const uint32_t count)
					{
						uint8_t seed[sizeof(uint32_t)]{};
						WriteUint32(seed, ChecksumSeed);

						uint8_t payload[HeaderPayloadSize]{};
						WriteUint16(payload, version);
						WriteUint32(payload + sizeof(uint16_t), capacity);
						WriteUint32(payload + sizeof(uint16_t) + sizeof(uint32_t), headIndex);
						WriteUint32(payload + sizeof(uint16_t) + sizeof(uint32_t) * 2, count);

						Hasher.begin();
						Hasher.add(seed, sizeof(seed));
						Hasher.add(payload, sizeof(payload));
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

					static uint32_t ReadUint32(const uint8_t* src)
					{
						return static_cast<uint32_t>(src[0])
							| (static_cast<uint32_t>(src[1]) << 8)
							| (static_cast<uint32_t>(src[2]) << 16)
							| (static_cast<uint32_t>(src[3]) << 24);
					}
				};
			}
		}
	}
}

#endif