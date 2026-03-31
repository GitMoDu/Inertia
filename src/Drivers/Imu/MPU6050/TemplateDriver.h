#ifndef _INERTIA_DRIVERS_IMU_MPU6050_TEMPLATE_DRIVER_h
#define _INERTIA_DRIVERS_IMU_MPU6050_TEMPLATE_DRIVER_h

#include "../../../Framework/Model.h"
#include "DeviceDriver.h"
#include <Wire.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Imu
		{
			namespace Mpu6050
			{
				using namespace IntegerSignal;
				using namespace IntegerSignal::FixedPoint::FactorScale;

				static constexpr uint32_t CLOCK_SPEED_I2C = 400000;

				static constexpr uint8_t SETUP_TIMEOUT_MS = 4;
				static constexpr uint8_t READ_TIMEOUT_MS = 1;

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

				/// <summary>
				/// Driver for MPU6050 IMU. Manages the MPU6050 sensor, samples acceleration, gyro, and temperature data, and exposes the latest timestamped sample to callers.
			 /// Uses the local MPU6050 I2C device driver around Model::IPeriodicDriver and exposes the latest IMU data.
				/// </summary>
				/// <typeparam name="Address">I2C device address. Use DEVICE_ADDRESS_LOW (AD0 low) or DEVICE_ADDRESS_HIGH (AD0 high).</typeparam>
				/// <typeparam name="accelerometerRange">Accelerometer full-scale range.</typeparam>
				/// <typeparam name="gyroscopeRange">Gyroscope full-scale range.</typeparam>
				/// <typeparam name="dlpfMode">Digital low-pass filter mode.</typeparam>
				template<uint8_t Address = Device::DEVICE_ADDRESS_LOW,
					AccelerometerRangeEnum accelerometerRange = AccelerometerRangeEnum::Range16g,
					GyroscopeRangeEnum gyroscopeRange = GyroscopeRangeEnum::Range2000dps,
					DlpfModeEnum dlpfMode = DlpfModeEnum::Bw42>
				class TemplateDriver : public Model::IPeriodicDriver,
					public Model::IDataSource<Model::timestamped_acceleration_t>,
					public Model::IDataSource<Model::timestamped_angular_velocity_t>,
					public Model::IDataSource<Model::timestamped_temperature_t>
				{
				public:
					using DataTypes = Inertia::Drivers::Variadic::VariadicDataTypeList<
						Inertia::Model::timestamped_acceleration_t,
						Inertia::Model::timestamped_angular_velocity_t,
						Inertia::Model::timestamped_temperature_t>;

				private:
					static constexpr int16_t TEMPERATURE_SENSITIVITY = 340;   // LSB per degree Celsius.
					static constexpr int16_t TEMPERATURE_OFFSET_CENTI_DEG = 3653;  // 36.53 degrees Celsius in centi-degrees.
					static constexpr int16_t TEMPERATURE_KELVIN_OFFSET = 27315; // 273.15 degrees Celsius in centi-degrees.
					static constexpr int16_t TEMPERATURE_CONVERSION_OFFSET = TEMPERATURE_KELVIN_OFFSET + TEMPERATURE_OFFSET_CENTI_DEG;

				private:
					TwoWire& WireInstance;

					using DeviceDriverType = Device::Driver;
					DeviceDriverType DeviceDriver;

					AccelerometerRangeEnum AccelRange;
					GyroscopeRangeEnum GyroRange;
					DlpfModeEnum DlpfMode;

					Model::timestamped_acceleration_t AccelerationData{};
					Model::timestamped_angular_velocity_t AngularVelocityData{};
					Model::timestamped_temperature_t TemperatureData{};

					scale16_t AccelScaleFactor = 0;
					scale16_t GyroScaleFactor = 0;
					scale16_t TempScaleFactor = 0;
					bool ImuDataAvailable = false;

				public:
					TemplateDriver(TwoWire& wire = Wire)
						: Model::IPeriodicDriver()
						, Model::IDataSource<Model::timestamped_acceleration_t>()
						, Model::IDataSource<Model::timestamped_angular_velocity_t>()
						, Model::IDataSource<Model::timestamped_temperature_t>()
						, WireInstance(wire)
						, DeviceDriver(wire, Address)
						, AccelRange(accelerometerRange)
						, GyroRange(gyroscopeRange)
						, DlpfMode(dlpfMode)
					{}

					bool GetData(Model::timestamped_acceleration_t& data) final
					{
						if (!ImuDataAvailable) { return false; }
						memcpy(&data, &AccelerationData, sizeof(Model::timestamped_acceleration_t));
						return true;
					}

					bool GetData(Model::timestamped_angular_velocity_t& data) final
					{
						if (!ImuDataAvailable) { return false; }
						memcpy(&data, &AngularVelocityData, sizeof(Model::timestamped_angular_velocity_t));
						return true;
					}

					bool GetData(Model::timestamped_temperature_t& data) final
					{
						if (!ImuDataAvailable) { return false; }
						memcpy(&data, &TemperatureData, sizeof(Model::timestamped_temperature_t));
						return true;
					}

					bool Start() final
					{
						ImuDataAvailable = false;

						WireInstance.setClock(CLOCK_SPEED_I2C);

#if defined(ARDUINO_ARCH_RP2040)
						WireInstance.setTimeout(SETUP_TIMEOUT_MS, false);
#endif

						if (!DeviceDriver.initialize()
							|| !DeviceDriver.testConnection()
							|| !DeviceDriver.setFullScaleAccelRange(uint8_t(AccelRange))
							|| !DeviceDriver.setFullScaleGyroRange(uint8_t(GyroRange))
							|| !DeviceDriver.setDLPFMode(uint8_t(DlpfMode)))
						{
							return false;
						}

						// Accel sensitivity: 61 µg/LSB for 2 g, doubling per range step.
						AccelScaleFactor = Scale16::GetFactor<int32_t>(61LL * (1u << uint8_t(AccelRange)), 1000);

						// Gyro divisor: 250 dps -> 2, 500 dps -> 4, 1000 dps -> 8, 2000 dps -> 16.
						const int32_t gyroRangeDivisor = 2 * (1 << uint8_t(GyroRange));
						GyroScaleFactor = Scale16::GetFactor<int32_t>(ANGLE_RANGE, static_cast<int32_t>(360) * gyroRangeDivisor);

						TempScaleFactor = Scale16::GetFactor<int32_t>(100, TEMPERATURE_SENSITIVITY);

						return true;
					}

					void Stop() final
					{
						ImuDataAvailable = false;
					}

					void Step() final
					{
						const uint32_t timestamp = micros();

						// Ensure I2C clock speed is set correctly in case other drivers have changed it.
						WireInstance.setClock(CLOCK_SPEED_I2C);

#if defined(ARDUINO_ARCH_RP2040)
						WireInstance.setTimeout(READ_TIMEOUT_MS, true);
#endif

						// Scope the motion data reading to avoid stack usage of the raw data variables.
						{
							int16_t ax, ay, az, gx, gy, gz;
							if (!DeviceDriver.getMotion6(&ax, &ay, &az, &gx, &gy, &gz))
							{
								//Serial.println("Failed to read motion data from MPU6050.");
								RecoverI2c();
								return;
							}

							// Convert accelerometer readings to acceleration_t units (1/1000 g per precision).
							AccelerationData.x = static_cast<int16_t>(Scale(AccelScaleFactor, ax));
							AccelerationData.y = static_cast<int16_t>(Scale(AccelScaleFactor, ay));
							AccelerationData.z = static_cast<int16_t>(Scale(AccelScaleFactor, az));
							AccelerationData.timestamp = timestamp;

							// Convert gyro readings to angle_t units per second.
							AngularVelocityData.x = Scale(GyroScaleFactor, static_cast<int32_t>(gx));
							AngularVelocityData.y = Scale(GyroScaleFactor, static_cast<int32_t>(gy));
							AngularVelocityData.z = Scale(GyroScaleFactor, static_cast<int32_t>(gz));
							AngularVelocityData.timestamp = timestamp;
						}


						int16_t rawTemp = 0;
						if (!DeviceDriver.getTemperature(rawTemp))
						{
							//Serial.println("Failed to read temperature data from MPU6050.");
							RecoverI2c();
							return;
						}

						// Convert temperature to centi-Kelvin.
						TemperatureData.temperature = static_cast<int16_t>(
							LimitValue<int32_t, 0, UINT16_MAX>(Scale(TempScaleFactor, static_cast<int32_t>(rawTemp))
								+ TEMPERATURE_CONVERSION_OFFSET));
						TemperatureData.timestamp = timestamp;

						if (!ImuDataAvailable)
						{
							ImuDataAvailable = true;
						}
					}

				private:
					void RecoverI2c()
					{
						// Attempt to recover the I2C bus by sending a stop condition and clearing any error states.
						WireInstance.endTransmission();
#if defined(ARDUINO_ARCH_RP2040)
						WireInstance.clearTimeoutFlag();
						WireInstance.abortAsync();
						WireInstance.clearWriteError();
#endif
					}
				};
			}
		}
	}
}

#endif