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
	}
}

#endif
