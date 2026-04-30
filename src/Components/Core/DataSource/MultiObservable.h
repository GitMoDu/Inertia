#ifndef _INERTIA_COMPONENTS_DATASOURCE_MULTI_OBSERVABLE_h
#define _INERTIA_COMPONENTS_DATASOURCE_MULTI_OBSERVABLE_h

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace DataSource
		{
			/// <summary>
			/// Implements an observable pattern that supports multiple observers. 
			/// Allows registration, removal, and notification of up to a fixed number of observers.
			/// </summary>
			/// <typeparam name="DataType">The type of data that will be observed and passed to observers.</typeparam>
			/// <typeparam name="MaxObserverCount">The maximum number of observers that can be registered. Defaults to 3.</typeparam>
			template<typename DataType,
				uint8_t MaxObserverCount = 3>
			class MultiObservable : public IObservable<DataType>
			{
			private:
				IObserver<DataType>* Observers[MaxObserverCount]{};

			public:
				MultiObservable() : IObservable<DataType>()
				{}

				~MultiObservable() = default;

				/// <summary>
				/// Registers an observer to the collection of observers.
				/// Wraps the base SetObserver method to allow multiple observers to be registered.
				/// </summary>
				/// <param name="observer">A pointer to the observer to register.</param>
				/// <returns>True if the observer was successfully registered; false if the observer collection is full.</returns>
				bool SetObserver(IObserver<DataType>* observer) override
				{
					if (observer == nullptr)
					{
						return false;
					}

					RemoveObserver(observer); // Ensure the observer is not already registered.

					for (uint8_t i = 0; i < MaxObserverCount; ++i)
					{
						if (Observers[i] == nullptr)
						{
							Observers[i] = observer;
							return true;
						}
					}

					return false;
				}

				/// <summary>
				/// Removes an observer from the collection of registered observers.
				/// </summary>
				/// <param name="observer">A pointer to the observer to remove from the collection.</param>
				bool RemoveObserver(IObserver<DataType>* observer) override
				{
					if (observer == nullptr)
					{
						return false;
					}

					for (uint8_t i = 0; i < MaxObserverCount; ++i)
					{
						if (Observers[i] == observer)
						{
							Observers[i] = nullptr;
							return true;
						}
					}

					return false;
				}

				/// <summary>
				/// Same API as the IObserver's OnDataUpdate, but will broadcast to all registered observers instead of just one.
				/// </summary>
				/// <param name="data">The updated data to broadcast to all observers.</param>
				void OnDataUpdate(const DataType& data)
				{
					for (uint8_t i = 0; i < MaxObserverCount; ++i)
					{
						if (Observers[i] != nullptr)
						{
							Observers[i]->OnDataUpdate(data);
						}
					}
				}
			};
		}
	}
}

#endif
