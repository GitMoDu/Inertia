#ifndef _INERTIA_DRIVERS_IMU_MPU6050_MOTION_STEP_DRIVER_h
#define _INERTIA_DRIVERS_IMU_MPU6050_MOTION_STEP_DRIVER_h

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
				class MotionStepDriver : public Inertia::Components::Lifecycle::IPeriodicDriver,
					public Inertia::Components::DataSource::IDataSource<Model::timestamped_acceleration_t>,
					public Inertia::Components::DataSource::IDataSource<Model::timestamped_angular_velocity_t>
				{
				private:
					SharedDriverType& SharedDriver;

				public:
					MotionStepDriver(SharedDriverType& sharedDriver)
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
						SharedDriver.StepMotion();
					}

					bool GetData(Model::timestamped_acceleration_t& data) final
					{
						return SharedDriver.GetAccelerationData(data);
					}

					bool GetData(Model::timestamped_angular_velocity_t& data) final
					{
						return SharedDriver.GetAngularVelocityData(data);
					}
				};
			}
		}
	}
}

#endif