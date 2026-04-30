#ifndef _INERTIA_COMPONENTS_KINEMATIC_VELOCITY_ESTIMATOR_h
#define _INERTIA_COMPONENTS_KINEMATIC_VELOCITY_ESTIMATOR_h

#include <InertiaModel.h>
#include <IntegerSignal.h>

namespace Inertia
{
	namespace Components
	{
		namespace Kinematic
		{
			/// <summary>
			/// Estimates velocity from position data by calculating the rate of change over time, with optional low-pass filtering for noise reduction.
			/// </summary>
			/// <typeparam name="LowPassFilterFactor">The filtering strength factor for the low-pass filter applied to velocity estimates. Higher values provide more smoothing but increase latency. Defaults to 3.</typeparam>
			template<uint8_t LowPassFilterFactor = 3>
			class PeriodicVelocityEstimator
				: public Inertia::Components::Lifecycle::IPeriodicDriver
				, public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_velocity_t>
			{
			private:
				static constexpr uint32_t ONE_SECOND_MICROS = 1000000;

			private:
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_position_t>* TranslationSource;

			public:
				Inertia::Model::ILogListener* LogListener = nullptr;
				uint8_t InstanceId = 0;

			private:
				IntegerSignal::Filters::LowPassI32<LowPassFilterFactor> VelocityFilterX{};
				IntegerSignal::Filters::LowPassI32<LowPassFilterFactor> VelocityFilterY{};

				Inertia::Model::timestamped_velocity_t FilteredVelocity{};
				Inertia::Model::timestamped_velocity_t LastVelocity{};
				bool HasVelocity = false;
				Inertia::Model::timestamped_position_t LastTranslation{};


			public:
				PeriodicVelocityEstimator(
					Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_position_t>* translationSource)
					: Inertia::Components::Lifecycle::IPeriodicDriver()
					, Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_velocity_t>()
					, TranslationSource(translationSource)
				{}

				bool Start() override
				{
					LastVelocity = {};
					HasVelocity = false;
					LastTranslation = {};

					VelocityFilterX.Clear();
					VelocityFilterY.Clear();

					return true;
				}

				void Stop() override
				{
					LastVelocity = {};
					HasVelocity = false;
					LastTranslation = {};
				}

				bool GetData(Inertia::Model::timestamped_velocity_t& out_data) override
				{
					out_data = FilteredVelocity;
					return HasVelocity;
				}

				bool GetDataRaw(Inertia::Model::timestamped_velocity_t& out_data)
				{
					out_data = LastVelocity;
					return HasVelocity;
				}

			public:
				void Step() override
				{
					Inertia::Model::timestamped_position_t translation{};

					if (TranslationSource != nullptr
						&& TranslationSource->GetData(translation))
					{
						if (translation.timestamp != LastTranslation.timestamp)
						{
							const uint32_t timestamp = micros();

							const uint32_t deltaMicros = timestamp - LastTranslation.timestamp;
							const int32_t translatedX = translation.x - LastTranslation.x;
							const int32_t translatedY = translation.y - LastTranslation.y;

							LastVelocity.timestamp = timestamp;
							LastVelocity.x = (static_cast<int64_t>(ONE_SECOND_MICROS) * translatedX) / deltaMicros;
							LastVelocity.y = (static_cast<int64_t>(ONE_SECOND_MICROS) * translatedY) / deltaMicros;

							VelocityFilterX.Set(LastVelocity.x);
							VelocityFilterY.Set(LastVelocity.y);

							HasVelocity = true;
						}

						VelocityFilterX.Step();
						VelocityFilterY.Step();

						FilteredVelocity.x = VelocityFilterX.Get();
						FilteredVelocity.y = VelocityFilterY.Get();

						LastTranslation = translation;
					}
					else
					{
						HasVelocity = false;
					}
				}
			};
		}
	}
}

#endif