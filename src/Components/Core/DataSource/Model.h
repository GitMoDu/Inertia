#ifndef _INERTIA_COMPONENTS_DATASOURCE_MODEL_h
#define _INERTIA_COMPONENTS_DATASOURCE_MODEL_h	

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace DataSource
		{
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

			namespace Variadic
			{
				template<typename... Ts> struct VariadicDataTypeList {};

				/// <summary>
				/// Forwards IDataSource<> requests to an underlying driver instance.
				/// Base for VariadicDataSource.
				/// </summary>
				/// <typeparam name="DriverType">Concrete driver type exposing bool GetData(DataType&).</typeparam>
				/// <typeparam name="DataType">Payload type produced by the driver.</typeparam>
				template<typename DriverType, typename DataType>
				class BaseDataSource : public Inertia::Components::DataSource::IDataSource<DataType>
				{
				private:
					// Reference to data source driver.
					DriverType& Driver;

				public:
					/// <summary>
					/// Construct forwarding data source.
					/// </summary>
					/// <param name="driver">Reference to concrete driver.</param>
					explicit BaseDataSource(DriverType& driver)
						:Inertia::Components::DataSource::IDataSource<DataType>()
						, Driver(driver)
					{}

					/// <summary>Retrieve one data sample from driver.</summary>
					/// <param name="out">Filled on success.</param>
					/// <returns>true if data acquired.</returns>
					virtual bool GetData(DataType& out) override
					{
						return Driver.GetData(out);
					}
				};

				/// <summary>
				/// Aggregates multiple ForwardDataSource specializations, enabling one object
				/// to be queried as any IDataSource&lt;T&gt; for T in DataTypes...
				/// </summary>
				/// <typeparam name="DriverType">Driver supporting GetData for each DataTypes...</typeparam>
				/// <typeparam name="DataTypes">Variadic list of supported payload types.</typeparam>
				template<typename DriverType, typename... DataTypes>
				class TemplateDataSource : public BaseDataSource<DriverType, DataTypes>...
				{
				public:
					/// <summary>Construct all forwarding bases.</summary>
					/// <param name="driver">Driver reference.</param>
					explicit TemplateDataSource(DriverType& driver)
						: BaseDataSource<DriverType, DataTypes>(driver)...
					{
					}
				};
			}
		}
	}

	// Temporary expose.
	namespace Model
	{
		using namespace Inertia::Components::DataSource;
	}
}

#endif
