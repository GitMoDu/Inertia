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
			template<Inertia::Model::LogTypeEnum LogLevel = Inertia::Model::LogTypeEnum::Debug,
				size_t BufferSize = 32>
			class BufferTask
				: public Inertia::Model::ILogListener
				, public TS::Task
			{
			private:
				static constexpr uint8_t PUSH_PERIOD = 2;
				static constexpr uint8_t HOLD_PERIOD = 5;
				static constexpr uint8_t RETRY_PERIOD = 10;

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
							TS::Task::enableDelayed(RETRY_PERIOD); // Retry after a delay if the repository is full.
							return true;
						}

						const auto& bufferedLog = Buffer[Tail];
						const uint32_t currentMillis = millis();

						Inertia::Model::millis_timestamp_t timestamp = TimestampSource.GetMillisTimestamp();
						// Offset the millis timestamp with the entry timestamp.
						const uint32_t previous = timestamp.timestamp;
						timestamp.timestamp -= (currentMillis - bufferedLog.Timestamp);
						if (timestamp.timestamp > previous)
						{
							timestamp.overflows -= 1; // Adjust overflow count if the timestamp underflowed due to the offset.
						}

						if (LogRepository.AddEntry(BootId, timestamp, bufferedLog.LogEntry))
						{
							Tail = (Tail + 1) % BufferSize;
							--Count;

							if (Count > BufferSize / 2)
							{
								TS::Task::enableDelayed(0); // Immediately process the next log entry if the buffer is more than half full.
							}
							else
							{
								TS::Task::enableDelayed(PUSH_PERIOD); // Process the next log entry after a short delay to avoid hogging the processor.
							}
						}
						else
						{
							TS::Task::enableDelayed(RETRY_PERIOD); // Retry after a delay if the repository is unable to accept the log entry.
						}
					}
					else
					{
						TS::Task::disable();
					}

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

					if (!TS::Task::isEnabled())
					{
						if (Count < 2)
						{
							TS::Task::enableDelayed(HOLD_PERIOD);
						}
						else
						{
							TS::Task::enableDelayed(PUSH_PERIOD);
						}
					}
				}
			};
		}
	}
}

#endif
