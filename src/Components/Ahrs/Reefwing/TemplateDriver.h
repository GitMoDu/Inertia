#ifndef _INERTIA_COMPONENTS_AHRS_REEFWING_TEMPLATE_DRIVER_h
#define _INERTIA_COMPONENTS_AHRS_REEFWING_TEMPLATE_DRIVER_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "../../../Framework/Interface.h"

#include <ReefwingAHRS.h> // https://github.com/Reefwing-Software/Reefwing-AHRS

namespace Inertia
{
	namespace Components
	{
		namespace Ahrs
		{
			namespace Reefwing
			{
				/// <summary>
				/// Reefwing AHRS orientation driver template.
				/// Provides orientation data, gravity removed acceleration,
				/// and AHRS aware angular velocity
				/// by aggregating sensor data from acceleration, angular velocity, and magnetometer sources.
				/// </summary>
				class TemplateDriver : public Inertia::Model::IPeriodicDriver
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_euler_angle_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_quaternion_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>
				{
				public:
					using DataTypes = Inertia::Components::Variadic::VariadicDataTypeList<
						Inertia::Model::timestamped_euler_angle_t,
						Inertia::Model::timestamped_quaternion_t,
						Inertia::Model::timestamped_acceleration_t,
						Inertia::Model::timestamped_angular_velocity_t>;

				private:
					enum class SensorModeEnum : uint8_t
					{
						AccelerometerOnly,
						GyroscopeOnly,
						AccelerometerAndGyroscope,
						AccelerometerGyroscopeAndMagnetometer,
						Invalid
					};

				private:
					// Sensor data source.
					Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* AccelerationSource;
					Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* AngularVelocitySource;
					Inertia::Model::IDataSource<Inertia::Model::timestamped_vector16_t>* MagnetometerSource;

					// Model native sensor data format.
					Inertia::Model::timestamped_acceleration_t AccelerationData{};
					Inertia::Model::timestamped_angular_velocity_t AngularVelocityData{};
					Inertia::Model::timestamped_magnet_t MagnetData{};
					Inertia::Model::timestamped_acceleration_t GravityRemovedAccelerationData{};
					Inertia::Model::timestamped_angular_velocity_t AhrsAngularVelocityData{};

					// Reefwing AHRS sensor data format.
					SensorData AhrsSensorData{};

					// Reefwing AHRS driver instance.
					ReefwingAHRS AhrsDriver{};

					// AHRS tracking state.
					uint32_t LastUpdateTimestamp = 0;
					bool AhrsDataAvailable = false;
					bool GravityRemovedAccelerationAvailable = false;
					bool AhrsAngularVelocityAvailable = false;

					const SensorModeEnum SensorMode;
					const BoardType Board;
					const ImuType Imu;

				public:
					TemplateDriver(Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* accelerationSource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* angularVelocitySource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_magnet_t>* magnetometerSource,
						BoardType boardType,
						ImuType imuType)
						: Model::IPeriodicDriver()
						, AccelerationSource(accelerationSource)
						, AngularVelocitySource(angularVelocitySource)
						, MagnetometerSource(magnetometerSource)
						, SensorMode(GetSensorMode(AccelerationSource != nullptr, AngularVelocitySource != nullptr, MagnetometerSource != nullptr))
						, Board(boardType)
						, Imu(imuType)
					{}

					bool Start() override
					{
						AhrsDataAvailable = false;
						GravityRemovedAccelerationAvailable = false;
						AhrsAngularVelocityAvailable = false;
						if (SensorMode == SensorModeEnum::Invalid)
						{
							return false;
						}

						DOF dof = DOF::DOF_6;
						SensorFusion fusion = SensorFusion::NONE;

						switch (SensorMode)
						{
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if (AngularVelocitySource == nullptr
								|| AccelerationSource == nullptr
								|| MagnetometerSource == nullptr)
							{
								return false;
							}
							dof = DOF::DOF_9;
							fusion = SensorFusion::MAHONY;
							break;

						case SensorModeEnum::AccelerometerOnly:
							if (AccelerationSource == nullptr)
							{
								return false;
							}
							dof = DOF::DOF_6;
							fusion = SensorFusion::CLASSIC;
							break;

						case SensorModeEnum::GyroscopeOnly:
							if (AngularVelocitySource == nullptr)
							{
								return false;
							}
							dof = DOF::DOF_6;
							fusion = SensorFusion::NONE;
							break;

						case SensorModeEnum::AccelerometerAndGyroscope:
							if (AngularVelocitySource == nullptr
								|| AccelerationSource == nullptr)
							{
								return false;
							}
							dof = DOF::DOF_6;
							fusion = SensorFusion::MAHONY;
							break;

						default:
							return false;
						}

						// Full 3D orientation. Yaw is relative to magnetic north, or initial heading if no magnetometer.
						AhrsDriver.reset();
						AhrsDriver.begin();
						AhrsDriver.setFusionAlgorithm(fusion);
						AhrsDriver.setBoardType(Board);
						AhrsDriver.setImuType(Imu);
						AhrsDriver.setDOF(dof);
						AhrsDriver.setKp(1.0f); // Mahony filter proportional gain
						AhrsDriver.setKi(0.001f);  // Mahony filter integral gain
						return true;
					}

					void Stop() override
					{
						AhrsDriver.reset();
						AhrsDataAvailable = false;
						GravityRemovedAccelerationAvailable = false;
						AhrsAngularVelocityAvailable = false;
					}

					void Step() final
					{
						bool dataReady = true;

						// Gather sensor data based on configured sensor mode.
						switch (SensorMode)
						{
						case SensorModeEnum::AccelerometerOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if (AccelerationSource != nullptr
								&& AccelerationSource->GetData(AccelerationData))
							{
								AhrsSensorData.ax = ScaleAcceleration(AccelerationData.x);
								AhrsSensorData.ay = ScaleAcceleration(AccelerationData.y);
								AhrsSensorData.az = ScaleAcceleration(AccelerationData.z);
								AhrsSensorData.aTimeStamp = AccelerationData.timestamp;
							}
							else
							{
								dataReady = false;
							}
							break;

						default:
							break;
						}

						switch (SensorMode)
						{
						case SensorModeEnum::GyroscopeOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if (AngularVelocitySource != nullptr
								&& AngularVelocitySource->GetData(AngularVelocityData))
							{
								AhrsSensorData.gx = ScaleAngularVelocity(AngularVelocityData.x);
								AhrsSensorData.gy = ScaleAngularVelocity(AngularVelocityData.y);
								AhrsSensorData.gz = ScaleAngularVelocity(AngularVelocityData.z);
								AhrsSensorData.gTimeStamp = AngularVelocityData.timestamp;
							}
							else
							{
								dataReady = false; // Flag missing data.
							}
							break;

						default:
							break;
						}

						switch (SensorMode)
						{
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if (MagnetometerSource != nullptr
								&& MagnetometerSource->GetData(MagnetData))
							{
								AhrsSensorData.mx = static_cast<float>(MagnetData.x);
								AhrsSensorData.my = static_cast<float>(MagnetData.y);
								AhrsSensorData.mz = static_cast<float>(MagnetData.z);
								AhrsSensorData.mTimeStamp = MagnetData.timestamp;
							}
							else
							{
								dataReady = false; // Flag missing data.
							}
							break;

						default:
							break;
						}

						if (dataReady)
						{
							// All required data is available to update AHRS.
							AhrsDriver.setData(AhrsSensorData);

							// Perform AHRS update.
							AhrsDriver.update();

							// Timestamp updated depending on sensor mode.
							switch (SensorMode)
							{
							case SensorModeEnum::AccelerometerOnly:
								// Timestamp only for accelerometer-only mode.
								LastUpdateTimestamp = AccelerationData.timestamp;
								break;

							case SensorModeEnum::GyroscopeOnly:
							case SensorModeEnum::AccelerometerAndGyroscope:
							case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
								// Gyro is the fastest sensor for timestamping.
								LastUpdateTimestamp = AngularVelocityData.timestamp;
								break;

							default:
								break;
							}

							UpdateGravityRemovedAcceleration();
							UpdateAhrsAngularVelocity();

							if (!AhrsDataAvailable)
							{
								// AHRS data is now available.
								AhrsDataAvailable = true;
							}
						}
						else
						{
							// Not enough data to update AHRS.
						}
					}

					bool GetData(Inertia::Model::timestamped_quaternion_t& data) final
					{
						if (AhrsDataAvailable)
						{
							auto quat = AhrsDriver.getQuaternion();
							data.w = Model::GetIntegerAngle(quat.q0);
							data.x = Model::GetIntegerAngle(quat.q1);
							data.y = Model::GetIntegerAngle(quat.q2);
							data.z = Model::GetIntegerAngle(quat.q3);
							data.timestamp = LastUpdateTimestamp;

							return true;
						}

						return false;
					}

					bool GetData(Inertia::Model::timestamped_euler_angle_t& data) final
					{
						if (AhrsDataAvailable)
						{
							data.yaw = Model::GetIntegerAngle(AhrsDriver.angles.yaw);
							data.pitch = Model::GetIntegerAngle(AhrsDriver.angles.pitch);
							data.roll = Model::GetIntegerAngle(AhrsDriver.angles.roll);
							data.timestamp = LastUpdateTimestamp;

							return true;
						}

						return false;
					}

					bool GetData(Inertia::Model::timestamped_acceleration_t& data) final
					{
						if (GravityRemovedAccelerationAvailable)
						{
							data = GravityRemovedAccelerationData;
							return true;
						}

						return false;
					}

					bool GetData(Inertia::Model::timestamped_angular_velocity_t& data) final
					{
						if (AhrsAngularVelocityAvailable)
						{
							data = AhrsAngularVelocityData;
							return true;
						}

						return false;
					}

				private:
					void UpdateGravityRemovedAcceleration()
					{
						if (!HasAccelerationSource())
						{
							GravityRemovedAccelerationAvailable = false;
							return;
						}

						auto quat = AhrsDriver.getQuaternion();

						const float gravityX = (2.0f * quat.q1 * quat.q3) - (2.0f * quat.q0 * quat.q2);
						const float gravityY = (2.0f * quat.q0 * quat.q1) + (2.0f * quat.q2 * quat.q3);
						const float gravityZ = (quat.q0 * quat.q0) - (quat.q1 * quat.q1) - (quat.q2 * quat.q2) + (quat.q3 * quat.q3);

						GravityRemovedAccelerationData.x = ScaleAccelerationInverse(AhrsSensorData.ax - gravityX);
						GravityRemovedAccelerationData.y = ScaleAccelerationInverse(AhrsSensorData.ay - gravityY);
						GravityRemovedAccelerationData.z = ScaleAccelerationInverse(AhrsSensorData.az - gravityZ);
						GravityRemovedAccelerationData.timestamp = LastUpdateTimestamp;
						GravityRemovedAccelerationAvailable = true;
					}

					void UpdateAhrsAngularVelocity()
					{
						if (!HasAngularVelocitySource())
						{
							AhrsAngularVelocityAvailable = false;
							return;
						}

						const float sinPhi = sin(AhrsDriver.angles.rollRadians);
						const float cosPhi = cos(AhrsDriver.angles.rollRadians);
						const float cosTheta = cos(AhrsDriver.angles.pitchRadians);
						const float tanTheta = tan(AhrsDriver.angles.pitchRadians);

						const float eulerRollRate = AhrsSensorData.gx + (sinPhi * tanTheta * AhrsSensorData.gy) + (cosPhi * tanTheta * AhrsSensorData.gz);
						const float eulerPitchRate = (cosPhi * AhrsSensorData.gy) - (sinPhi * AhrsSensorData.gz);
						float eulerYawRate = 0.0f;

						if (fabs(cosTheta) > 1e-6f)
						{
							eulerYawRate = ((sinPhi * AhrsSensorData.gy) / cosTheta) + ((cosPhi * AhrsSensorData.gz) / cosTheta);
						}

						AhrsAngularVelocityData.x = ScaleAngularVelocityInverse(eulerRollRate);
						AhrsAngularVelocityData.y = ScaleAngularVelocityInverse(eulerPitchRate);
						AhrsAngularVelocityData.z = ScaleAngularVelocityInverse(eulerYawRate);
						AhrsAngularVelocityData.timestamp = LastUpdateTimestamp;
						AhrsAngularVelocityAvailable = true;
					}

					static constexpr float ScaleAcceleration(const int16_t acceleration)
					{
						return static_cast<float>(acceleration) * Model::G_PER_ACCELERATION_UNIT;
					}

					static constexpr int16_t ScaleAccelerationInverse(const float acceleration)
					{
						const float scaledValue = acceleration / Model::G_PER_ACCELERATION_UNIT;

						if (scaledValue > 32767.0f)
						{
							return 32767;
						}
						else if (scaledValue < -32768.0f)
						{
							return -32768;
						}

						return static_cast<int16_t>(scaledValue >= 0.0f ? scaledValue + 0.5f : scaledValue - 0.5f);
					}

					static constexpr float ScaleAngularVelocity(const int32_t angularVelocity)
					{
						return static_cast<float>(angularVelocity) * Model::DEG_PER_ANGLE_UNIT;
					}

					static constexpr int32_t ScaleAngularVelocityInverse(const float angularVelocity)
					{
						const float scaledValue = angularVelocity / Model::DEG_PER_ANGLE_UNIT;

						if (scaledValue > 2147483647.0f)
						{
							return 2147483647;
						}
						else if (scaledValue < -2147483648.0f)
						{
							return (-2147483647 - 1);
						}

						return static_cast<int32_t>(scaledValue >= 0.0f ? scaledValue + 0.5f : scaledValue - 0.5f);
					}

					static constexpr bool HasAccelerationSource(const SensorModeEnum sensorMode)
					{
						switch (sensorMode)
						{
						case SensorModeEnum::AccelerometerOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							return true;

						default:
							return false;
						}
					}

					constexpr bool HasAccelerationSource() const
					{
						return HasAccelerationSource(SensorMode);
					}

					static constexpr bool HasAngularVelocitySource(const SensorModeEnum sensorMode)
					{
						switch (sensorMode)
						{
						case SensorModeEnum::GyroscopeOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							return true;

						default:
							return false;
						}
					}

					constexpr bool HasAngularVelocitySource() const
					{
						return HasAngularVelocitySource(SensorMode);
					}

					static SensorModeEnum GetSensorMode(const bool accelerometer, const bool gyroscope, const bool magnetometer)
					{
						if (accelerometer && gyroscope && magnetometer)
						{
							return SensorModeEnum::AccelerometerGyroscopeAndMagnetometer;
						}
						else if (accelerometer && gyroscope)
						{
							return SensorModeEnum::AccelerometerAndGyroscope;
						}
						else if (gyroscope)
						{
							return SensorModeEnum::GyroscopeOnly;
						}
						else if (accelerometer)
						{
							return SensorModeEnum::AccelerometerOnly;
						}
						else
						{
							return SensorModeEnum::Invalid;
						}
					}
				};
			}
		}
	}
}
#endif
#endif