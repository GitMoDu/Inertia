#ifndef _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_TEMPLATE_DRIVER_h
#define _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_TEMPLATE_DRIVER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "Model.h"
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
					uint8_t MaxReadBytes = 36>
				class TaskDriver : public Model::ILifecycleDriver
					, public Model::IDataSource<Model::timestamped_quality_flow_translation_t>
					, public Model::IDataSource<Model::timestamped_quality_range16_t>
					, public TS::Task
				{
				public:
                   static constexpr uint32_t WarningLogIntervalMillis = 1000;

				public:
					using DataTypes = Components::Variadic::VariadicDataTypeList<
						Model::timestamped_quality_flow_translation_t,
						Model::timestamped_quality_range16_t>;

				private:
					Device::Driver DeviceDriver{};
					SerialType& SerialInstance;
					uint32_t LastReadLimitWarningMillis = 0;

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;

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
						DeviceDriver.LogListener = LogListener;

						if (DeviceDriver.Start())
						{
							TS::Task::enableDelayed(0);
							return true;
						}

						if (LogListener != nullptr)
						{
							LogListener->OnLog(Inertia::Model::LogEntryStruct{
								.Tag = Model::LOG_TAG,
								.Instance = 0,
								.Type = Inertia::Model::LogTypeEnum::Error,
								.Code = static_cast<uint8_t>(Model::LogCodeEnum::TaskDriverStartFailed),
								.Value = 0
								});
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

						if (readCount >= MaxReadBytes
							&& SerialInstance.available()
							&& LogListener != nullptr)
						{
                          const uint32_t now = millis();
							if ((now - LastReadLimitWarningMillis) >= WarningLogIntervalMillis)
							{
								LastReadLimitWarningMillis = now;
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = Model::LOG_TAG,
									.Instance = 0,
									.Type = Inertia::Model::LogTypeEnum::Warning,
									.Code = static_cast<uint8_t>(Model::LogCodeEnum::WarningSerialReadLimitReached),
									.Value = readCount
									});
							}
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