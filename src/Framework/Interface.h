#ifndef _INERTIA_INTERFACE_h
#define _INERTIA_INTERFACE_h

#include "Model.h"

namespace Inertia
{
	namespace Model
	{
		/// <summary>
		/// Defines an interface for managing the lifecycle of an object with start and stop operations.
		/// </summary>
		struct ILifecycleDriver
		{
			~ILifecycleDriver() = default;

			/// <summary>
			/// Starts the object. Pure virtual function that must be implemented by derived classes.
			/// </summary>
			/// <returns>true if the start succeeded; otherwise false.</returns>
			virtual bool Start() = 0;

			/// <summary>
			/// Stop the object, regardless of current state. Pure virtual function that must be implemented by derived classes.
			/// </summary>
			virtual void Stop() = 0;
		};

		/// <summary>
		/// Generic driver interface with basic lifecycle and step method for periodic update.
		/// </summary>
		struct IPeriodicDriver : public ILifecycleDriver
		{
			~IPeriodicDriver() = default;

			/// <summary>
			/// Generic call for periodic step/update. Pure virtual function that must be implemented by derived classes.
			/// </summary>
			virtual void Step() = 0;
		};

		/// <summary>
		/// Generic interface for retrieving items of a specified DataType, by copy to reference.
		/// </summary>
		/// <typeparam name="DataType">The type of data to be retrieved.</typeparam>
		template<typename DataType>
		struct IDataSource
		{
			~IDataSource() = default;

			/// <summary>
			/// Copies data into the provided reference parameter.
			/// Should be treated as a const method that does not modify the state of the data source.
			/// </summary>
			/// <param name="data">A reference to a DataType object where the retrieved data will be stored.</param>
			/// <returns>True if the data was successfully retrieved; otherwise, false.</returns>
			virtual bool GetData(DataType& data) = 0;
		};

		/// <summary>
		/// Generic interface for observers that receive pushed data of a specified DataType.
		/// </summary>
		/// <typeparam name="DataType">The type of data observed by the observer.</typeparam>
		template<typename DataType>
		struct IObserver
		{
			~IObserver() = default;

			/// <summary>
			/// Handles an observed data update.
			/// </summary>
			/// <param name="data">The observed data to copied.</param>
			/// <returns>True if the observed data was handled successfully; otherwise, false.</returns>
			virtual void OnDataUpdate(const DataType& data) = 0;
		};

		/// <summary>
		/// Generic interface for observable sources that can have an observer attached to receive pushed data of a specified DataType.
		/// </summary>
		/// <typeparam name="DataType">The type of data observed from the source.</typeparam>
		template<typename DataType>
		struct IObservable
		{
			~IObservable() = default;

			/// <summary>
			/// Sets the observer for receiving pushed data updates.
			/// Will notify the observer at least once when it is set, and may notify more times as data is updated.
			/// </summary>
			/// <param name="observer">A pointer to the observer to be set.</param>
			/// <returns>True if the observer was registered successfully; otherwise, false.</returns>
			virtual bool SetObserver(IObserver<DataType>* observer) = 0;

			/// <summary>
			/// Removes the specified observer from receiving pushed data updates.
			/// </summary>
			/// <param name="observer">A pointer to the observer to be removed.</param>
			/// <returns>True if the observer was removed successfully; otherwise, false.</returns>
			virtual bool RemoveObserver(IObserver<DataType>* observer) = 0;
		};

		/// <summary>
		/// Generic interface for events, paired with an event data type, which is also used to identify the source of the event.
		/// </summary>
		/// <typeparam name="DataType">The type of data associated with the event.</typeparam>
		template<typename DataType>
		struct IEventListener
		{
			~IEventListener() = default;

			virtual void OnEvent(const DataType data) = 0;
		};

		/// <summary>
		/// Interface for listeners that receive log entry notifications.
		/// </summary>
		struct ILogListener
		{
			~ILogListener() = default;

			virtual void OnLog(const LogEntryStruct& logEntry) = 0;
		};

		struct IMillisTimestampSource
		{
			~IMillisTimestampSource() = default;

			virtual millis_timestamp_t GetMillisTimestamp() = 0;
		};

		/// <summary>
		/// Interface for managing a boot counter repository.
		/// </summary>
		struct IBootCounterRepository : ILifecycleDriver
		{
			~IBootCounterRepository() = default;

			virtual uint32_t GetCounter() = 0;
		};

		/// <summary>
		/// Interface for a log repository that manages storage and retrieval of log records with associated boot IDs and timestamps.
		/// </summary>
		struct ILogRepository : ILifecycleDriver
		{
			~ILogRepository() = default;

			/// <summary>
			/// Creates a log record with the provided timestamp and log entry data, and adds it to the repository.
			/// </summary>
			/// <param name="bootId">The boot ID associated with the log entry.</param>
			/// <param name="timestamp">The timestamp associated with the log entry.</param>
			/// <param name="logEntry">The log entry data to be added.</param>
			/// <returns>True if the log entry was successfully added; otherwise, false.</returns>
			virtual bool AddEntry(const uint32_t bootId, const millis_timestamp_t& timestamp, const LogEntryStruct& logEntry) = 0;

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

#endif
