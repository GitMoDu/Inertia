#ifndef _INERTIA_COMPONENTS_LOG_REPOSITORY_FRAM_LOG_REPOSITORY_h
#define _INERTIA_COMPONENTS_LOG_REPOSITORY_FRAM_LOG_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <Fletcher16.h>
#include "../../../Components/Log/Model.h"
#include "../../../Components/Timestamp/Model.h"
#include "../../Storage/Fram/CircularStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Log
		{
			namespace Repository
			{
				template<uint32_t MaxCapacity = 256, uint16_t TBaseAddress = 0>
				class FramLogRepository
					: public Inertia::Components::Log::ILogRepository
				{
				private:
					static constexpr uint16_t StoreVersion = 1;

					using Store = Inertia::Drivers::Storage::Fram::CircularStore<
						Inertia::Components::Log::LogRecordStruct,
						MaxCapacity,
						StoreVersion,
						TBaseAddress>;

				public:
					static constexpr uint16_t BaseAddress = TBaseAddress;
					static constexpr size_t UsedSize = Store::UsedSize;

				private:
					Store FileStore;
					Fletcher16 Hasher{};
					uint32_t NextRecordId = 1;
					bool Started = false;

				public:
					explicit FramLogRepository(Inertia::Components::Storage::Fram::IFramDriver& driver)
						: Inertia::Components::Log::ILogRepository()
						, FileStore(driver)
					{}

					bool Start() override
					{
						if (Started) { return true; }
						if (!FileStore.Start()) { return false; }

						NextRecordId = 1;

						if (FileStore.GetCount() > 0)
						{
							Inertia::Components::Log::LogRecordStruct oldest{};
							Inertia::Components::Log::LogRecordStruct newest{};
							const uint32_t count = FileStore.GetCount();

							const bool ok =
								FileStore.Read(0, oldest)
								&& FileStore.Read(count - 1, newest)
								&& IsValidRecord(oldest)
								&& IsValidRecord(newest)
								&& oldest.RecordId > 0
								&& newest.RecordId == oldest.RecordId + (count - 1);

							if (!ok)
							{
								FileStore.Reset();
							}
							else
							{
								NextRecordId = newest.RecordId + 1;
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
						const Inertia::Components::Timestamp::millis_timestamp_t& timestamp,
						const Inertia::Model::LogEntryStruct& logEntry) override
					{
						if (!Started) { return false; }

						Inertia::Components::Log::LogRecordStruct record{};
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
						Inertia::Components::Log::LogRecordStruct& logRecord) override
					{
						if (!Started || index >= FileStore.GetCount()) { return false; }
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

						const uint32_t oldestId = NextRecordId - FileStore.GetCount();
						if (recordId < oldestId) { return true; }

						uint32_t toDelete = (recordId - oldestId) + 1;
						return FileStore.TrimFront(toDelete);
					}

					void ClearRecords() override
					{
						FileStore.Reset();
						NextRecordId = 1;
					}

					uint32_t GetCount()    override { return Started ? FileStore.GetCount() : 0; }
					uint32_t GetCapacity() override { return MaxCapacity; }
					bool     IsFull()      override { return !Started || FileStore.IsFull(); }

					// Bytes occupied in FRAM — use to calculate baseAddress for adjacent stores.
					static constexpr size_t GetStoreSize() { return UsedSize; }

				private:
					bool IsValidRecord(const Inertia::Components::Log::LogRecordStruct& record)
					{
						return record.RecordId != 0
							&& ComputeRecordCrc(record) == record.Crc;
					}

					uint16_t ComputeRecordCrc(const Inertia::Components::Log::LogRecordStruct& record)
					{
						Hasher.begin();
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 8));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 16));
						Hasher.add(static_cast<uint8_t>(Inertia::Components::Log::ENTRY_CRC_SEED >> 24));
						Hasher.add(
							reinterpret_cast<const uint8_t*>(&record),
							sizeof(Inertia::Components::Log::LogRecordStruct)
							- sizeof(Inertia::Components::Log::LogRecordStruct::Crc));
						return Hasher.getFletcher();
					}
				};
			}
		}
	}
}

#endif