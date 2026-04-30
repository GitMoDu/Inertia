#ifndef _INERTIA_COMPONENTS_AHRS_XIO_TEMPLATE_DRIVER_h
#define _INERTIA_COMPONENTS_AHRS_XIO_TEMPLATE_DRIVER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "../../../Framework/Interface.h"

// https://github.com/xioTechnologies/Fusion
#ifdef __cplusplus
extern "C"
{
#endif
#include "Source/FusionAhrs.h"
#include "Source/FusionBias.h"
#ifdef __cplusplus
}
#endif

namespace Inertia
{
	namespace Components
	{
		namespace Ahrs
		{
			namespace Xio
			{
				/// <summary>
				/// Xio AHRS orientation driver template.
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

					static constexpr float DefaultGyroscopeRangeDegreesPerSecond = 2000.0f;
					static constexpr float MicrosecondsToSeconds = 0.000001f;
					static constexpr float MinimumBiasSampleRate = 1.0f;

				private:
					Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* AccelerationSource;
					Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* AngularVelocitySource;
					Inertia::Model::IDataSource<Inertia::Model::timestamped_magnet_t>* MagnetometerSource;

					Inertia::Model::timestamped_acceleration_t AccelerationData{};
					Inertia::Model::timestamped_angular_velocity_t AngularVelocityData{};
					Inertia::Model::timestamped_magnet_t MagnetData{};
					Inertia::Model::timestamped_acceleration_t GravityRemovedAccelerationData{};
					Inertia::Model::timestamped_angular_velocity_t AhrsAngularVelocityData{};

					FusionAhrs AhrsDriver{};
					FusionBias BiasDriver{};

					uint32_t LastUpdateTimestamp = 0;
					uint32_t PreviousUpdateTimestamp = 0;
					bool AhrsDataAvailable = false;
					bool GravityRemovedAccelerationAvailable = false;
					bool AhrsAngularVelocityAvailable = false;

					const SensorModeEnum SensorMode;
					const float GyroscopeRangeDegreesPerSecond;
					const float Gain;

				public:
					TemplateDriver(Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* accelerationSource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* angularVelocitySource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_magnet_t>* magnetometerSource = nullptr,
						const float gyroscopeRangeDegreesPerSecond = DefaultGyroscopeRangeDegreesPerSecond,
						const float gain = fusionAhrsDefaultSettings.gain)
						: Model::IPeriodicDriver()
						, AccelerationSource(accelerationSource)
						, AngularVelocitySource(angularVelocitySource)
						, MagnetometerSource(magnetometerSource)
						, SensorMode(GetSensorMode(AccelerationSource != nullptr, AngularVelocitySource != nullptr, MagnetometerSource != nullptr))
						, GyroscopeRangeDegreesPerSecond(gyroscopeRangeDegreesPerSecond)
						, Gain(gain)
					{}

					bool Start() override
					{
						AhrsDataAvailable = false;
						GravityRemovedAccelerationAvailable = false;
						AhrsAngularVelocityAvailable = false;
						LastUpdateTimestamp = 0;
						PreviousUpdateTimestamp = 0;

						if ((SensorMode == SensorModeEnum::Invalid)
							|| (SensorMode == SensorModeEnum::AccelerometerOnly))
						{
							return false;
						}

						FusionAhrsInitialise(&AhrsDriver);
						FusionBiasInitialise(&BiasDriver);

						FusionAhrsSettings settings = fusionAhrsDefaultSettings;
						settings.convention = FusionConventionNwu;
						settings.gyroscopeRange = GyroscopeRangeDegreesPerSecond;
						settings.gain = HasAccelerationSource() ? Gain : 0.0f;
						FusionAhrsSetSettings(&AhrsDriver, &settings);

						return true;
					}

					void Stop() override
					{
						AhrsDataAvailable = false;
						GravityRemovedAccelerationAvailable = false;
						AhrsAngularVelocityAvailable = false;
						LastUpdateTimestamp = 0;
						PreviousUpdateTimestamp = 0;
					}

