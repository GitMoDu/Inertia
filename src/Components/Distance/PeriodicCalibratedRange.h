#ifndef _INERTIA_COMPONENTS_DISTANCE_PERIODIC_CALIBRATED_RANGE_h
#define _INERTIA_COMPONENTS_DISTANCE_PERIODIC_CALIBRATED_RANGE_h

#include "Model.h"
#include "../Core/DataSource/Model.h"
#include "../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Distance
		{
			template<uint8_t FilterFactor = 0>
			class PeriodicCalibratedRange : public Lifecycle::IPeriodicDriver,
				public DataSource::IDataSource<Inertia::Model::timestamped_quality_range16_t>
			{
				static_assert(FilterFactor <= 16, "Range filter factor must be between 0 and 16.");

			private:
				DataSource::IDataSource<timestamped_signal_range16_t>& Source;
				const return_rate_calibration_t Calibration;
				const uint32_t MaxSampleGapMicros;
				IntegerSignal::Filters::LowPassU16<FilterFactor> Filter{};
				Inertia::Model::timestamped_quality_range16_t Sample{};
				bool Running = false;
				bool Available = false;

			public:
				PeriodicCalibratedRange(DataSource::IDataSource<timestamped_signal_range16_t>& source,
					const return_rate_calibration_t calibration, const uint32_t maxSampleGapMicros)
					: Source(source), Calibration(calibration), MaxSampleGapMicros(maxSampleGapMicros)
				{}

				bool Start() override
				{
					Available = false;
					Running = Calibration.RatePerMillimetre != 0 && MaxSampleGapMicros != 0
						&& MaxSampleGapMicros <= INT32_MAX;
					return Running;
				}

				void Stop() override
				{
					Running = Available = false;
				}

				void Step() override
				{
					if (!Running) return;
					timestamped_signal_range16_t input{};
					if (!Source.GetData(input))
					{
						Available = false;
						return;
					}
					if (Available && input.timestamp == Sample.timestamp) return;

					uint32_t correction = 0;
					if (input.ReturnRate > Calibration.Threshold)
					{
						correction = (uint32_t(input.ReturnRate) - Calibration.Threshold
							+ Calibration.RatePerMillimetre / 2) / Calibration.RatePerMillimetre;
						correction = IntegerSignal::MinValue<uint32_t>(correction, Calibration.MaxCorrectionMillimetres);
					}
					const uint16_t corrected = static_cast<uint16_t>(IntegerSignal::MinValue<uint32_t>(
						uint32_t(input.distance) + correction, UINT16_MAX));
					if (!Available || uint32_t(input.timestamp - Sample.timestamp) >= MaxSampleGapMicros)
					{
						Filter.Clear(corrected);
					}
					else
					{
						Filter.Set(corrected);
						Filter.Step();
					}
					Sample = input;
					Sample.distance = Filter.Get();
					Available = true;
				}

				bool GetData(Inertia::Model::timestamped_quality_range16_t& output) override
				{
					timestamped_signal_range16_t current{};
					if (!Running || !Available || !Source.GetData(current) || current.timestamp != Sample.timestamp)
						return false;
					output = Sample;
					return true;
				}
			};
		}
	}
}

#endif
