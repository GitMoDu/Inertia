#ifndef _INERTIA_COMPONENTS_LOG_MODEL_h
#define _INERTIA_COMPONENTS_LOG_MODEL_h

#include "../../Components/Core/Lifecycle/Model.h"
#include "../../Components/Timestamp/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Log
		{
			static constexpr uint32_t LOG_TAG = 985820946; // Random unique tag for Log meta logs.

			static constexpr uint32_t ENTRY_CRC_SEED = LOG_TAG; // Use the same unique tag as a seed for log entry CRC calculations.

			/// <summary>
			/// Defines the types/levels of log messages.
			/// </summary>
			enum class LogTypeEnum : uint8_t
			{
				Debug,
				Info,
				Warning,
				Error
			};

			enum class LogCodeEnum : uint8_t
			{
				ErrorRepositoryInvalidEntry, // Attempted to read an invalid log entry from a repository.
				ErrorCorruptedEntry // A log entry was found to be corrupted during retrieval from the repository.
			};

			struct LogEntryStruct
			{
				// Source tag. Uniquely identifies the source or category of the log entry, such as a specific module, component, or subsystem. 
				// This allows for categorization and filtering of log entries based on their origin.
				uint32_t Tag;

				// Source instance Id. This allows for distinguishing between multiple instances of the same source type, 
				// such as multiple sensors of the same mode.
				uint8_t Instance;

				// uint8_t type. Defines the type or severity level of the log entry: debug, info, warning, or error.
				LogTypeEnum Type;

				// Code. User-defined code that indicates specific events, error codes, or state changes within the source.
				uint8_t Code;

				// Value. Can provide additional information relevant to the log entry.
				uint8_t Value;
			};

			static constexpr size_t LogEntrySize = sizeof(LogEntryStruct);
			static_assert(sizeof(LogTypeEnum) == sizeof(uint8_t), "LogTypeEnum must remain one byte.");
			static_assert(LogEntrySize == 8, "LogEntryStruct wire size changed.");

			struct LogRecordStruct : LogEntryStruct
			{
				// Rolling record ID to uniquely identify each log entry and 
				// allow for tracking the order of log entries, even across sessions and reboots.
				uint32_t RecordId;

				// Boot ID to correlate log entries within the same session, 
				// allowing for grouping and analysis of logs based on sessions.
				uint32_t BootId;

				// Boot timestamp in milliseconds, with overflow count to allow for extended uptime tracking.
				// The timestamp represents the time since the system booted, and is used to correlate log entries with system uptime and events.
				uint32_t TimestampMillis;
				uint16_t TimestampOverflows;

				// Internal CRC for the log record, calculated over the entire record except for the CRC field itself. 
				// This allows for integrity verification of log records when stored or transmitted.
				uint16_t Crc;
			};

			static constexpr size_t LogRecordSize = sizeof(LogRecordStruct);
			static_assert(LogRecordSize == 24, "LogRecordStruct wire size changed.");

			/// <summary>
			/// Interface for listeners that receive log entry notifications.
			/// </summary>
			struct ILogListener
			{
				~ILogListener() = default;

				virtual void OnLog(const LogEntryStruct& logEntry) = 0;
			};

			/// <summary>
			/// Interface for a log repository that manages storage and retrieval of log records with associated boot IDs and timestamps.
			/// </summary>
			struct ILogRepository : Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~ILogRepository() = default;

				/// <summary>
				/// Creates a log record with the provided timestamp and log entry data, and adds it to the repository.
				/// </summary>
				/// <param name="bootId">The boot ID associated with the log entry.</param>
				/// <param name="timestamp">The timestamp associated with the log entry.</param>
				/// <param name="logEntry">The log entry data to be added.</param>
				/// <returns>True if the log entry was successfully added; otherwise, false.</returns>
				virtual bool AddEntry(const uint32_t bootId, const Inertia::Components::Timestamp::millis_timestamp_t& timestamp, const LogEntryStruct& logEntry) = 0;

				/// <summary>
				/// Retrieves a log record at the specified zero-based index.
				/// </summary>
				/// <param name="index">The zero-based index of the log record to retrieve.</param>
				/// <param name="logRecord">An output parameter that will be populated with the retrieved log record data.</param>
				/// <returns>True if a log record was successfully retrieved; otherwise, false.</returns>
				virtual bool GetRecordAt(const size_t index, LogRecordStruct& logRecord) = 0;

				/// <summary>
				/// Retrieves the most recent log record ID currently stored by the repository.
				/// </summary>
				/// <param name="recordId">An output parameter that will be populated with the most recent record ID.</param>
				/// <returns>True if a record ID was successfully retrieved; otherwise, false.</returns>
				virtual bool GetLatestEntryId(uint32_t& recordId) = 0;

				/// <summary>
				/// Deletes all log records with record IDs up to and including the provided record ID.
				/// </summary>
				/// <param name="recordId">The last record ID to delete.</param>
				/// <returns>True if the records were deleted successfully; otherwise, false.</returns>
				virtual bool DeleteRecordsThrough(const uint32_t recordId) = 0;

				/// <summary>
				/// Clears all records.
				/// </summary>
				virtual void ClearRecords() = 0;

				/// <summary>
				/// Gets the count of records. The count should be capped at the return type max of UINT32_MAX,
				/// even if the underlying repository may contain more records, to respect the interface contract.
				/// </summary>
				/// <returns>The count of records, capped at UINT32_MAX.</returns>
				virtual uint32_t GetCount() = 0;

				/// <summary>
				/// Gets the capacity. The count should be capped at the return type max of UINT32_MAX,
				/// even if the underlying repository may have a larger capacity, to respect the interface contract.
				/// </summary>
				/// <returns>The capacity as an unsigned 32-bit integer.</returns>
				virtual uint32_t GetCapacity() = 0;

				/// <summary>
				/// Quick check to determine if the repository is full and cannot accept new entries
				/// </summary>
				/// <returns>True if the repository is full; otherwise, false.</returns>
				virtual bool IsFull() = 0;
			};
		}
	}

	// Exposes base log features.
	namespace Model
	{
		using Inertia::Components::Log::LogTypeEnum;
		using Inertia::Components::Log::LogEntryStruct;
		using Inertia::Components::Log::ILogListener;
	}
}
#endif