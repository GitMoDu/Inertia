#ifndef _INERTIA_COMPONENTS_VARIADIC_VARIADIC_DRIVER_TASK_h
#define _INERTIA_COMPONENTS_VARIADIC_VARIADIC_DRIVER_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "../../Framework/Model.h"

namespace Inertia
{
	namespace Components
	{
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
			class BaseDataSource : public Model::IDataSource<DataType>
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
					: Driver(driver)
				{
				}

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

		namespace Composite
		{
			/// <summary>
			/// Periodic driver step task, invoking IPeriodicDriver::Step() on schedule.
			/// Start() attempts driver startup; on success the internal Task is enabled.
			/// </summary>
			class DriverStepperTask : public Model::ILifecycleDriver, public TS::Task
			{
			public:
				// Reference to step driver.
				Model::IPeriodicDriver& Driver;

			public:
				/// <summary></summary>
				/// <param name="scheduler">Scheduler reference.</param>
				/// <param name="driver">Driver implementing IPeriodicDriver.</param>
				/// <param name="periodMillis">Execution interval (ms).</param>
				DriverStepperTask(TS::Scheduler& scheduler, Model::IPeriodicDriver& driver, const uint32_t periodMillis)
					: Model::ILifecycleDriver()
					, TS::Task(periodMillis, TASK_FOREVER, &scheduler, false)
					, Driver(driver)
				{
				}

				/// <summary>
				/// Start driver and enable scheduled stepping.
				/// </summary>
				/// <returns>true if driver started.</returns>
				bool Start() final
				{
					if (Driver.Start())
					{
						TS::Task::enable();

						return true;
					}
					else
					{
						Stop();

						return false;
					}
				}

				/// <summary>
				/// Disable task and stop driver.
				/// </summary>
				void Stop() final
				{
					Driver.Stop();
					TS::Task::disable();
				}

				bool Callback() final
				{
					Driver.Step();

					return true;
				}
			};
		}
	}
}
#endif