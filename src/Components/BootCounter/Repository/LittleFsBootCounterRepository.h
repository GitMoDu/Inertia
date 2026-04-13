#ifndef _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h
#define _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <LittleFS.h>
#include <Fletcher16.h>
#include "../Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace BootCounter
		{
			namespace Repository
			{
				class LittleFsBootCounterRepository
					: public Inertia::Model::IBootCounterRepository
				{
				private:
					struct stored_boot_count_t
					{
						uint32_t Count = 0;
						uint16_t Crc = 0;
					};

					const char* Path;
					Fletcher16 Hasher{};
					uint32_t BootCount = 0;

					static constexpr uint32_t ChecksumSeed = 984818663;
					static constexpr size_t RecordPayloadSize = sizeof(uint32_t);
					static constexpr size_t RecordSize = RecordPayloadSize + sizeof(uint16_t);

				public:
					explicit LittleFsBootCounterRepository(const char* path = "/boot-count.bin")
						: Inertia::Model::IBootCounterRepository()
						, Path(path)
					{}

					void Stop() override
					{
						// No-op.
					}

					bool Start() override
					{
						if (!LittleFS.begin())
						{
							return false;
						}

						uint32_t storedBootCount = 0;

						if (LittleFS.exists(Path))
						{
							if (!TryReadBootCount(storedBootCount))
							{
								if (!ResetStorageFile())
								{
									LittleFS.end();
									return false;
								}
							}
						}

						BootCount = storedBootCount + 1;

						if (!WriteBootCount(BootCount))
						{
							LittleFS.end();
							return false;
						}

						LittleFS.end();

						return true;
					}

					bool ResetStorageFile()
					{
						BootCount = 0;
						if (LittleFS.exists(Path) && !LittleFS.remove(Path))
						{
							return false;
						}

						return true;
					}

					virtual uint32_t GetCounter() override
					{
						return BootCount;
					}

				private:
					bool TryReadBootCount(uint32_t& storedBootCount)
					{
						File readFile = LittleFS.open(Path, "r");
						if (!readFile)
						{
							return false;
						}

						uint8_t recordBuffer[RecordSize]{};
						const size_t bytesRead = readFile.read(recordBuffer, sizeof(recordBuffer));
						readFile.close();

						return bytesRead == sizeof(recordBuffer)
							&& TryParseRecord(recordBuffer, storedBootCount);
					}

					bool WriteBootCount(const uint32_t bootCount)
					{
						File writeFile = LittleFS.open(Path, "w");
						if (!writeFile)
						{
							return false;
						}

						uint8_t recordBuffer[RecordSize]{};
						BuildRecord(bootCount, recordBuffer);

						const size_t bytesWritten = writeFile.write(recordBuffer, sizeof(recordBuffer));
						writeFile.flush();
						writeFile.close();

						return bytesWritten == sizeof(recordBuffer);
					}

					void BuildRecord(const uint32_t bootCount, uint8_t* const recordBuffer)
					{
						stored_boot_count_t record{};
						record.Count = bootCount;
						record.Crc = ComputeChecksum(record.Count);

						WriteUint32(recordBuffer, record.Count);
						WriteUint16(recordBuffer + RecordPayloadSize, record.Crc);
					}

					bool TryParseRecord(const uint8_t* const recordBuffer, uint32_t& bootCount)
					{
						stored_boot_count_t record{};
						record.Count = ReadUint32(recordBuffer);
						record.Crc = ReadUint16(recordBuffer + RecordPayloadSize);

						if (record.Crc != ComputeChecksum(record.Count))
						{
							return false;
						}

						bootCount = record.Count;
						return true;
					}

					uint16_t ComputeChecksum(const uint32_t bootCount)
					{
						uint8_t payload[RecordPayloadSize]{};
						uint8_t seedBuffer[sizeof(ChecksumSeed)]{};

						WriteUint32(seedBuffer, ChecksumSeed);
						WriteUint32(payload, bootCount);

						Hasher.begin();
						Hasher.add(seedBuffer, sizeof(seedBuffer));
						Hasher.add(payload, sizeof(payload));
						return Hasher.getFletcher();
					}

					static void WriteUint16(uint8_t* const destination, const uint16_t value)
					{
						destination[0] = static_cast<uint8_t>(value & 0xFFu);
						destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
					}

					static void WriteUint32(uint8_t* const destination, const uint32_t value)
					{
						destination[0] = static_cast<uint8_t>(value & 0xFFu);
						destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
						destination[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
						destination[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
					}

					static uint16_t ReadUint16(const uint8_t* const source)
					{
						return static_cast<uint16_t>(source[0])
							| static_cast<uint16_t>(static_cast<uint16_t>(source[1]) << 8);
					}

					static uint32_t ReadUint32(const uint8_t* const source)
					{
						return static_cast<uint32_t>(source[0])
							| (static_cast<uint32_t>(source[1]) << 8)
							| (static_cast<uint32_t>(source[2]) << 16)
							| (static_cast<uint32_t>(source[3]) << 24);
					}
				};
			}
		}
	}
}

#endif