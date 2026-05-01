#ifndef _INERTIA_DRIVERS_LSM6DS3_WRAPPER_h
#define _INERTIA_DRIVERS_LSM6DS3_WRAPPER_h

#include "../../Components/Core/Primitives.h"

#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense)
#include <LSM6DS3.h> // SparkFun driver included in the Seeed XIAO nRF52840 Sense core https://github.com/Seeed-Studio/OSHW-XIAO-Series
//#else
//#include <SparkFunLSM6DS3.h> // https://github.com/sparkfun/SparkFun_LSM6DS3_Arduino_Library
//#endif

namespace Inertia
{
	namespace Drivers
	{
		namespace Imu
		{
			namespace LSM6DS3Wrapper
			{
				enum class AccelerometerBandwidthEnum : uint16_t
				{
					Bandwidth50Hz = 50,
					Bandwidth100Hz = 100,
					Bandwidth200Hz = 200,
					Bandwidth400Hz = 400
				};

				enum class AccelerometerRangeEnum : uint16_t
				{
					Range2g = 2,
					Range4g = 4,
					Range8g = 8,
					Range16g = 16
				};

				enum class AccelerometerSampleRateEnum : uint16_t
				{
					SampleRate13Hz = 13,
					SampleRate26Hz = 26,
					SampleRate52Hz = 52,
					SampleRate104Hz = 104,
					SampleRate208Hz = 208,
					SampleRate416Hz = 416,
					SampleRate833Hz = 833,
					SampleRate1660Hz = 1660,
					SampleRate3330Hz = 3330,
					SampleRate6660Hz = 6660,
					SampleRate13330Hz = 13330
				};

				enum class GyroscopeRangeEnum : uint16_t
				{
					Range125dps = 125,
					Range245dps = 245,
					Range500dps = 500,
					Range1000dps = 1000,
					Range2000dps = 2000
				};

				enum class GyroscopeSampleRateEnum : uint16_t
				{
					SampleRate13Hz = 13,
					SampleRate26Hz = 26,
					SampleRate52Hz = 52,
					SampleRate104Hz = 104,
					SampleRate208Hz = 208,
					SampleRate416Hz = 416,
					SampleRate833Hz = 833,
					SampleRate1660Hz = 1660
				};

				static constexpr uint8_t DEVICE_ADDRESS = 0x6A;

#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense)
				static constexpr uint32_t CLOCK_SPEED_I2C = 1000000;
#else
				static constexpr uint32_t CLOCK_SPEED_I2C = 400000;
#endif

