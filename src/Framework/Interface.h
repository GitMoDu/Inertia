#ifndef _INERTIA_INTERFACE_h
#define _INERTIA_INTERFACE_h

#include <stdint.h>

namespace Inertia
{
	namespace Model
	{
		struct ILifecycleDriver
		{
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
			/// <summary>
			/// Generic call for periodic step/update. Pure virtual function that must be implemented by derived classes.
			/// </summary>
			virtual void Step() = 0;
		};

		/// <summary>
		/// Generic interface for retrieving items of a specified DataType, by copy to reference.
		/// </summary>
		/// <typeparam name="DataType"> Payload data type. </typeparam>
		template<typename DataType>
		struct IDataSource
		{
			virtual bool GetData(DataType& data) = 0;
		};
	}
}

#endif
