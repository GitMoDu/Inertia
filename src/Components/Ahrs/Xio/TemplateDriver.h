#ifndef _INERTIA_COMPONENTS_AHRS_XIO_TEMPLATE_DRIVER_h
#define _INERTIA_COMPONENTS_AHRS_XIO_TEMPLATE_DRIVER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>
#include <math.h>

#include "../../../Components/Core/Primitives.h"
#include "../../../Components/Core/Lifecycle/Model.h"
#include "../../../Components/Core/DataSource/Model.h"
#include "Model.h"

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
				class TemplateDriver : public Inertia::Components::Lifecycle::IPeriodicDriver
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_euler_angle_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_quaternion_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>
					, public Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>
				{
				public:
					using DataTypes = Inertia::Components::DataSource::Variadic::VariadicDataTypeList<
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
					static constexpr float GyroscopeOverrangeThresholdRatio = 0.98f;
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
					uint32_t SamplePeriodMicros = 0;
					bool Running = false;
					bool HasPreviousTimestamp = false;
					bool AhrsDataAvailable = false;
					bool GravityRemovedAccelerationAvailable = false;
					bool AhrsAngularVelocityAvailable = false;

					const SensorModeEnum SensorMode;
					const FusionAhrsSettings Settings;
					const FusionBiasSettings BiasSettings;

				public:
					TemplateDriver(Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* accelerationSource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* angularVelocitySource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_magnet_t>* magnetometerSource = nullptr,
						const float gyroscopeRangeDegreesPerSecond = DefaultGyroscopeRangeDegreesPerSecond,
						const float gain = fusionAhrsDefaultSettings.gain)
						: TemplateDriver(accelerationSource, angularVelocitySource, magnetometerSource,
							MakeSettings(gyroscopeRangeDegreesPerSecond, gain))
					{}

					TemplateDriver(Inertia::Model::IDataSource<Inertia::Model::timestamped_acceleration_t>* accelerationSource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* angularVelocitySource,
						Inertia::Model::IDataSource<Inertia::Model::timestamped_magnet_t>* magnetometerSource,
						const FusionAhrsSettings settings,
						const FusionBiasSettings biasSettings = fusionBiasDefaultSettings)
						: Inertia::Components::Lifecycle::IPeriodicDriver()
						, AccelerationSource(accelerationSource)
						, AngularVelocitySource(angularVelocitySource)
						, MagnetometerSource(magnetometerSource)
						, SensorMode(GetSensorMode(AccelerationSource != nullptr, AngularVelocitySource != nullptr, MagnetometerSource != nullptr))
						, Settings(settings)
						, BiasSettings(biasSettings)
					{}

					bool Start() override
					{
						Stop();

						if ((SensorMode == SensorModeEnum::Invalid)
							|| (SensorMode == SensorModeEnum::AccelerometerOnly)
							|| !ValidSettings())
						{
							return false;
						}

						FusionAhrsInitialise(&AhrsDriver);
						FusionBiasInitialise(&BiasDriver);

						FusionAhrsSettings settings = Settings;
						settings.gain = HasAccelerationSource() ? Settings.gain : 0.0f;
						FusionAhrsSetSettings(&AhrsDriver, &settings);
						Running = true;

						return true;
					}

					void Stop() override
					{
						Running = false;
						HasPreviousTimestamp = false;
						SamplePeriodMicros = 0;
						AhrsDataAvailable = false;
						GravityRemovedAccelerationAvailable = false;
						AhrsAngularVelocityAvailable = false;
						LastUpdateTimestamp = 0;
						PreviousUpdateTimestamp = 0;
					}

					void Step() final
					{
						if (!Running) return;

						Inertia::Model::timestamped_acceleration_t accelerationData{};
						Inertia::Model::timestamped_angular_velocity_t angularVelocityData{};
						Inertia::Model::timestamped_magnet_t magnetData{};
						bool dataReady = true;

						FusionVector gyroscope{};
						FusionVector accelerometer{};
						FusionVector magnetometer{};

						switch (SensorMode)
						{
						case SensorModeEnum::AccelerometerOnly:
						case SensorModeEnum::AccelerometerAndGyroscope:
						case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
							if ((AccelerationSource != nullptr)
								&& AccelerationSource->GetData(accelerationData))
							{
								accelerometer.axis.x = ScaleAcceleration(accelerationData.x);
								accelerometer.axis.y = ScaleAcceleration(accelerationData.y);
								accelerometer.axis.z = ScaleAcceleration(accelerationData.z);
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
								&& AngularVelocitySource->GetData(angularVelocityData))
							{
								gyroscope.axis.x = ScaleAngularVelocity(angularVelocityData.x);
								gyroscope.axis.y = ScaleAngularVelocity(angularVelocityData.y);
								gyroscope.axis.z = ScaleAngularVelocity(angularVelocityData.z);
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
								&& MagnetometerSource->GetData(magnetData))
							{
								magnetometer.axis.x = static_cast<float>(magnetData.x);
								magnetometer.axis.y = static_cast<float>(magnetData.y);
								magnetometer.axis.z = static_cast<float>(magnetData.z);
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

						const uint32_t currentTimestamp = angularVelocityData.timestamp;

						if (!HasPreviousTimestamp)
						{
							PreviousUpdateTimestamp = currentTimestamp;
							HasPreviousTimestamp = true;
							return;
						}

						const uint32_t deltaTimeMicros = currentTimestamp - PreviousUpdateTimestamp;
						PreviousUpdateTimestamp = currentTimestamp;

						if (deltaTimeMicros == 0)
						{
							return;
						}

						const float deltaTimeSeconds = static_cast<float>(deltaTimeMicros) * MicrosecondsToSeconds;
						FusionBiasSettings biasSettings = BiasSettings;
						const float biasSampleRate = deltaTimeSeconds > 0.0f ? (1.0f / deltaTimeSeconds) : MinimumBiasSampleRate;
						biasSettings.sampleRate = biasSampleRate > MinimumBiasSampleRate ? biasSampleRate : MinimumBiasSampleRate;
						FusionBiasSetSettings(&BiasDriver, &biasSettings);
						gyroscope = FusionBiasUpdate(&BiasDriver, gyroscope);
						const bool gyroscopeOverrange = IsGyroscopeOverrange(gyroscope);
						const float ahrsSamplePeriod = gyroscopeOverrange ? 0.0f : deltaTimeSeconds;

						if (SensorMode == SensorModeEnum::AccelerometerGyroscopeAndMagnetometer)
						{
							FusionAhrsSetSamplePeriod(&AhrsDriver, ahrsSamplePeriod);
							FusionAhrsUpdate(&AhrsDriver, gyroscope, accelerometer, magnetometer);
						}
						else
						{
							FusionAhrsSetSamplePeriod(&AhrsDriver, ahrsSamplePeriod);
							FusionAhrsUpdateNoMagnetometer(&AhrsDriver, gyroscope, accelerometer);
						}
						FusionAhrsSetSamplePeriod(&AhrsDriver, deltaTimeSeconds);

						LastUpdateTimestamp = currentTimestamp;
						AccelerationData = accelerationData;
						AngularVelocityData = angularVelocityData;
						MagnetData = magnetData;
						SamplePeriodMicros = deltaTimeMicros;
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

					bool GetDiagnostics(diagnostics_t& output) const
					{
						if (!AhrsDataAvailable) return false;
						output.Acceleration = AccelerationData;
						output.AngularVelocity = AngularVelocityData;
						output.Magnetometer = MagnetData;
						output.SamplePeriodMicros = SamplePeriodMicros;
						output.Orientation = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&AhrsDriver));
						output.GyroscopeBiasDegreesPerSecond = FusionBiasGetOffset(&BiasDriver);
						output.States = FusionAhrsGetInternalStates(&AhrsDriver);
						output.Flags = FusionAhrsGetFlags(&AhrsDriver);
						return true;
					}

				private:
					static FusionAhrsSettings MakeSettings(const float range, const float gain)
					{
						FusionAhrsSettings settings = fusionAhrsDefaultSettings;
						settings.gyroscopeRange = range;
						settings.gain = gain;
						return settings;
					}

					bool ValidSettings() const
					{
						return Settings.convention == FusionConventionNwu
							&& isfinite(Settings.sampleRate) && Settings.sampleRate > 0.0f
							&& isfinite(Settings.gain) && Settings.gain >= 0.0f
							&& isfinite(Settings.gyroscopeRange) && Settings.gyroscopeRange >= 0.0f
							&& isfinite(Settings.accelerationRejection) && Settings.accelerationRejection >= 0.0f && Settings.accelerationRejection <= 90.0f
							&& isfinite(Settings.magneticRejection) && Settings.magneticRejection >= 0.0f && Settings.magneticRejection <= 90.0f
							&& isfinite(Settings.rejectionTimeout) && Settings.rejectionTimeout >= 0.0f
							&& Settings.sampleRate * Settings.rejectionTimeout < 2147483648.0f
							&& isfinite(BiasSettings.stationaryThreshold) && BiasSettings.stationaryThreshold >= 0.0f
							&& isfinite(BiasSettings.stationaryPeriod) && BiasSettings.stationaryPeriod >= 0.0f
							&& BiasSettings.stationaryPeriod * 1000000.0f < 4294967296.0f;
					}

					bool IsGyroscopeOverrange(const FusionVector& gyroscope) const
					{
						if (Settings.gyroscopeRange <= 0.0f)
						{
							return false;
						}

						const float overrangeThreshold = GyroscopeOverrangeThresholdRatio * Settings.gyroscopeRange;
						return (fabsf(gyroscope.axis.x) > overrangeThreshold)
							|| (fabsf(gyroscope.axis.y) > overrangeThreshold)
							|| (fabsf(gyroscope.axis.z) > overrangeThreshold);
					}

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

					static int16_t ScaleAccelerationInverse(const float acceleration)
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

					static int32_t ScaleAngularVelocityInverse(const float angularVelocity)
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