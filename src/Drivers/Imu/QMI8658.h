#ifndef _INERTIA_DRIVERS_QMI8658_WRAPPER_h
#define _INERTIA_DRIVERS_QMI8658_WRAPPER_h

#if !defined(ARDUINO_ARCH_AVR)
#include <QMI8658.h> // https://github.com/lahavg/QMI8658-Arduino-Library


#include "../../Components/Core/Primitives.h"
#include "../../Components/Core/DataSource/Model.h"
#include "../../Components/Core/Lifecycle/Model.h"


namespace Inertia
{
	namespace Drivers
	{
		namespace QMI8658Wrapper
		{
			using namespace IntegerSignal;
			using namespace IntegerSignal::FixedPoint::FactorScale;

			enum class AccelerometerRangeEnum : uint8_t
			{
				Range2g = QMI8658_ACCEL_RANGE_2G,
				Range4g = QMI8658_ACCEL_RANGE_4G,
				Range8g = QMI8658_ACCEL_RANGE_8G,
				Range16g = QMI8658_ACCEL_RANGE_16G
			};

			enum class AccelerometerSampleRateEnum : uint16_t
			{
				SampleRate62_5Hz = QMI8658_ACCEL_ODR_62_5HZ,
				SampleRate125Hz = QMI8658_ACCEL_ODR_125HZ,
				SampleRate250Hz = QMI8658_ACCEL_ODR_250HZ,
				SampleRate500Hz = QMI8658_ACCEL_ODR_500HZ,
				SampleRate1000Hz = QMI8658_ACCEL_ODR_1000HZ,
				SampleRate2000Hz = QMI8658_ACCEL_ODR_2000HZ,
				SampleRate4000Hz = QMI8658_ACCEL_ODR_4000HZ,
				SampleRate8000Hz = QMI8658_ACCEL_ODR_8000HZ
			};

			enum class GyroscopeRangeEnum : uint16_t
			{
				Range32dps = QMI8658_GYRO_RANGE_32DPS,
				Range64dps = QMI8658_GYRO_RANGE_64DPS,
				Range128dps = QMI8658_GYRO_RANGE_128DPS,
				Range256dps = QMI8658_GYRO_RANGE_256DPS,
				Range512dps = QMI8658_GYRO_RANGE_512DPS,
				Range1024dps = QMI8658_GYRO_RANGE_1024DPS,
				Range2048dps = QMI8658_GYRO_RANGE_2048DPS
			};

			enum class GyroscopeSampleRateEnum : uint16_t
			{
				SampleRate62_5Hz = QMI8658_GYRO_ODR_62_5HZ,
				SampleRate125Hz = QMI8658_GYRO_ODR_125HZ,
				SampleRate250Hz = QMI8658_GYRO_ODR_250HZ,
				SampleRate500Hz = QMI8658_GYRO_ODR_500HZ,
				SampleRate1000Hz = QMI8658_GYRO_ODR_1000HZ,
				SampleRate2000Hz = QMI8658_GYRO_ODR_2000HZ,
				SampleRate4000Hz = QMI8658_GYRO_ODR_4000HZ,
				SampleRate8000Hz = QMI8658_GYRO_ODR_8000HZ
			};

			static constexpr uint8_t DEVICE_ADDRESS = 0x6A;

			static constexpr uint32_t CLOCK_SPEED_I2C = 800000;

