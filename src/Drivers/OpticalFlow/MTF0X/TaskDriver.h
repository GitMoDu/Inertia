#ifndef _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_TEMPLATE_DRIVER_h
#define _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_TEMPLATE_DRIVER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "../../../Framework/Model.h"
#include "DeviceDriver.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace OpticalFlow
		{
			namespace MTF0X
			{
				template<typename SerialType,
					uint8_t MaxReadBytes = 32>
				class TaskDriver : public Model::ILifecycleDriver
					, public Model::IDataSource<Model::timestamped_quality_flow_translation_t>
					, public Model::IDataSource<Model::timestamped_quality_range16_t>
					, private TS::Task
				{
				public:
					using DataTypes = Components::Variadic::VariadicDataTypeList<
						Model::timestamped_quality_flow_translation_t,
						Model::timestamped_quality_range16_t>;

				private:
					Device::Driver DeviceDriver{};
					SerialType& SerialInstance;

				public:
					TaskDriver(TS::Scheduler& scheduler, SerialType& serial_port)
						: Model::ILifecycleDriver()
						, Model::IDataSource<Model::timestamped_quality_flow_translation_t>()
						, Model::IDataSource<Model::timestamped_quality_range16_t>()
						, TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
						, SerialInstance(serial_port)
					{}

					// Delegates directly to DeviceDriver — no intermediate copy.
					bool GetData(Model::timestamped_quality_flow_translation_t& out_data) final
					{
						return DeviceDriver.GetFlow(out_data);
					}

					bool GetData(Model::timestamped_quality_range16_t& out_data) final
					{
						return DeviceDriver.GetRange(out_data);
					}

					bool Start() override
					{
						SerialInstance.begin(Device::MTF01_BAUDRATE);

						if (DeviceDriver.Start())
						{
							TS::Task::enableDelayed(0);
							return true;
						}

						return false;
					}

					void Stop() override
					{
						DeviceDriver.Stop();
						SerialInstance.end();
					}

					void OnSerialEvent()
					{
						TS::Task::enableDelayed(0);
					}

					bool Callback() override
					{
						// Disable task while processing incoming data.
						// Re-enabled task means a serial event has occurred during processing.
						TS::Task::disable();
						uint8_t readCount = 0;
						while (SerialInstance.available()
							&& readCount < MaxReadBytes)
						{
							readCount++;
							DeviceDriver.Parse(SerialInstance.read());
						}

						// Re-enable if we just read some bytes or a serial event occurred during processing.
						if (!TS::Task::isEnabled() || readCount > 0)
						{
							TS::Task::enable();
						}
						return true;
					}
				};
			}
		}
	}
}

#endif