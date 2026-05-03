#ifndef _INERTIA_DRIVERS_IMU_MPU6050_TEMPERATURE_STEP_DRIVER_h
#define _INERTIA_DRIVERS_IMU_MPU6050_TEMPERATURE_STEP_DRIVER_h

#include "SharedDriver.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Imu
		{
			namespace Mpu6050
			{
				template<typename SharedDriverType>
				class TemperatureStepDriver : public Inertia::Components::Lifecycle::IPeriodicDriver,
					public Inertia::Components::DataSource::IDataSource<Model::timestamped_temperature_t>
				{
				private:
					SharedDriverType& SharedDriver;

				public:
					TemperatureStepDriver(SharedDriverType& sharedDriver)
						: SharedDriver(sharedDriver)
					{}

					bool Start() final
					{
						return SharedDriver.Start();
					}

					void Stop() final
					{
						SharedDriver.Stop();
					}

					void Step() final
					{
						SharedDriver.StepTemperatureWithRefresh();
					}

					bool GetData(Model::timestamped_temperature_t& data) final
					{
						return SharedDriver.GetTemperatureData(data);
					}
				};
			}
		}
	}
}

#endif