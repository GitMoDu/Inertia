#ifndef _INERTIA_CALIBRATED_RANGE_TESTS_h
#define _INERTIA_CALIBRATED_RANGE_TESTS_h

#include <InertiaModel.h>

namespace CalibratedRangeTests
{
	using namespace Inertia::Components::Distance;
	using Sample = Inertia::Model::timestamped_quality_range16_t;

	struct Source : Inertia::Components::DataSource::IDataSource<timestamped_signal_range16_t>
	{
		timestamped_signal_range16_t Value{};
		bool Available = true;
		bool GetData(timestamped_signal_range16_t& output) override
		{
			if (!Available) return false;
			output = Value;
			return true;
		}
	};

	static void Run(void (*check)(const char*, bool))
	{
		Source source;
		// Test fixture only, not a sensor or vehicle default.
		return_rate_calibration_t calibration{ 14000, 1100, 12 };
		PeriodicCalibratedRange<> corrected(source, calibration, 150000);
		calibration.MaxCorrectionMillimetres = 0;
		Sample output{};
		output.distance = 4321;
		check("Unstarted compensation preserves caller output", !corrected.GetData(output) && output.distance == 4321);
		check("Copied software calibration starts", corrected.Start());
		struct Case { uint16_t Distance; uint16_t Rate; uint16_t Expected; };
		const Case cases[] = {
			{15, 12379, 15}, {1, 27578, 13}, {3, 28635, 15}, {8, 17605, 11}, {11, 16759, 14},
			{10, 0, 10}, {10, 14000, 10}, {10, 14001, 10}, {10, 14549, 10}, {10, 14550, 11},
			{0, 26649, 11}, {0, 26650, 12}, {0, 27750, 12}, {255, 65535, 267},
			{UINT16_MAX, UINT16_MAX, UINT16_MAX}, {65530, UINT16_MAX, UINT16_MAX}
		};
		for (const auto& test : cases)
		{
			source.Value.distance = test.Distance;
			source.Value.ReturnRate = test.Rate;
			source.Value.quality = 173;
			++source.Value.timestamp;
			corrected.Step();
			check("Calibration boundary vector, metadata and uint16 saturation", corrected.GetData(output)
				&& output.distance == test.Expected && output.timestamp == source.Value.timestamp && output.quality == 173);
		}
		source.Available = false;
		corrected.Step();
		check("Unavailable source withdraws correction", !corrected.GetData(output));
		source.Available = true;
		++source.Value.timestamp;
		check("New source sample is not exposed before processing", !corrected.GetData(output));
		corrected.Step();
		check("Source recovery resumes correction", corrected.GetData(output) && output.distance == UINT16_MAX);

		PeriodicCalibratedRange<> invalid(source, { 0, 0, 10 }, 150000);
		check("Zero calibration divisor is explicitly rejected", !invalid.Start());
		invalid.Step();
		check("Failed start cannot expose output", !invalid.GetData(output));
		PeriodicCalibratedRange<> invalidGap(source, { 0, 1, 10 }, 0);
		check("Zero sample gap is explicitly rejected", !invalidGap.Start());
		PeriodicCalibratedRange<> bypass(source, { 0, 1, 0 }, 150000);
		bypass.Start(); bypass.Step();
		check("Zero correction limit bypasses compensation", bypass.GetData(output) && output.distance == source.Value.distance);

		PeriodicCalibratedRange<1> filtered(source, { 0, 1, 0 }, 150000);
		filtered.Start();
		source.Value.timestamp = UINT32_MAX - 10000;
		source.Value.distance = 100;
		filtered.Step();
		source.Value.timestamp = 9999;
		source.Value.distance = 200;
		filtered.Step();
		check("Sample gap handles timestamp wrap", filtered.GetData(output) && output.distance == 150);
		source.Value.timestamp += 150000;
		source.Value.distance = 300;
		filtered.Step();
		check("Exact sample gap boundary reseeds filter", filtered.GetData(output) && output.distance == 300);
	}
}

#endif
