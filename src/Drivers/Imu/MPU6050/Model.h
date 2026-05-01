#ifndef _INERTIA_DRIVERS_IMU_MPU6050_DEVICE_MODEL_h
#define _INERTIA_DRIVERS_IMU_MPU6050_DEVICE_MODEL_h

#include "../../../Components/Core/Primitives.h"
#include "../../../Components/HardwareInterface/I2c/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Imu
		{
			namespace Mpu6050
			{
				static constexpr uint32_t LOG_TAG = 867172820; // Random unique tag for Mpu6050 logs.

				enum class LogCodeEnum : uint8_t
				{
					ErrorBoot,
					ErrorReadMotion,
					ErrorReadTemperature,
					RecoveryAttempt
				};

				struct LogRecoveryAttempt
				{
					static uint8_t GetRecoveryCount(const Inertia::Model::LogEntryStruct& entry)
					{
						return entry.Value;
					}
				};


				/// <summary>
				/// Accelerometer full-scale range.
				/// </summary>
				enum class AccelerometerRangeEnum : uint8_t
				{
					Range2g = 0,
					Range4g = 1,
					Range8g = 2,
					Range16g = 3
				};

				/// <summary>
				/// Gyroscope full-scale range.
				/// </summary>
				enum class GyroscopeRangeEnum : uint8_t
				{
					Range250dps = 0,
					Range500dps = 1,
					Range1000dps = 2,
					Range2000dps = 3
				};

				/// <summary>
				/// Digital low-pass filter mode.
				/// </summary>
				enum class DlpfModeEnum : uint8_t
				{
					Off = 0,
					Bw188 = 1,
					Bw98 = 2,
					Bw42 = 3,
					Bw20 = 4,
					Bw10 = 5,
					Bw5 = 6
				};
			}
		}
	}
}

#endif