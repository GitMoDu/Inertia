#ifndef _INERTIA_AHRS_REEFWING_h
#define _INERTIA_AHRS_REEFWING_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "../Framework/Model.h"

#include <ReefwingAHRS.h> // https://github.com/Reefwing-Software/Reefwing-AHRS

namespace Inertia
{
	namespace Ahrs
	{
		namespace Reefwing
		{
			/// <summary>
			/// Reefwing AHRS orientation driver template.
			/// Provides orientation data (Euler angles and quaternion)
			/// by aggregating sensor data from acceleration, angular velocity, and magnetometer sources.
			/// </summary>
			/// <typeparam name="ahrsMode">Model::AhrsModeEnum value that selects the AHRS fusion mode and algorithm configuration (default: Model::AhrsModeEnum::AbsoluteOrientation). Affects which fusion algorithm and gains are used.</typeparam>
			class Driver : public Model::IPeriodicDriver
				, public Model::IDataSource<Model::timestamped_euler_angle_t>
				, public Model::IDataSource<Model::timestamped_quaternion_t>
			{
			public:
				using DataTypes = Inertia::Drivers::Variadic::VariadicDataTypeList<
					Inertia::Model::timestamped_euler_angle_t,
					Inertia::Model::timestamped_quaternion_t>;

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
				ReefwingAHRS AhrsDriver{};

			private:
				// Sensor data source.
				Model::IDataSource<Model::timestamped_acceleration_t>* AccelerationSource;
				Model::IDataSource<Model::timestamped_angular_velocity_t>* AngularVelocitySource;
				Model::IDataSource<Model::timestamped_vector16_t>* MagnetometerSource;

				// Model native sensor data format.
				Model::timestamped_acceleration_t AccelerationData{};
				Model::timestamped_angular_velocity_t AngularVelocityData{};
				Model::timestamped_magnet_t MagnetData{};

				// Reefwing AHRS sensor data format.
				SensorData AhrsSensorData{};

				// AHRS tracking state.
				uint32_t LastUpdateTimestamp = 0;
				bool AhrsDataAvailable = false;

				const SensorModeEnum SensorMode;

			public:
				Driver(TS::Scheduler& scheduler,
					Model::IDataSource<Model::timestamped_acceleration_t>* accelerationSource = nullptr,
					Model::IDataSource<Model::timestamped_angular_velocity_t>* angularVelocitySource = nullptr,
					Model::IDataSource<Model::timestamped_magnet_t>* magnetometerSource = nullptr)
					: Model::IPeriodicDriver()
					, AhrsDriver()
					, AccelerationSource(accelerationSource)
					, AngularVelocitySource(angularVelocitySource)
					, MagnetometerSource(magnetometerSource)
					, SensorMode(GetSensorMode(AccelerationSource != nullptr, AngularVelocitySource != nullptr, MagnetometerSource != nullptr))
				{
				}

				bool Start() final
				{
					AhrsDataAvailable = false;

					if (SensorMode == SensorModeEnum::Invalid)
					{
						// No valid sensor combination provided.
						return false;
					}

					AhrsDriver.begin();

					switch (SensorMode)
					{
					case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
						if (AngularVelocitySource == nullptr
							|| AccelerationSource == nullptr
							|| MagnetometerSource == nullptr)
						{
							// Required sensors not provided.
							return false;
						}
						AhrsDriver.setDOF(DOF::DOF_9);
						break;
					case SensorModeEnum::AccelerometerOnly:
						if (AccelerationSource == nullptr)
						{
							// Required sensors not provided.
							return false;
						}
						AhrsDriver.setDOF(DOF::DOF_6);
						break;
					case SensorModeEnum::GyroscopeOnly:
						if (AngularVelocitySource == nullptr)
						{
							// Required sensors not provided.
							return false;
						}
						AhrsDriver.setDOF(DOF::DOF_6);
						break;
					case SensorModeEnum::AccelerometerAndGyroscope:
						if (AngularVelocitySource == nullptr
							&& AccelerationSource == nullptr)
						{
							// Required sensors not provided.
							return false;
						}
						AhrsDriver.setDOF(DOF::DOF_6);
						break;
					default:
						return false;
						break;
					}

					AhrsDriver.setImuType(ImuType::UNKNOWN);

					// Full 3D orientation. Yaw is relative to magnetic north, or initial heading if no magnetometer.
					AhrsDriver.setFusionAlgorithm(SensorFusion::MAHONY);
					AhrsDriver.setKp(1.0f); // Mahony filter proportional gain
					AhrsDriver.setKi(0.0f);  // Mahony filter integral gain

					return true;
				}

				void Stop() final
				{
					AhrsDriver.reset();
					AhrsDataAvailable = false;
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
						if (AccelerationSource != nullptr && AccelerationSource->GetData(AccelerationData))
						{
							AhrsSensorData.ax = ScaleAcceleration(AccelerationData.x);
							AhrsSensorData.ay = ScaleAcceleration(AccelerationData.y);
							AhrsSensorData.az = ScaleAcceleration(AccelerationData.z);
							AhrsSensorData.aTimeStamp = AccelerationData.timestamp;
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
					case SensorModeEnum::GyroscopeOnly:
					case SensorModeEnum::AccelerometerAndGyroscope:
					case SensorModeEnum::AccelerometerGyroscopeAndMagnetometer:
						if (AngularVelocitySource->GetData(AngularVelocityData))
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
						if (MagnetometerSource->GetData(MagnetData))
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

						if (!AhrsDataAvailable)
						{
							// AHRS data is now available.
							AhrsDataAvailable = true;
						}
					}
					else
					{
						// Not enough data to update AHRS.
						AhrsDataAvailable = false;
					}
				}

				bool GetData(Inertia::Model::timestamped_quaternion_t& data) final
				{
					if (AhrsDataAvailable)
					{
						Quaternion quat = AhrsDriver.getQuaternion();
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

			private:
				static constexpr float ScaleAcceleration(const int16_t acceleration)
				{
					return static_cast<float>(acceleration) * Model::G_PER_ACCELERATION_UNIT;
				}

				static constexpr float ScaleAngularVelocity(const int32_t angularVelocity)
				{
					return static_cast<float>(angularVelocity) * Model::DEG_PER_ANGLE_UNIT;
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

#endif