				/// <summary>
				/// Driver for LSM6DS3 IMU. Manages the LSM6DS3 sensor, samples acceleration and gyro data, and exposes the latest timestamped sample to callers.
				/// Wraps SparkFun LSM6DS3 library around Model::IPeriodicDriver interface and exposes the latest IMU data.
				/// Fixed use of Wire1 for nRF52840 Sense and Wire for other platforms.
				/// </summary>
				template<uint32_t ClockSpeedI2C = CLOCK_SPEED_I2C,
					AccelerometerBandwidthEnum accelerometerBandwidth = AccelerometerBandwidthEnum::Bandwidth100Hz,
					AccelerometerSampleRateEnum accelerometerSampleRate = AccelerometerSampleRateEnum::SampleRate416Hz,
					AccelerometerRangeEnum accelerometerRange = AccelerometerRangeEnum::Range16g,
					GyroscopeRangeEnum gyroscopeRange = GyroscopeRangeEnum::Range2000dps,
					GyroscopeSampleRateEnum gyroscopeSampleRate = GyroscopeSampleRateEnum::SampleRate416Hz>
				class TemplateDriver : public Model::IPeriodicDriver,
					public Model::IDataSource<Model::timestamped_acceleration_t>,
					public Model::IDataSource<Model::timestamped_angular_velocity_t>,
					public Model::IDataSource<Model::timestamped_temperature_t>
				{
				public:
					using DataTypes = Inertia::Components::Variadic::VariadicDataTypeList<
						Inertia::Model::timestamped_acceleration_t,
						Inertia::Model::timestamped_angular_velocity_t,
						Inertia::Model::timestamped_temperature_t>;

				private:
					Model::timestamped_acceleration_t AccelerationData{};
					Model::timestamped_angular_velocity_t AngularVelocityData{};
					Model::timestamped_temperature_t TemperatureData{};

					scale16_t GyroScaleFactor = 0;
					scale16_t AccelScaleFactor = 0;
					scale16_t TemperatureScaleFactor = 0;

					static constexpr int16_t KELVIN_OFFSET = 27315; // Offset to convert Celsius to Kelvin in centi-degrees.
					static constexpr int16_t TEMPERATURE_OFFSET = 2500; // Offset to apply to raw temperature readings to get degrees Celsius.
					static constexpr int16_t TEMPERATURE_CONVERSION_OFFSET = (KELVIN_OFFSET + TEMPERATURE_OFFSET);

					bool ImuDataAvailable = false;

				private:
					LSM6DS3 Sensor;

				public:
					TemplateDriver()
						: Model::IPeriodicDriver()
						, Model::IDataSource<Model::timestamped_acceleration_t>()
						, Model::IDataSource<Model::timestamped_angular_velocity_t>()
						, Sensor(I2C_MODE, DEVICE_ADDRESS)
					{}

					bool GetData(Model::timestamped_acceleration_t& data) final
					{
						if (ImuDataAvailable)
						{
							memcpy(&data, &AccelerationData, sizeof(Model::timestamped_acceleration_t));

							return true;
						}

						return false;
					}

					bool GetData(Model::timestamped_temperature_t& data) final
					{
						if (ImuDataAvailable)
						{
							memcpy(&data, &TemperatureData, sizeof(Model::timestamped_temperature_t));

							return true;
						}
						return false;
					}

					bool GetData(Model::timestamped_angular_velocity_t& data) final
					{
						if (ImuDataAvailable)
						{
							memcpy(&data, &AngularVelocityData, sizeof(Model::timestamped_angular_velocity_t));

							return true;
						}

						return false;
					}

					bool Start() final
					{
						ImuDataAvailable = false;

						Wire1.setClock(ClockSpeedI2C);

						Sensor.settings.commMode = 1; // I2C mode
						Sensor.settings.timestampEnabled = 1; // Enable timestamp

						Sensor.settings.accelEnabled = 1;
						Sensor.settings.accelSampleRate = uint16_t(accelerometerSampleRate);
						Sensor.settings.accelRange = uint16_t(accelerometerRange);
						Sensor.settings.accelBandWidth = uint16_t(accelerometerBandwidth);

						Sensor.settings.gyroEnabled = 1;
						Sensor.settings.gyroRange = uint16_t(gyroscopeRange);
						Sensor.settings.gyroSampleRate = uint16_t(gyroscopeSampleRate);
						Sensor.settings.accelFifoEnabled = 0;
						Sensor.settings.gyroFifoEnabled = 0;
						Sensor.settings.timestampFifoEnabled = 0;

						Sensor.settings.tempEnabled = 1;

						if (Sensor.begin() == 0)
						{
							uint8_t gyroRangeDivisor = Sensor.settings.gyroRange / 125;

							if (Sensor.settings.gyroRange == 245)
							{
								gyroRangeDivisor = 2;
							}

							// Calculate scale factors based on configured ranges.
							GyroScaleFactor = Scale16::GetFactor<int32_t>(ANGLE_RANGE, static_cast<int32_t>(360) * gyroRangeDivisor);
							AccelScaleFactor = Scale16::GetFactor<int32_t>(61LL * (Sensor.settings.accelRange >> 1), 1000);
							TemperatureScaleFactor = Scale16::GetFactor<int32_t>(100, Sensor.settings.tempSensitivity);

							return true;
						}

						return false;
					}

					void Stop() final
					{
						ImuDataAvailable = false;
					}

					void Step() final
					{
						const uint32_t timestamp = micros();

						// Ensure I2C clock speed is set correctly in case other drivers have changed it.
						Wire1.setClock(ClockSpeedI2C);

						// Convert accelerometer readings to acceleration_t units (1/1000 g per precision).
						AccelerationData.x = static_cast<int16_t>(Scale(AccelScaleFactor, Sensor.readRawAccelX()));
						AccelerationData.y = static_cast<int16_t>(Scale(AccelScaleFactor, Sensor.readRawAccelY()));
						AccelerationData.z = static_cast<int16_t>(Scale(AccelScaleFactor, Sensor.readRawAccelZ()));
						AccelerationData.timestamp = timestamp;

						// Convert gyro readings to angle_t units (0.05 degrees per precision).
						AngularVelocityData.x = Scale(GyroScaleFactor, static_cast<int32_t>(Sensor.readRawGyroX()));
						AngularVelocityData.y = Scale(GyroScaleFactor, static_cast<int32_t>(Sensor.readRawGyroY()));
						AngularVelocityData.z = Scale(GyroScaleFactor, static_cast<int32_t>(Sensor.readRawGyroZ()));
						AngularVelocityData.timestamp = timestamp;

						// Convert temperature to centi-degrees Kelvin.
						TemperatureData.temperature = static_cast<int16_t>(
							LimitValue<int32_t, 0, UINT16_MAX>(Scale(TemperatureScaleFactor, static_cast<int32_t>(Sensor.readRawTemp()))
								+ TEMPERATURE_CONVERSION_OFFSET));
						TemperatureData.timestamp = timestamp;

						if (!ImuDataAvailable)
						{
							ImuDataAvailable = true;
						}
					}
				};
			}
		}
	}
}
#endif
#endif