			template<uint32_t ClockSpeedI2C = CLOCK_SPEED_I2C,
				AccelerometerRangeEnum accelerometerRange = AccelerometerRangeEnum::Range16g,
				AccelerometerSampleRateEnum accelerometerSampleRate = AccelerometerSampleRateEnum::SampleRate500Hz,
				GyroscopeRangeEnum gyroscopeRange = GyroscopeRangeEnum::Range2048dps,
				GyroscopeSampleRateEnum gyroscopeSampleRate = GyroscopeSampleRateEnum::SampleRate500Hz>
			class TemplateDriver : public Inertia::Components::Lifecycle::IPeriodicDriver,
				public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>,
				public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>,
				public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_temperature_t>
			{
			public:
				using DataTypes = Inertia::Components::DataSource::Variadic::VariadicDataTypeList<
					Inertia::Model::timestamped_acceleration_t,
					Inertia::Model::timestamped_angular_velocity_t,
					Inertia::Model::timestamped_temperature_t>;

			private:
				Model::timestamped_acceleration_t AccelerationData{};
				Model::timestamped_angular_velocity_t AngularVelocityData{};
				Model::timestamped_temperature_t TemperatureData{};
				bool ImuDataAvailable = false;
				QMI8658 Sensor{};

				TwoWire& WireInstance;

				// Precompute gyro scale: input in milli-dps -> angle units.
				// angle = mdps * ANGLE_RANGE / 360000
				static constexpr scale16_t GyroScaleFactor = Scale16::GetFactor<int32_t>(ANGLE_RANGE / 2, 360000);

			public:
				TemplateDriver(TwoWire& wire = Wire)
					: Inertia::Components::Lifecycle::IPeriodicDriver()
					, Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>()
					, Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>()
					, Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_temperature_t>()
					, WireInstance(wire) {}

				bool GetData(Model::timestamped_acceleration_t& data) final
				{
					if (!ImuDataAvailable)
					{
						return false;
					}
					memcpy(&data, &AccelerationData, sizeof(Model::timestamped_acceleration_t));
					return true;
				}

				bool GetData(Model::timestamped_temperature_t& data) final
				{
					if (!ImuDataAvailable)
					{
						return false;
					}
					memcpy(&data, &TemperatureData, sizeof(Model::timestamped_temperature_t));
					return true;
				}

				bool GetData(Model::timestamped_angular_velocity_t& data) final
				{
					if (!ImuDataAvailable)
					{
						return false;
					}
					memcpy(&data, &AngularVelocityData, sizeof(Model::timestamped_angular_velocity_t));
					return true;
				}

				bool Start() final
				{
					ImuDataAvailable = false;

					WireInstance.setClock(ClockSpeedI2C);

					if (!Sensor.begin(WireInstance, DEVICE_ADDRESS))
					{
						return false;
					}

					const bool accelConfigured = Sensor.setAccelRange(static_cast<QMI8658_AccelRange>(accelerometerRange))
						&& Sensor.setAccelODR(static_cast<QMI8658_AccelODR>(accelerometerSampleRate));

					const bool gyroConfigured = Sensor.setGyroRange(static_cast<QMI8658_GyroRange>(gyroscopeRange))
						&& Sensor.setGyroODR(static_cast<QMI8658_GyroODR>(gyroscopeSampleRate));

					if (!(accelConfigured && gyroConfigured))
					{
						return false;
					}

					Sensor.setAccelUnit_mg(true);
					Sensor.setGyroUnit_dps(true);
					Sensor.setDisplayPrecision(QMI8658_Precision::QMI8658_PRECISION_6);

					const bool enabled = Sensor.enableSensors(QMI8658_ENABLE_ACCEL | QMI8658_ENABLE_GYRO);

					if (enabled)
					{
						return true;
					}
					else
					{
						return false;
					}
				}

				void Stop() final
				{
					ImuDataAvailable = false;
				}

				void Step() final
				{
					const uint32_t timestamp = micros();
					WireInstance.setClock(ClockSpeedI2C);

					QMI8658_Data sensorData{};
					if (!Sensor.readSensorData(sensorData))
					{
						return;
					}

					// Acceleration: float -> mg -> acceleration_t (1/1000 g).
					AccelerationData.x = static_cast<int16_t>(sensorData.accelX);
					AccelerationData.y = static_cast<int16_t>(sensorData.accelY);
					AccelerationData.z = static_cast<int16_t>(sensorData.accelZ);
					AccelerationData.timestamp = timestamp;

					// Gyro in dps -> angle units using precomputed scale.
					const int32_t gxMdps = static_cast<int32_t>(sensorData.gyroX * 1000.0f + (sensorData.gyroX >= 0.0f ? 0.5f : -0.5f));
					const int32_t gyMdps = static_cast<int32_t>(sensorData.gyroY * 1000.0f + (sensorData.gyroY >= 0.0f ? 0.5f : -0.5f));
					const int32_t gzMdps = static_cast<int32_t>(sensorData.gyroZ * 1000.0f + (sensorData.gyroZ >= 0.0f ? 0.5f : -0.5f));

					AngularVelocityData.x = Scale(GyroScaleFactor, gxMdps);
					AngularVelocityData.y = Scale(GyroScaleFactor, gyMdps);
					AngularVelocityData.z = Scale(GyroScaleFactor, gzMdps);
					AngularVelocityData.timestamp = timestamp;

					const float tempC = sensorData.temperature;
					const int32_t tempCentiKelvin = static_cast<int32_t>((tempC + 273.15f) * 100.0f + 0.5f);
					TemperatureData.temperature = static_cast<Model::temperature_t>(
						LimitValue<int32_t, 0, UINT16_MAX>(tempCentiKelvin));
					TemperatureData.timestamp = timestamp;

					ImuDataAvailable = true;
				}
			};
		}
	}
}
#endif
#endif