					void Step() final
					{
						bool dataReady = true;

						FusionVector gyroscope = FUSION_VECTOR_ZERO;
						FusionVector accelerometer = FUSION_VECTOR_ZERO;
						FusionVector magnetometer = FUSION_VECTOR_ZERO;

						switch (SensorMode)
						{
						case SensorModeEnum::AccelerometerOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if ((AccelerationSource != nullptr)
								&& AccelerationSource->GetData(AccelerationData))
							{
								accelerometer.axis.x = ScaleAcceleration(AccelerationData.x);
								accelerometer.axis.y = ScaleAcceleration(AccelerationData.y);
								accelerometer.axis.z = ScaleAcceleration(AccelerationData.z);
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
							if ((AngularVelocitySource != nullptr)
								&& AngularVelocitySource->GetData(AngularVelocityData))
							{
								gyroscope.axis.x = ScaleAngularVelocity(AngularVelocityData.x);
								gyroscope.axis.y = ScaleAngularVelocity(AngularVelocityData.y);
								gyroscope.axis.z = ScaleAngularVelocity(AngularVelocityData.z);
							}
							else
							{
								dataReady = false;
							}
							break;

						default:
							break;
						}

						if (SensorMode == SensorModeEnum::AccelerometerGyroscopeAndMagnetometer)
						{
							if ((MagnetometerSource != nullptr)
								&& MagnetometerSource->GetData(MagnetData))
							{
								magnetometer.axis.x = static_cast<float>(MagnetData.x);
								magnetometer.axis.y = static_cast<float>(MagnetData.y);
								magnetometer.axis.z = static_cast<float>(MagnetData.z);
							}
							else
							{
								dataReady = false;
							}
						}

						if (!dataReady)
						{
							return;
						}

						const uint32_t currentTimestamp = HasAngularVelocitySource()
							? AngularVelocityData.timestamp
							: AccelerationData.timestamp;

						LastUpdateTimestamp = currentTimestamp;

						if (PreviousUpdateTimestamp == 0)
						{
							PreviousUpdateTimestamp = currentTimestamp;
							return;
						}

						const uint32_t deltaTimeMicros = currentTimestamp - PreviousUpdateTimestamp;
						PreviousUpdateTimestamp = currentTimestamp;

						if (deltaTimeMicros == 0)
						{
							return;
						}

						const float deltaTimeSeconds = static_cast<float>(deltaTimeMicros) * MicrosecondsToSeconds;
						FusionBiasSettings biasSettings = fusionBiasDefaultSettings;
						const float biasSampleRate = deltaTimeSeconds > 0.0f ? (1.0f / deltaTimeSeconds) : MinimumBiasSampleRate;
						biasSettings.sampleRate = biasSampleRate > MinimumBiasSampleRate ? biasSampleRate : MinimumBiasSampleRate;
						FusionBiasSetSettings(&BiasDriver, &biasSettings);
						gyroscope = FusionBiasUpdate(&BiasDriver, gyroscope);

						if (SensorMode == SensorModeEnum::AccelerometerGyroscopeAndMagnetometer)
						{
							FusionAhrsUpdate(&AhrsDriver, gyroscope, accelerometer, magnetometer, deltaTimeSeconds);
						}
						else
						{
							FusionAhrsUpdateNoMagnetometer(&AhrsDriver, gyroscope, accelerometer, deltaTimeSeconds);
						}

						UpdateGravityRemovedAcceleration();
						UpdateAhrsAngularVelocity(gyroscope);
						AhrsDataAvailable = true;
					}

					bool GetData(Inertia::Model::timestamped_quaternion_t& data) final
					{
						if (AhrsDataAvailable)
						{
							const auto quat = FusionAhrsGetQuaternion(&AhrsDriver);
							data.w = Model::GetIntegerAngle(quat.element.w);
							data.x = Model::GetIntegerAngle(quat.element.x);
							data.y = Model::GetIntegerAngle(quat.element.y);
							data.z = Model::GetIntegerAngle(quat.element.z);
							data.timestamp = LastUpdateTimestamp;

							return true;
						}

						return false;
					}

					bool GetData(Inertia::Model::timestamped_euler_angle_t& data) final
					{
						if (AhrsDataAvailable)
						{
							const auto euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&AhrsDriver));
							data.yaw = Model::GetIntegerAngle(euler.angle.yaw);
							data.pitch = Model::GetIntegerAngle(euler.angle.pitch);
							data.roll = Model::GetIntegerAngle(euler.angle.roll);
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

						const auto linearAcceleration = FusionAhrsGetLinearAcceleration(&AhrsDriver);

						GravityRemovedAccelerationData.x = ScaleAccelerationInverse(linearAcceleration.axis.x);
						GravityRemovedAccelerationData.y = ScaleAccelerationInverse(linearAcceleration.axis.y);
						GravityRemovedAccelerationData.z = ScaleAccelerationInverse(linearAcceleration.axis.z);
						GravityRemovedAccelerationData.timestamp = LastUpdateTimestamp;
						GravityRemovedAccelerationAvailable = true;
					}

					void UpdateAhrsAngularVelocity(const FusionVector& gyroscope)
					{
						if (!HasAngularVelocitySource())
						{
							AhrsAngularVelocityAvailable = false;
							return;
						}

						const auto euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&AhrsDriver));
						const float rollRadians = FusionDegreesToRadians(euler.angle.roll);
						const float pitchRadians = FusionDegreesToRadians(euler.angle.pitch);

						const float sinPhi = sinf(rollRadians);
						const float cosPhi = cosf(rollRadians);
						const float cosTheta = cosf(pitchRadians);
						const float tanTheta = tanf(pitchRadians);

						const float eulerRollRate = gyroscope.axis.x + (sinPhi * tanTheta * gyroscope.axis.y) + (cosPhi * tanTheta * gyroscope.axis.z);
						const float eulerPitchRate = (cosPhi * gyroscope.axis.y) - (sinPhi * gyroscope.axis.z);
						float eulerYawRate = 0.0f;

						if (fabsf(cosTheta) > 1e-6f)
						{
							eulerYawRate = ((sinPhi * gyroscope.axis.y) / cosTheta) + ((cosPhi * gyroscope.axis.z) / cosTheta);
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
						return (sensorMode == SensorModeEnum::AccelerometerOnly)
							|| (sensorMode == SensorModeEnum::AccelerometerAndGyroscope)
							|| (sensorMode == SensorModeEnum::AccelerometerGyroscopeAndMagnetometer);
					}

					constexpr bool HasAccelerationSource() const
					{
						return HasAccelerationSource(SensorMode);
					}

					static constexpr bool HasAngularVelocitySource(const SensorModeEnum sensorMode)
					{
						return (sensorMode == SensorModeEnum::GyroscopeOnly)
							|| (sensorMode == SensorModeEnum::AccelerometerAndGyroscope)
							|| (sensorMode == SensorModeEnum::AccelerometerGyroscopeAndMagnetometer);
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