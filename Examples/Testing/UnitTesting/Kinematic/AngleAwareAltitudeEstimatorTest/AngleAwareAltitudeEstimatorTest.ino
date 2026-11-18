#include <InertiaModel.h>

using namespace Inertia::Model;
using namespace IntegerSignal::Trigonometry;
using Inertia::Components::Kinematic::PeriodicAngleAwareAltitudeEstimator;

template<typename T>
struct Source : Inertia::Components::DataSource::IDataSource<T>
{
	T Value{};
	bool Available = true;

	bool GetData(T& output) override
	{
		if (!Available) return false;
		output = Value;
		return true;
	}
};

static uint16_t Passed = 0;
static uint16_t Failed = 0;

void Check(const char* name, const bool passed)
{
	Serial.print(passed ? F("PASS: ") : F("FAIL: "));
	Serial.println(name);
	if (passed) ++Passed; else ++Failed;
}

void CheckClearance(const char* name, const uint16_t distance,
	const angle_t pitch, const angle_t roll, const angle_t yaw,
	const vector16_t offset, const uint16_t base, const uint16_t maximum,
	const uint16_t expected, const uint16_t tolerance = 0)
{
	Source<timestamped_quality_range16_t> range;
	Source<timestamped_euler_angle_t> orientation;
	range.Value.distance = distance;
	range.Value.timestamp = micros();
	range.Value.quality = 173;
	orientation.Value.pitch = pitch;
	orientation.Value.roll = roll;
	orientation.Value.yaw = yaw;
	orientation.Value.timestamp = range.Value.timestamp;
	PeriodicAngleAwareAltitudeEstimator<0> estimator(&range, &orientation);
	estimator.SetDistanceLimits(base, maximum);
	estimator.SetSensorOffset(offset);
	estimator.Start();
	estimator.Step();
	timestamped_quality_range16_t output{};
	const bool available = estimator.GetData(output);
	const int32_t error = int32_t(output.distance) - expected;
	Check(name, available && error >= -int32_t(tolerance) && error <= tolerance
		&& output.timestamp == range.Value.timestamp && output.quality == 173);
	timestamped_quality_range16_t raw{};
	Check("factor zero agrees with raw output", estimator.GetRawData(raw)
		&& raw.distance == output.distance && raw.timestamp == output.timestamp
		&& raw.quality == output.quality);
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	const angle_t tilt = ANGLE_90 / 2;
	const angle_t negativeTilt = static_cast<angle_t>(0 - tilt);
	CheckClearance("zero offset level", 1000, 0, 0, 0, {}, 0, 2000, 1000);
	CheckClearance("vertical mounting height", 1000, 0, 0, 0, { 0, 0, 100 }, 0, 2000, 900);
	CheckClearance("pitch projection", 1000, tilt, 0, 0, {}, 0, 2000, 707, 1);
	CheckClearance("roll projection", 1000, 0, tilt, 0, {}, 0, 2000, 707, 1);
	CheckClearance("combined tilt", 1000, tilt, tilt, 0, {}, 0, 2000, 500, 1);
	CheckClearance("forward origin positive pitch", 1000, tilt, 0, 0, { 100, 0, 0 }, 0, 2000, 777, 1);
	CheckClearance("forward origin negative pitch", 1000, negativeTilt, 0, 0, { 100, 0, 0 }, 0, 2000, 636, 1);
	CheckClearance("left origin positive roll", 1000, 0, tilt, 0, { 0, 100, 0 }, 0, 2000, 636, 1);
	CheckClearance("left origin negative roll", 1000, 0, negativeTilt, 0, { 0, 100, 0 }, 0, 2000, 777, 1);
	CheckClearance("combined offset", 1000, tilt, tilt, 0, { 100, 200, 100 }, 0, 2000, 420, 1);
	CheckClearance("yaw independence", 1000, tilt, tilt, ANGLE_180, { 100, 200, 100 }, 0, 2000, 420, 1);
	CheckClearance("negative offsets", 1000, 0, 0, 0, { -100, -200, -100 }, 0, 2000, 1100);
	CheckClearance("negative clearance clamps", 10, 0, 0, 0, { 0, 0, 100 }, 0, 2000, 0);
	CheckClearance("base after mounting compensation", 1000, 0, 0, 0, { 0, 0, 100 }, 100, 2000, 800);
	CheckClearance("lower limit", 150, 0, 0, 0, { 0, 0, 100 }, 100, 2000, 0);
	CheckClearance("upper limit", 1000, 0, 0, 0, { 0, 0, 100 }, 100, 800, 700);
	CheckClearance("wide signed intermediate", UINT16_MAX, 0, 0, 0, { 0, 0, INT16_MIN }, 0, UINT16_MAX, UINT16_MAX);
	CheckClearance("full range remains unsigned", UINT16_MAX, 0, 0, 0, {}, 0, UINT16_MAX, UINT16_MAX);
	CheckClearance("negative extreme clamps before conversion", 0, 0, 0, 0, { 0, 0, INT16_MAX }, 0, UINT16_MAX, 0);
	CheckClearance("full range tilted", UINT16_MAX, tilt, tilt, 0, {}, 0, UINT16_MAX, 32767, 4);

	Source<timestamped_quality_range16_t> range;
	Source<timestamped_euler_angle_t> orientation;
	range.Value.distance = 1000;
	range.Value.timestamp = micros();
	range.Value.quality = 201;
	PeriodicAngleAwareAltitudeEstimator<2> estimator(&range, &orientation);
	estimator.SetDistanceLimits(0, 2000);
	estimator.Start();
	estimator.Step();
	timestamped_quality_range16_t output{};
	Check("filtered metadata and smoothing", estimator.GetData(output)
		&& output.distance == 250 && output.timestamp == range.Value.timestamp && output.quality == 201);
	Check("unfiltered metadata and distance", estimator.GetRawData(output)
		&& output.distance == 1000 && output.timestamp == range.Value.timestamp && output.quality == 201);

	orientation.Value.pitch = ANGLE_90;
	estimator.Step();
	Check("horizontal beam rejected", !estimator.GetData(output));
	orientation.Value.pitch = ANGLE_180;
	estimator.Step();
	Check("upward beam rejected", !estimator.GetData(output));
	orientation.Value.pitch = 0;
	orientation.Value.roll = ANGLE_270;
	estimator.Step();
	Check("horizontal roll rejected", !estimator.GetData(output));
	orientation.Value.roll = 0;
	orientation.Available = false;
	estimator.Step();
	Check("missing orientation rejected", !estimator.GetData(output));
	orientation.Available = true;
	range.Available = false;
	estimator.Step();
	Check("missing range rejected", !estimator.GetData(output));
	range.Available = true;
	range.Value.timestamp = micros() - 500001UL;
	estimator.Step();
	Check("stale range rejected", !estimator.GetData(output));
	range.Value.timestamp = micros();
	estimator.Step();
	Check("valid sample recovers availability", estimator.GetData(output));
	estimator.Stop();
	Check("stop invalidates output", !estimator.GetData(output));
	estimator.Start();
	Check("start waits for sample", !estimator.GetData(output));

	Serial.print(F("Passed: ")); Serial.println(Passed);
	Serial.print(F("Failed: ")); Serial.println(Failed);
}

void loop() {}
