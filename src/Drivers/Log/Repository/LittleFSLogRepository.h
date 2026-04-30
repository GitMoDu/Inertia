#ifndef _INERTIA_DRIVERS_LOG_REPOSITORY_LITTLEFS_LOG_REPOSITORY_h
#define _INERTIA_DRIVERS_LOG_REPOSITORY_LITTLEFS_LOG_REPOSITORY_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "../../../Components/Log/Model.h"
#include "../../Storage/LittleFs/CircularStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Log
		{
			namespace Repository
			{
				using namespace Inertia::Components::Log;

				template<uint32_t MaxCapacity = 1024>
				class LittleFsLogRepository : public Inertia::Model::ILogRepository
				{
				private:
					// Bump StoreVersion whenever LogRecordStruct layout changes.
					static constexpr uint16_t StoreVersion = 1;

					using Store = Inertia::Drivers::Storage::LittleFs::CircularStore<
						Inertia::Model::LogRecordStruct,
						MaxCapacity,
						StoreVersion>;

				private:
					Store       FileStore{};
					Fletcher16  Hasher{};
					uint32_t    NextRecordId = 1;
					bool        Started = false;
					const char* Path;

				public:
					explicit LittleFsLogRepository(const char* path = "/logs.bin")
						: Inertia::Model::ILogRepository()
						, Path(path)
					{}

					// Caller is responsible for LittleFS.begin() before Start()
					// and LittleFS.end() after Stop().
					bool Start() override
					{
						if (Started)
						{
							return true;
						}

						if (!FileStore.Start(Path))
						{
							return false;
						}

						NextRecordId = 1;

						if (FileStore.GetCount() > 0)
						{
							Inertia::Model::LogRecordStruct record{};
							Inertia::Model::LogRecordStruct lastRecord{};
							const uint32_t count = FileStore.GetCount();
							bool integrityOk = true;

							for (uint32_t i = 0; i < count; ++i)
							{
								if (!FileStore.Read(i, record)
									|| !IsValidRecord(record)
									|| (i > 0 && record.RecordId != (lastRecord.RecordId + 1)))
								{
									integrityOk = false;
									break;
								}

								lastRecord = record;
							}

							if (!integrityOk)
							{
								FileStore.Reset();
								// NextRecordId stays 1, count is now 0.
							}
							else
							{
								NextRecordId = lastRecord.RecordId + 1;
							}
						}

						Started = true;

						return true;
					}

					void Stop() override
					{
						FileStore.Stop();
						NextRecordId = 1;
						Started = false;
					}

					bool AddEntry(const uint32_t bootId,
						const Inertia::Model::millis_timestamp_t& timestamp,
						const Inertia::Model::LogEntryStruct& logEntry) override
					{
						if (!Started)
						{
							return false;
						}

						Inertia::Model::LogRecordStruct record{};
						static_cast<Inertia::Model::LogEntryStruct&>(record) = logEntry;
						record.RecordId = NextRecordId;
						record.BootId = bootId;
						record.TimestampMillis = timestamp.timestamp;
						record.TimestampOverflows = timestamp.overflows;
						record.Crc = ComputeRecordCrc(record);

						if (!FileStore.Write(record)) { return false; }

						++NextRecordId;
						return true;
					}

					bool GetRecordAt(const size_t index,
						Inertia::Model::LogRecordStruct& logRecord) override
					{
						if (!Started || index >= FileStore.GetCount())
						{
							return false;
						}

						return FileStore.Read(static_cast<uint32_t>(index), logRecord);
					}

					bool GetLatestEntryId(uint32_t& recordId) override
					{
						if (!Started || FileStore.GetCount() == 0) { return false; }
						recordId = NextRecordId - 1;
						return true;
					}

					bool DeleteRecordsThrough(const uint32_t recordId) override
					{
						if (!Started) { return false; }
						if (FileStore.GetCount() == 0) { return true; }

						Inertia::Model::LogRecordStruct oldestRecord{};
						if (!FileStore.Read(0, oldestRecord) || !IsValidRecord(oldestRecord))
						{
							return false;
						}

						const uint32_t oldestId = oldestRecord.RecordId;
						if (recordId < oldestId) { return true; }

						uint32_t toDelete = (recordId - oldestId) + 1;
						return FileStore.TrimFront(toDelete);
					}

					void ClearRecords() override
					{
						FileStore.Reset();
						NextRecordId = 1;
					}

					uint32_t GetCount() override
					{
						return Started ? FileStore.GetCount() : 0;
					}

					uint32_t GetCapacity() override
					{
						return MaxCapacity;
					}

					bool IsFull() override
					{
						return !Started;
					}

				private:
					bool IsValidRecord(const Inertia::Model::LogRecordStruct& record)
					{
						return record.RecordId != 0
							&& ComputeRecordCrc(record) == record.Crc;
					}

					uint16_t ComputeRecordCrc(const Inertia::Model::LogRecordStruct& record)
					{
						Hasher.begin();
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 8));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 16));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 24));
						Hasher.add(
							reinterpret_cast<const uint8_t*>(&record),
							sizeof(Inertia::Model::LogRecordStruct)
							- sizeof(Inertia::Model::LogRecordStruct::Crc));
						return Hasher.getFletcher();
					}
				};
			}
		}
	}
}

#endif
#endif