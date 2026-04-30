#ifndef _INERTIA_COMPONENTS_LIFECYCLE_VARIADIC_DRIVER_TASK_h
#define _INERTIA_COMPONENTS_LIFECYCLE_VARIADIC_DRIVER_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Lifecycle
		{
			/// <summary>
			/// Periodic driver step task, invoking IPeriodicDriver::Step() on schedule.
			/// Start() attempts driver startup; on success the internal Task is enabled.
			/// </summary>
			class DriverStepperTask : public ILifecycleDriver, public TS::Task
			{
			public:
				// Reference to step driver.
				IPeriodicDriver& Driver;

			public:
				/// <summary></summary>
				/// <param name="scheduler">Scheduler reference.</param>
				/// <param name="driver">Driver implementing IPeriodicDriver.</param>
				/// <param name="periodMillis">Execution interval (ms).</param>
				DriverStepperTask(TS::Scheduler& scheduler, IPeriodicDriver& driver, const uint32_t periodMillis)
					: ILifecycleDriver()
					, TS::Task(periodMillis, TASK_FOREVER, &scheduler, false)
					, Driver(driver)
				{}

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