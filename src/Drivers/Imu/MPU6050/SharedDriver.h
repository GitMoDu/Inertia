#ifndef _INERTIA_DRIVERS_IMU_MPU6050_SHARED_DRIVER_h
#define _INERTIA_DRIVERS_IMU_MPU6050_SHARED_DRIVER_h

#include "Model.h"
#include "DeviceDriver.h"

#include "../../../Components/Core/DataSource/Model.h"
#include "../../../Components/Core/Lifecycle/Model.h"

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

				/// <summary>
				/// Driver for MPU6050 IMU. Manages the MPU6050 sensor, samples acceleration, gyro, and temperature data, and exposes the latest timestamped sample to callers.
				/// Uses the local MPU6050 I2C device driver around Model::IPeriodicDriver and exposes the latest IMU data.
				/// </summary>
				/// <typeparam name="Address">I2C device address.</typeparam>
				/// <typeparam name="accelerometerRange">Accelerometer full-scale range.</typeparam>
				/// <typeparam name="gyroscopeRange">Gyroscope full-scale range.</typeparam>
				/// <typeparam name="dlpfMode">Digital low-pass filter mode.</typeparam>
				template<typename WireType = TwoWire,
					uint8_t Address = Device::DEVICE_ADDRESS_LOW,
					AccelerometerRangeEnum accelerometerRange = AccelerometerRangeEnum::Range16g,
					GyroscopeRangeEnum gyroscopeRange = GyroscopeRangeEnum::Range2000dps,
					DlpfModeEnum dlpfMode = DlpfModeEnum::Bw98,
					uint32_t RecoveryTimeoutMillis = 10>
				class SharedDriver
				{
				private:
					static constexpr int16_t TEMPERATURE_SENSITIVITY = 340;   // LSB per degree Celsius.
					static constexpr int16_t TEMPERATURE_OFFSET_CENTI_DEG = 3653;  // 36.53 degrees Celsius in centi-degrees.
					static constexpr int16_t TEMPERATURE_KELVIN_OFFSET = 27315; // 273.15 degrees Celsius in centi-degrees.
					static constexpr int16_t TEMPERATURE_CONVERSION_OFFSET = TEMPERATURE_KELVIN_OFFSET + TEMPERATURE_OFFSET_CENTI_DEG;

					static constexpr uint8_t SETUP_TIMEOUT_MS = 4;
					static constexpr uint8_t READ_TIMEOUT_MS = 2;

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0; // Optional instance ID for distinguishing between multiple IMU instances in logs.

				private:
					WireType& WireInstance;
					Device::Driver<WireType> DeviceDriver;
					const AccelerometerRangeEnum AccelRange;
					const GyroscopeRangeEnum GyroRange;
					const DlpfModeEnum DlpfMode;

					scale16_t AccelScaleFactor = 0;
					scale16_t GyroScaleFactor = 0;
					scale16_t TempScaleFactor = 0;

					Device::RawSample RawData{};
					Model::timestamped_acceleration_t AccelerationData{};
					Model::timestamped_angular_velocity_t AngularVelocityData{};
					Model::timestamped_temperature_t TemperatureData{};

					uint32_t LastSampleTimestamp = 0;
					uint32_t LastErrorTimestamp = 0;
					uint8_t StartRefCount = 0;
					uint8_t RecoveryCount = 0;
					bool HasLoggedReadError = false;
					bool ShouldLogReadFailure = false;
					bool HasSampleAvailable = false;
					bool MotionDataAvailable = false;
					bool TemperatureDataAvailable = false;

				public:
					SharedDriver(WireType& wire)
						: WireInstance(wire)
						, DeviceDriver(wire, Address)
						, AccelRange(accelerometerRange)
						, GyroRange(gyroscopeRange)
						, DlpfMode(dlpfMode)
					{}

					bool Start()
					{
						if (StartRefCount > 0)
						{
							StartRefCount++;
							return true;
						}

						MotionDataAvailable = false;
						TemperatureDataAvailable = false;
						HasSampleAvailable = false;

#if defined(ARDUINO_ARCH_RP2040)
						WireInstance.setTimeout(SETUP_TIMEOUT_MS, false);
#endif

						if (!DeviceDriver.initialize()
							|| !DeviceDriver.testConnection()
							|| !DeviceDriver.setFullScaleAccelRange(uint8_t(AccelRange))
							|| !DeviceDriver.setFullScaleGyroRange(uint8_t(GyroRange))
							|| !DeviceDriver.setDLPFMode(uint8_t(DlpfMode)))
						{
							if (LogListener != nullptr)
							{
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = InstanceId,
									.Type = Inertia::Model::LogTypeEnum::Error,
									.Code = static_cast<uint8_t>(LogCodeEnum::ErrorBoot),
									.Value = 0
									});
							}
							return false;
						}

						AccelScaleFactor = Scale16::GetFactor<int32_t>(61LL * (1u << uint8_t(AccelRange)), 1000);
						const int32_t gyroRangeDivisor = 2 * (1 << uint8_t(GyroRange));
						GyroScaleFactor = Scale16::GetFactor<int32_t>(ANGLE_RANGE, static_cast<int32_t>(360) * gyroRangeDivisor);
						TempScaleFactor = Scale16::GetFactor<int32_t>(100, TEMPERATURE_SENSITIVITY);
						StartRefCount = 1;

						return true;
					}

					void Stop()
					{
						if (StartRefCount == 0)
						{
							return;
						}

						StartRefCount--;
						if (StartRefCount > 0)
						{
							return;
						}

						MotionDataAvailable = false;
						TemperatureDataAvailable = false;
						HasSampleAvailable = false;
					}

					bool StepMotion()
					{
						if (!EnsureSample(true))
						{
							LogReadFailure(LogCodeEnum::ErrorReadMotion);
							MotionDataAvailable = false;
							return false;
						}

						AccelerationData.x = static_cast<int16_t>(Scale(AccelScaleFactor, RawData.accelerationX));
						AccelerationData.y = static_cast<int16_t>(Scale(AccelScaleFactor, RawData.accelerationY));
						AccelerationData.z = static_cast<int16_t>(Scale(AccelScaleFactor, RawData.accelerationZ));
						AccelerationData.timestamp = LastSampleTimestamp;

						AngularVelocityData.x = Scale(GyroScaleFactor, static_cast<int32_t>(RawData.gyroscopeX));
						AngularVelocityData.y = Scale(GyroScaleFactor, static_cast<int32_t>(RawData.gyroscopeY));
						AngularVelocityData.z = Scale(GyroScaleFactor, static_cast<int32_t>(RawData.gyroscopeZ));
						AngularVelocityData.timestamp = LastSampleTimestamp;

						MotionDataAvailable = true;
						return true;
					}

					bool StepTemperature()
					{
						return StepTemperature(false);
					}

					bool StepTemperatureWithRefresh()
					{
						return StepTemperature(true);
					}

				private:
					bool StepTemperature(const bool forceRefresh)
					{
						if (!EnsureSample(forceRefresh))
						{
							if (ShouldLogReadFailure)
							{
								LogReadFailure(LogCodeEnum::ErrorReadTemperature);
							}

							TemperatureDataAvailable = false;
							return false;
						}

						TemperatureData.temperature = static_cast<int16_t>(
							LimitValue<int32_t, 0, UINT16_MAX>(Scale(TempScaleFactor, static_cast<int32_t>(RawData.temperature))
								+ TEMPERATURE_CONVERSION_OFFSET));
						TemperatureData.timestamp = LastSampleTimestamp;

						TemperatureDataAvailable = true;
						return true;
					}

				public:
					bool GetAccelerationData(Model::timestamped_acceleration_t& data) const
					{
						if (!MotionDataAvailable)
						{
							return false;
						}

						memcpy(&data, &AccelerationData, sizeof(Model::timestamped_acceleration_t));
						return true;
					}

					bool GetAngularVelocityData(Model::timestamped_angular_velocity_t& data) const
					{
						if (!MotionDataAvailable)
						{
							return false;
						}

						memcpy(&data, &AngularVelocityData, sizeof(Model::timestamped_angular_velocity_t));
						return true;
					}

					bool GetTemperatureData(Model::timestamped_temperature_t& data) const
					{
						if (!TemperatureDataAvailable)
						{
							return false;
						}

						memcpy(&data, &TemperatureData, sizeof(Model::timestamped_temperature_t));
						return true;
					}

				private:
					bool EnsureSample(const bool forceRefresh)
					{
						if (HasSampleAvailable && !forceRefresh)
						{
							ShouldLogReadFailure = false;
							return true;
						}

						LastSampleTimestamp = micros();

#if defined(ARDUINO_ARCH_RP2040)
						WireInstance.setTimeout(READ_TIMEOUT_MS, true);
#endif

						if (!DeviceDriver.getSample(RawData))
						{
							ShouldLogReadFailure = OnError();
							return false;
						}

						HasLoggedReadError = false;
						ShouldLogReadFailure = false;
						HasSampleAvailable = true;
						return true;
					}

					void LogReadFailure(const LogCodeEnum code)
					{
						if (LogListener != nullptr)
						{
							LogListener->OnLog(Inertia::Model::LogEntryStruct{
								.Tag = LOG_TAG,
								.Instance = InstanceId,
								.Type = Inertia::Model::LogTypeEnum::Warning,
								.Code = static_cast<uint8_t>(code),
								.Value = 0
								});
						}
					}

					bool OnError()
					{
						HasSampleAvailable = false;
						MotionDataAvailable = false;
						TemperatureDataAvailable = false;

						const uint32_t timestamp = millis();
						const uint32_t elapsedSinceLastError = timestamp - LastErrorTimestamp;
						LastErrorTimestamp = timestamp;

						if (!HasLoggedReadError)
						{
							HasLoggedReadError = true;
							return true;
						}
						else if (elapsedSinceLastError < RecoveryTimeoutMillis)
						{
							RecoveryCount++;
							if (LogListener != nullptr)
							{
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = InstanceId,
									.Type = Inertia::Model::LogTypeEnum::Warning,
									.Code = static_cast<uint8_t>(LogCodeEnum::RecoveryAttempt),
									.Value = RecoveryCount
									});
							}

							Inertia::Drivers::HardwareInterface::I2c::Drivers::RecoverInterface(WireInstance);
							HasLoggedReadError = false; // Reset error logging after recovery attempt.
							return false;
						}

						return false;
					}
				};
			}
		}
	}
}

#endif