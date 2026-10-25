#include <Arduino.h>
#include <InertiaModel.h>
#include <InertiaDrivers.h>

using AccelerationSource = Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>;
using AngularVelocitySource = Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>;

class MutableAccelerationSource final : public AccelerationSource
{
public:
	Inertia::Model::timestamped_acceleration_t Value{};

	bool GetData(Inertia::Model::timestamped_acceleration_t& data) override
	{
		data = Value;
		return true;
	}
};

class MutableAngularVelocitySource final : public AngularVelocitySource
{
public:
	Inertia::Model::timestamped_angular_velocity_t Value{};

	bool GetData(Inertia::Model::timestamped_angular_velocity_t& data) override
	{
		data = Value;
		return true;
	}
};

bool TestTimestampSamplePeriods()
{
	MutableAngularVelocitySource gyroscope;
	Inertia::Components::Ahrs::Xio::TemplateDriver driver(nullptr, &gyroscope);
	if (!driver.Start())
	{
		return false;
	}

	gyroscope.Value.x = static_cast<int32_t>(90.0f / Inertia::Model::DEG_PER_ANGLE_UNIT);
	gyroscope.Value.timestamp = 1000;
	driver.Step();

	Inertia::Model::timestamped_euler_angle_t orientation{};
	gyroscope.Value.timestamp += 10000;
	driver.Step();
	if (!driver.GetData(orientation))
	{
		return false;
	}
	const float firstRoll = static_cast<float>(orientation.roll) * Inertia::Model::DEG_PER_ANGLE_UNIT;
	if (fabsf(firstRoll - 0.9f) >= 0.15f)
	{
		return false;
	}

	gyroscope.Value.timestamp += 20000;
	driver.Step();
	if (!driver.GetData(orientation))
	{
		return false;
	}
	const float secondRoll = static_cast<float>(orientation.roll) * Inertia::Model::DEG_PER_ANGLE_UNIT;
	return fabsf(secondRoll - 2.7f) < 0.2f;
}

bool TestOverrangeRecovery(bool& resumesGyroscopeUpdates)
{
	resumesGyroscopeUpdates = false;
	MutableAccelerationSource accelerometer;
	MutableAngularVelocitySource gyroscope;
	Inertia::Components::Ahrs::Xio::TemplateDriver driver(&accelerometer, &gyroscope);
	if (!driver.Start())
	{
		return false;
	}

	accelerometer.Value.z = 1000;
	accelerometer.Value.timestamp = 1000;
	gyroscope.Value.timestamp = 1000;
	driver.Step();

	for (uint32_t sample = 0; sample < 400; ++sample)
	{
		accelerometer.Value.timestamp += 10000;
		gyroscope.Value.timestamp = accelerometer.Value.timestamp;
		driver.Step();
	}

	accelerometer.Value.z = 1250;
	gyroscope.Value.x = static_cast<int32_t>(2100.0f / Inertia::Model::DEG_PER_ANGLE_UNIT);
	accelerometer.Value.timestamp += 10000;
	gyroscope.Value.timestamp = accelerometer.Value.timestamp;
	driver.Step();

	Inertia::Model::timestamped_acceleration_t linearAcceleration{};
	const bool preservesAcceleration = driver.GetData(linearAcceleration)
		&& linearAcceleration.timestamp == gyroscope.Value.timestamp
		&& linearAcceleration.x == 0
		&& linearAcceleration.y == 0
		&& abs(static_cast<int>(linearAcceleration.z) - 250) <= 1;

	gyroscope.Value.x = static_cast<int32_t>(90.0f / Inertia::Model::DEG_PER_ANGLE_UNIT);
	accelerometer.Value.timestamp += 10000;
	gyroscope.Value.timestamp = accelerometer.Value.timestamp;
	driver.Step();

	Inertia::Model::timestamped_euler_angle_t orientation{};
	if (driver.GetData(orientation) && orientation.timestamp == gyroscope.Value.timestamp)
	{
		const float resumedRoll = static_cast<float>(orientation.roll) * Inertia::Model::DEG_PER_ANGLE_UNIT;
		resumesGyroscopeUpdates = fabsf(resumedRoll - 0.9f) < 0.15f;
	}

	return preservesAcceleration;
}

void setup()
{
	Serial.begin(115200);
	delay(500);

	const bool samplePeriodsPassed = TestTimestampSamplePeriods();
	bool resumedGyroscopeUpdatesPassed = false;
	const bool overrangeRecoveryPassed = TestOverrangeRecovery(resumedGyroscopeUpdatesPassed);
	Serial.println(samplePeriodsPassed ? F("PASS: timestamp-derived sample periods.") : F("FAIL: timestamp-derived sample periods."));
	Serial.println(overrangeRecoveryPassed ? F("PASS: overrange recovery preserves current acceleration.") : F("FAIL: overrange recovery preserves current acceleration."));
	Serial.println(resumedGyroscopeUpdatesPassed ? F("PASS: in-range gyroscope updates resume after overrange recovery.") : F("FAIL: in-range gyroscope updates resume after overrange recovery."));
	Serial.println(samplePeriodsPassed && overrangeRecoveryPassed && resumedGyroscopeUpdatesPassed ? F("Xio template driver tests PASSED.") : F("Xio template driver tests FAILED."));
}

void loop()
{}