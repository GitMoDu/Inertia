#ifndef _XGV2_HEAD_UNIT_ALTITUDE_ESTIMATOR_TASK_h
#define _XGV2_HEAD_UNIT_ALTITUDE_ESTIMATOR_TASK_h

#include <InertiaModel.h>
#include <IntegerTrigonometry16.h>

namespace Inertia
{
	namespace Components
	{
		namespace Kinematic
		{
			/// <summary>
			/// Estimates altitude by combining ground distance sensor readings with orientation data to compute angle-compensated altitude using pitch and roll corrections.
			/// </summary>
			/// <typeparam name="LowPassFilterFactor">The low-pass filter factor applied to altitude readings for smoothing. Higher values provide more filtering. Default is 3.</typeparam>
			template<uint8_t LowPassFilterFactor = 3>
			class PeriodicAngleAwareAltitudeEstimator
				: public Inertia::Components::Lifecycle::IPeriodicDriver
				, public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_range16_t
				>
			{
			private:
				static constexpr uint32_t DISTANCE_SENSOR_TIMEOUT = 500000;

				IntegerSignal::Filters::LowPassU16<LowPassFilterFactor> AltitudeFilter{};

			private:
				// Raw ground distance sensor reading.
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_range16_t>* GroundDistanceSource;

				// AHRS orientation data, in Euler angles.
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_euler_angle_t>* OrientationSource;

			public:
				Inertia::Model::ILogListener* LogListener = nullptr;
				uint8_t InstanceId = 0;

			private:
				Inertia::Model::timestamped_quality_range16_t LastAltitude{};
				Inertia::Model::timestamped_quality_range16_t FilteredAltitude{};
				uint16_t BaseDistance = 0;
				uint16_t MaxDistance = 0;
				bool HasAltitude = false;

			public:
				PeriodicAngleAwareAltitudeEstimator(
					Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_range16_t>* ground_distance_source,
					Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_euler_angle_t>* orientation_source)
					: Inertia::Components::Lifecycle::IPeriodicDriver()
					, GroundDistanceSource(ground_distance_source)
					, OrientationSource(orientation_source)
				{}

				void SetDistanceLimits(const uint16_t baseDistance, const uint16_t maxDistance)
				{
					BaseDistance = baseDistance;
					MaxDistance = maxDistance;
				}

				bool Start() override
				{
					HasAltitude = false;

					return true;
				}

				void Stop() override
				{
					HasAltitude = false;
				}

				bool GetData(Inertia::Model::timestamped_quality_range16_t& out_data) override
				{
					//out_data = LastAltitude;
					out_data = FilteredAltitude;
					return HasAltitude;
				}

				bool GetRawData(Inertia::Model::timestamped_quality_range16_t& out_data)
				{
					out_data = LastAltitude;
					return HasAltitude;
				}

			public:
				void Step() override
				{
					Inertia::Model::timestamped_quality_range16_t range{};
					Inertia::Model::timestamped_euler_angle_t orientation{};

					if (GroundDistanceSource != nullptr
						&& GroundDistanceSource->GetData(range))
					{
						const uint32_t sampleAge = micros() - range.timestamp;
						if (sampleAge > (DISTANCE_SENSOR_TIMEOUT))
						{
							// Data is too old, skip processing and mark altitude as unavailable.
							HasAltitude = false;
						}
						else
						{
							// Check if the pitch and roll angles indicate that the sensor is generally facing towards the ground. 
							// If either angle indicates that the sensor is facing away from the ground (e.g., pitch near 180 degrees),
							// then the altitude reading would be unreliable, and we can mark it as such by setting quality to 0.
							if (OrientationSource != nullptr
								&& OrientationSource->GetData(orientation)
								&& (orientation.pitch < IntegerSignal::Trigonometry::ANGLE_90 || orientation.pitch > IntegerSignal::Trigonometry::ANGLE_270)
								&& (orientation.roll < IntegerSignal::Trigonometry::ANGLE_90 || orientation.roll > IntegerSignal::Trigonometry::ANGLE_270))
							{
								// Calculate the angle-aware altitude using the raw ground distance and the AHRS orientation data.
								const auto pitchScale = IntegerSignal::Trigonometry::Cosine16(orientation.pitch);
								const auto rollScale = IntegerSignal::Trigonometry::Cosine16(orientation.roll);

								const auto angleScale = IntegerSignal::FixedPoint::ScalarFraction::Fraction(pitchScale, rollScale);

								LastAltitude.distance =
									IntegerSignal::LimitValue<uint16_t>(
										IntegerSignal::FixedPoint::ScalarFraction::Fraction(angleScale, range.distance),
										BaseDistance, MaxDistance
									) - BaseDistance;
								LastAltitude.timestamp = range.timestamp;

								// Propagate quality from the raw range reading for now. Could be improved by factoring in the orientation data quality when available.
								LastAltitude.quality = range.quality;
								HasAltitude = true;

								AltitudeFilter.Set(LastAltitude.distance);
							}
							else
							{
								AltitudeFilter.Set(0);
								HasAltitude = false;
							}

							AltitudeFilter.Step();

							FilteredAltitude.distance = AltitudeFilter.Get();
						}
					}
					else
					{
						HasAltitude = false;
					}
				}
			};
		}
	}
}

#endif