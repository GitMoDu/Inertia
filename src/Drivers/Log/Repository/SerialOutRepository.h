#ifndef _INERTIA_COMPONENTS_LOG_REPOSITORY_SERIAL_OUT_REPOSITORY_h
#define _INERTIA_COMPONENTS_LOG_REPOSITORY_SERIAL_OUT_REPOSITORY_h

#include <Stream.h>

#include "../../../Components/Log/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Log
		{
			namespace Repository
			{
				using namespace Inertia::Components::Log;

				class SerialOutRepository
					: public Inertia::Model::ILogRepository
				{
				private:
					Stream& SerialOut;

				public:
					SerialOutRepository(Stream& serial)
						: Inertia::Model::ILogRepository()
						, SerialOut(serial)
					{}

					~SerialOutRepository() = default;

					bool Start() override
					{
						return true;
					}

					void Stop() override
					{
						// No-op.
					}

					bool AddEntry(const uint32_t /*bootId*/, const Inertia::Model::millis_timestamp_t& timestamp, const Inertia::Model::LogEntryStruct& logEntry) override
					{
						if (SerialOut.availableForWrite() == 0)
						{
							return true; // Serial not available, skip logging.
						}
						SerialOut.print(timestamp.GetSeconds());
						SerialOut.print(' ');
						switch (logEntry.Type)
						{
						case Inertia::Model::LogTypeEnum::Error:
							SerialOut.print(F("[ERROR] "));
							break;
						case Inertia::Model::LogTypeEnum::Warning:
							SerialOut.print(F("[WARNING] "));
							break;
						case Inertia::Model::LogTypeEnum::Info:
							SerialOut.print(F("[INFO] "));
							break;
						case Inertia::Model::LogTypeEnum::Debug:
						default:
							SerialOut.print(F("[DEBUG] "));
							break;
						}

						SerialOut.print(F(" Tag: "));
						SerialOut.print(logEntry.Tag);
						SerialOut.print(F(", Code: "));
						SerialOut.print(logEntry.Code);
						SerialOut.print(F(", Value: "));
						SerialOut.println(logEntry.Value);

						return true;
					}

					/// <summary>
					/// Retrieves a log record.
					/// </summary>
					/// <param name="logRecord">An output parameter that will be populated with the retrieved log record data.</param>
					/// <returns>True if a log record was successfully retrieved; otherwise, false.</returns>
					virtual bool GetRecordAt(const size_t index, Inertia::Model::LogRecordStruct& logRecord) override
					{
						(void)index;
						(void)logRecord;
						return false; // Not implemented for SerialOutRepository.
					}

					virtual bool GetLatestEntryId(uint32_t& recordId) override
					{
						(void)recordId;
						return false; // Not implemented for SerialOutRepository.
					}

					virtual bool DeleteRecordsThrough(const uint32_t recordId) override
					{
						(void)recordId;
						return true; // No-op for SerialOutRepository, as it does not store log records.
					}

					virtual void ClearRecords() {} // No-op for SerialOutRepository, as it does not store log records.

					virtual uint32_t GetCount() override
					{
						return 0; // SerialOutRepository does not store log records, so count is always 0.
					}

					virtual uint32_t GetCapacity() override
					{
						return SIZE_MAX; // SerialOutRepository does not have a capacity, so return maximum size.
					}

					virtual bool IsFull() override
					{
						return false;
					}
				};
			}
		}
	}
}

#endif