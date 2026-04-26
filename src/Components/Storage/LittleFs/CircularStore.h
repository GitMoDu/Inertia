#ifndef _INERTIA_COMPONENTS_STORAGE_LITTLEFS_CIRCULAR_STORE_h
#define _INERTIA_COMPONENTS_STORAGE_LITTLEFS_CIRCULAR_STORE_h

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include <LittleFS.h>
#include <Fletcher16.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace LittleFs
			{
				template<typename TRecord, uint32_t TCapacity, uint16_t TVersion = 1>
				class CircularStore
				{
				private:
					static_assert(TCapacity > 0, "TCapacity must be greater than zero.");

					static constexpr size_t   RecordSize = sizeof(TRecord);
					static constexpr uint32_t StoredCapacity = TCapacity;

					// Header layout (little-endian):
					// [0..1]   Version   uint16
					// [2..3]   CRC       uint16
					// [4..7]   Capacity  uint32
					// [8..11]  HeadIndex uint32
					// [12..15] Count     uint32
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t CapacityOffset = CrcOffset + sizeof(uint16_t);
					static constexpr size_t HeadOffset = CapacityOffset + sizeof(uint32_t);
					static constexpr size_t CountOffset = HeadOffset + sizeof(uint32_t);
					static constexpr size_t HeaderSize = CountOffset + sizeof(uint32_t);

					// Payload that gets checksummed: version + capacity + head + count
					static constexpr size_t HeaderPayloadSize = sizeof(uint16_t) + sizeof(uint32_t) * 3;

					// Seed unique per (TVersion, TCapacity) combination with Pi digits.
					static constexpr uint32_t ChecksumSeed =
						(static_cast<uint32_t>(TVersion) << 16) ^ TCapacity ^ 3173993693;

				private:
					const char* Path = nullptr;
					Fletcher16  Hasher{};
					uint32_t    HeadIndex = 0;
					uint32_t    Count = 0;
					bool        Started = false;

				public:
					CircularStore() = default;

					~CircularStore()
					{
						if (Started)
						{
							Stop();
						}
					}

					bool Start(const char* path)
					{
						if (Started)
						{
							return true;
						}

						Path = path;
						HeadIndex = 0;
						Count = 0;

						if (!LittleFS.exists(Path))
						{
							return ResetStorage();
						}

						File file = LittleFS.open(Path, "r");
						if (!file)
						{
							return ResetStorage();
						}

						uint16_t version = 0;
						uint32_t capacity = 0;
						uint32_t head = 0;
						uint32_t count = 0;

						const bool headerOk = TryReadHeader(file, version, capacity, head, count)
							&& version == TVersion
							&& capacity == StoredCapacity
							&& file.size() == GetFileSize()
							&& count <= StoredCapacity
							&& (count == 0 || head < StoredCapacity);

						file.close();

						if (!headerOk)
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

						const uint32_t writeIndex = (Count < StoredCapacity)
							? (HeadIndex + Count) % StoredCapacity
							: HeadIndex;
						const uint32_t newHead = (Count < StoredCapacity)
							? HeadIndex
							: (HeadIndex + 1) % StoredCapacity;
						const uint32_t newCount = (Count < StoredCapacity)
							? Count + 1
							: StoredCapacity;

						File file = LittleFS.open(Path, "r+");
						if (!file) { return false; }

						const bool ok = WriteRecordAtPhysical(file, writeIndex, record)
							&& WriteHeader(file, newHead, newCount);

						file.flush();
						file.close();

						if (!ok) { return false; }

						HeadIndex = newHead;
						Count = newCount;
						return true;
					}

					bool Read(const uint32_t logicalIndex, TRecord& record) const
					{
						if (!Started || logicalIndex >= Count) { return false; }

						File file = LittleFS.open(Path, "r");
						if (!file) { return false; }

						const bool ok = ReadRecordAtPhysical(file, LogicalToPhysical(logicalIndex), record);
						file.close();
						return ok;
					}

					bool TrimFront(uint32_t trimCount)
					{
						if (!Started) { return false; }
						if (trimCount == 0) { return true; }
						if (trimCount > Count) { trimCount = Count; }

						const uint32_t newCount = Count - trimCount;
						const uint32_t newHead = (newCount == 0) ? 0 : (HeadIndex + trimCount) % StoredCapacity;

						File file = LittleFS.open(Path, "r+");
						if (!file) { return false; }

						const bool ok = WriteHeader(file, newHead, newCount);
						file.flush();
						file.close();

						if (!ok) { return false; }

						HeadIndex = newHead;
						Count = newCount;
						return true;
					}

					bool Reset()
					{
						return ResetStorage();
					}

					uint32_t GetCount() const
					{
						return Started ? Count : 0;
					}

					uint32_t GetCapacity() const
					{
						return StoredCapacity;
					}

					bool IsFull() const
					{
						return Started && Count >= StoredCapacity;
					}

					bool IsStarted() const
					{
						return Started;
					}

				private:
					uint32_t LogicalToPhysical(const uint32_t logicalIndex) const
					{
						return (HeadIndex + logicalIndex) % StoredCapacity;
					}

					static constexpr size_t GetFileSize()
					{
						return HeaderSize + static_cast<size_t>(StoredCapacity) * RecordSize;
					}

					bool ResetStorage()
					{
						if (Path == nullptr) { return false; }

						FSInfo info{};
						if (!LittleFS.info(info)) { return false; }

						const size_t fileSize = GetFileSize();
						size_t reclaimable = 0;
						if (LittleFS.exists(Path))
						{
							File f = LittleFS.open(Path, "r");
							if (f) { reclaimable = f.size(); f.close(); }
						}

						const size_t available = (info.totalBytes > info.usedBytes
							? info.totalBytes - info.usedBytes : 0) + reclaimable;

						if (available < fileSize) { return false; }

						if (LittleFS.exists(Path) && !LittleFS.remove(Path)) { return false; }

						File file = LittleFS.open(Path, "w+");
						if (!file) { return false; }

						const bool ok = ReserveFileSize(file, fileSize) && WriteHeader(file, 0, 0);
						file.flush();
						file.close();

						if (!ok) { LittleFS.remove(Path); return false; }

						HeadIndex = 0;
						Count = 0;
						Started = true;
						return true;
					}

					bool ReserveFileSize(File& file, const size_t targetSize)
					{
						static constexpr size_t ChunkSize = 256;
						uint8_t zeros[ChunkSize]{};

						if (!file.seek(0)) { return false; }

						size_t written = 0;
						while (written < targetSize)
						{
							const size_t chunk = (targetSize - written) < ChunkSize
								? (targetSize - written) : ChunkSize;
							if (file.write(zeros, chunk) != chunk) { return false; }
							written += chunk;
						}
						return true;
					}

					bool TryReadHeader(File& file,
						uint16_t& version,
						uint32_t& capacity,
						uint32_t& headIndex,
						uint32_t& count)
					{
						if (file.size() < HeaderSize || !file.seek(0)) { return false; }

						uint8_t buf[HeaderSize]{};
						if (file.read(buf, HeaderSize) != HeaderSize) { return false; }

						version = ReadUint16(buf + VersionOffset);
						const uint16_t crc = ReadUint16(buf + CrcOffset);
						capacity = ReadUint32(buf + CapacityOffset);
						headIndex = ReadUint32(buf + HeadOffset);
						count = ReadUint32(buf + CountOffset);

						return crc == ComputeHeaderCrc(version, capacity, headIndex, count);
					}

					bool WriteHeader(File& file, const uint32_t headIndex, const uint32_t count)
					{
						if (!file.seek(0)) { return false; }

						uint8_t buf[HeaderSize]{};
						WriteUint16(buf + VersionOffset, TVersion);
						WriteUint16(buf + CrcOffset, ComputeHeaderCrc(TVersion, StoredCapacity, headIndex, count));
						WriteUint32(buf + CapacityOffset, StoredCapacity);
						WriteUint32(buf + HeadOffset, headIndex);
						WriteUint32(buf + CountOffset, count);

						return file.write(buf, HeaderSize) == HeaderSize;
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

					bool ReadRecordAtPhysical(File& file, const uint32_t physicalIndex, TRecord& record) const
					{
						const size_t offset = HeaderSize + static_cast<size_t>(physicalIndex) * RecordSize;
						if (!file.seek(offset)) { return false; }
						return file.read(reinterpret_cast<uint8_t*>(&record), RecordSize) == RecordSize;
					}

					bool WriteRecordAtPhysical(File& file, const uint32_t physicalIndex, const TRecord& record)
					{
						const size_t offset = HeaderSize + static_cast<size_t>(physicalIndex) * RecordSize;
						if (!file.seek(offset)) { return false; }
						return file.write(reinterpret_cast<const uint8_t*>(&record), RecordSize) == RecordSize;
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
#endif