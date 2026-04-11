#ifndef _INERTIA_COMPONENTS_LOG_BUFFER_TASK_h
#define _INERTIA_COMPONENTS_LOG_BUFFER_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#if defined(ARDUINO)
#include <Arduino.h>
#endif
//#include "../../Framework/Interface.h"
#include "Model.h"
#include "../Timestamp/MillisTimestampSource.h"

namespace Inertia
{
	namespace Components
	{
		namespace Log
		{
			/// <summary>
			/// A buffered log listener task that asynchronously processes and stores log entries in a circular buffer before forwarding them to a log repository.
			/// </summary>
			/// <typeparam name="BufferSize">The size of the circular buffer for storing log entries. Must be greater than zero. Defaults to 32.</typeparam>
			/// <typeparam name="CoalescePeriodMillis">The period in milliseconds to coalesce log entries before processing. Defaults to 500.</typeparam>
			template<Inertia::Model::LogTypeEnum LogLevel = Inertia::Model::LogTypeEnum::Debug,
				size_t BufferSize = 32,
				uint32_t CoalescePeriodMillis = 500>
			class BufferTask
				: public Inertia::Model::ILogListener
				, public TS::Task
			{
			private:
				static constexpr uint8_t DebugPersistPrintLimit = 8;

				struct BufferedLogEntryStruct
				{
					uint32_t Timestamp;
					Inertia::Model::LogEntryStruct LogEntry;
				};

				static_assert(BufferSize > 0, "BufferSize must be greater than zero.");

			public:
				uint32_t BootId = 0;

			private:
				Inertia::Model::ILogRepository& LogRepository;
				Inertia::Model::IMillisTimestampSource& TimestampSource;

				BufferedLogEntryStruct Buffer[BufferSize]{};
				size_t Head = 0;
				size_t Tail = 0;
				size_t Count = 0;
				uint8_t DebugPersistPrintCount = 0;

			public:
				BufferTask(TS::Scheduler& scheduler,
					Inertia::Model::IMillisTimestampSource& timestampSource,
					Inertia::Model::ILogRepository& logRepository)
					: TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
					, LogRepository(logRepository)
					, TimestampSource(timestampSource)
				{}

			public:
				bool Callback() override
				{
					if (Count > 0)
					{
						if (LogRepository.IsFull())
						{
							TS::Task::enableDelayed(10); // Retry after a delay if the repository is full.
							return true;
						}

						const auto& bufferedLog = Buffer[Tail];

						bool push = false;
						if (Count > BufferSize / 2)
						{
							// If the buffer is more than half full, process logs more aggressively to prevent overflow.
							push = true;
						}

						const uint32_t currentMillis = millis();

						if (!push && ((currentMillis - bufferedLog.Timestamp) > CoalescePeriodMillis))
						{
							// If the last log entry has been in the buffer for more than the period, process it to prevent excessive delay.
							push = true;
						}

						Inertia::Model::millis_timestamp_t timestamp = TimestampSource.GetMillisTimestamp();
						// Offset the millis timestamp with the entry timestamp.
						const uint32_t previous = timestamp.timestamp;
						timestamp.timestamp -= (currentMillis - bufferedLog.Timestamp);

						if (timestamp.timestamp > previous)
						{
							timestamp.overflows -= 1; // Adjust overflow count if the timestamp underflowed due to the offset.
						}

						if (push && LogRepository.AddEntry(BootId, timestamp, bufferedLog.LogEntry))
						{
							Tail = (Tail + 1) % BufferSize;
							--Count;
							TS::Task::enableDelayed(1); // Quickly process the next log entry if available.
						}
						else
						{
							TS::Task::enableDelayed(10); // Retry after a delay if the repository is full or unable to accept the log entry.
							return true;
						}
					}

					TS::Task::disable();
					return true;
				}

			public:
				void OnLog(const Inertia::Model::LogEntryStruct& logEntry) override
				{
					if (uint8_t(logEntry.Type) < uint8_t(LogLevel))
					{
						// Skip log entries below the configured log level.
						return;
					}

					Buffer[Head] = BufferedLogEntryStruct{
						.Timestamp = millis(),
						.LogEntry = logEntry
					};

					Head = (Head + 1) % BufferSize;

					if (Count == BufferSize)
					{
						Tail = (Tail + 1) % BufferSize;
					}
					else
					{
						++Count;
					}

					TS::Task::enableDelayed(0);
				}
			};
		}
	}
}

#endif
