#include <Arduino.h>
#include <InertiaModel.h>

using Inertia::Components::Ahrs::Xio::TemplateDriver;
using Inertia::Components::Ahrs::Xio::diagnostics_t;

template<typename T>
class MutableSource final : public Inertia::Components::DataSource::IDataSource<T>
{
public:
	T Value{};
	bool Available = true;

	bool GetData(T& data) override
	{
		if (!Available) return false;
		data = Value;
		return true;
	}
};

using MutableAccelerationSource = MutableSource<Inertia::Model::timestamped_acceleration_t>;
using MutableAngularVelocitySource = MutableSource<Inertia::Model::timestamped_angular_velocity_t>;

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

bool TestDiagnosticsSnapshot()
{
	MutableAccelerationSource accelerometer;
	MutableAngularVelocitySource gyroscope;
	MutableSource<Inertia::Model::timestamped_magnet_t> magnetometer;
	TemplateDriver driver(&accelerometer, &gyroscope, &magnetometer);
	diagnostics_t data{};
	data.SamplePeriodMicros = 123;
	if (driver.GetDiagnostics(data) || data.SamplePeriodMicros != 123 || !driver.Start()) return false;
	accelerometer.Value.z = 1000;
	magnetometer.Value.x = 100;
	driver.Step();
	if (driver.GetDiagnostics(data)) return false;
	accelerometer.Value.timestamp = 10000;
	gyroscope.Value.timestamp = 10000;
	magnetometer.Value.timestamp = 10000;
	driver.Step();
	if (!driver.GetDiagnostics(data) || data.SamplePeriodMicros != 10000
		|| data.Acceleration.z != 1000 || data.Acceleration.timestamp != 10000
		|| data.AngularVelocity.timestamp != 10000 || data.Magnetometer.x != 100
		|| data.Magnetometer.timestamp != 10000 || !data.Flags.startup) return false;
	const float roll = data.Orientation.angle.roll;
	const float bias = data.GyroscopeBiasDegreesPerSecond.axis.x;

	accelerometer.Value.z = 1500;
	magnetometer.Value.x = 200;
	driver.Step();
	if (!driver.GetDiagnostics(data) || data.Acceleration.z != 1000 || data.Magnetometer.x != 100) return false;
	accelerometer.Value.timestamp = 20000;
	gyroscope.Value.timestamp = 20000;
	gyroscope.Available = false;
	driver.Step();
	if (!driver.GetDiagnostics(data) || data.Acceleration.z != 1000
		|| data.AngularVelocity.timestamp != 10000 || data.Orientation.angle.roll != roll
		|| data.GyroscopeBiasDegreesPerSecond.axis.x != bias) return false;
	Inertia::Model::timestamped_euler_angle_t orientation{};
	if (!driver.GetData(orientation) || orientation.timestamp != 10000) return false;

	gyroscope.Available = true;
	driver.Step();
	if (!driver.GetDiagnostics(data) || data.Acceleration.z != 1500
		|| data.AngularVelocity.timestamp != 20000 || data.SamplePeriodMicros != 10000) return false;
	driver.Stop();
	gyroscope.Value.timestamp = 30000;
	driver.Step();
	data.SamplePeriodMicros = 456;
	if (driver.GetDiagnostics(data) || data.SamplePeriodMicros != 456 || driver.GetData(orientation)) return false;
	if (!driver.Start()) return false;
	driver.Step();
	if (driver.GetDiagnostics(data)) return false;
	gyroscope.Value.timestamp = 40000;
	driver.Step();
	return driver.GetDiagnostics(data) && data.SamplePeriodMicros == 10000;
}

bool TestTimestampWrap()
{
	MutableAngularVelocitySource gyroscope;
	TemplateDriver driver(nullptr, &gyroscope);
	if (!driver.Start()) return false;
	gyroscope.Value.timestamp = UINT32_MAX - 4999;
	driver.Step();
	gyroscope.Value.timestamp = 5000;
	driver.Step();
	diagnostics_t data{};
	return driver.GetDiagnostics(data) && data.SamplePeriodMicros == 10000
		&& data.AngularVelocity.timestamp == 5000 && data.Acceleration.z == 0
		&& data.Magnetometer.x == 0;
}

bool TestSettingsCopyAndOverrange()
{
	MutableAngularVelocitySource gyroscope;
	FusionAhrsSettings settings = fusionAhrsDefaultSettings;
	settings.sampleRate = 250.0f;
	settings.gyroscopeRange = 100.0f;
	TemplateDriver driver(nullptr, &gyroscope, nullptr, settings);
	settings.gyroscopeRange = 2000.0f;
	if (!driver.Start()) return false;
	gyroscope.Value.x = static_cast<int32_t>(110.0f / Inertia::Model::DEG_PER_ANGLE_UNIT);
	driver.Step();
	gyroscope.Value.timestamp = 4000;
	driver.Step();
	diagnostics_t data{};
	return driver.GetDiagnostics(data) && data.Flags.overrangeRecovery
		&& data.AngularVelocity.x == gyroscope.Value.x;
}

bool TestBiasSettings()
{
	MutableAngularVelocitySource gyroscope;
	FusionBiasSettings biasSettings = fusionBiasDefaultSettings;
	biasSettings.stationaryThreshold = 20.0f;
	biasSettings.stationaryPeriod = 0.0f;
	TemplateDriver driver(nullptr, &gyroscope, nullptr, fusionAhrsDefaultSettings, biasSettings);
	biasSettings.stationaryThreshold = 0.0f;
	if (!driver.Start()) return false;
	gyroscope.Value.x = static_cast<int32_t>(10.0f / Inertia::Model::DEG_PER_ANGLE_UNIT);
	for (uint32_t i = 0; i <= 20; ++i)
	{
		gyroscope.Value.timestamp = i * 10000;
		driver.Step();
	}
	diagnostics_t data{};
	return driver.GetDiagnostics(data) && data.GyroscopeBiasDegreesPerSecond.axis.x > 0.1f
		&& data.GyroscopeBiasDegreesPerSecond.axis.x < 1.0f;
}

bool TestInvalidSettings()
{
	MutableAngularVelocitySource gyroscope;
	FusionAhrsSettings settings = fusionAhrsDefaultSettings;
	settings.sampleRate = 0.0f;
	TemplateDriver zeroRate(nullptr, &gyroscope, nullptr, settings);
	if (zeroRate.Start()) return false;
	zeroRate.Step();
	gyroscope.Value.timestamp = 10000;
	zeroRate.Step();
	diagnostics_t data{};
	if (zeroRate.GetDiagnostics(data)) return false;
	settings = fusionAhrsDefaultSettings;
	settings.convention = FusionConventionNed;
	TemplateDriver wrongConvention(nullptr, &gyroscope, nullptr, settings);
	if (wrongConvention.Start()) return false;
	settings = fusionAhrsDefaultSettings;
	settings.gain = NAN;
	TemplateDriver nonFinite(nullptr, &gyroscope, nullptr, settings);
	if (nonFinite.Start()) return false;
	settings = fusionAhrsDefaultSettings;
	settings.sampleRate = 2147483648.0f;
	settings.rejectionTimeout = 1.0f;
	TemplateDriver overflowingTimeout(nullptr, &gyroscope, nullptr, settings);
	if (overflowingTimeout.Start()) return false;
	FusionBiasSettings biasSettings = fusionBiasDefaultSettings;
	biasSettings.stationaryPeriod = 4295.0f;
	TemplateDriver overflowingBias(nullptr, &gyroscope, nullptr, fusionAhrsDefaultSettings, biasSettings);
	return !overflowingBias.Start();
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
	const bool diagnosticsPassed = TestDiagnosticsSnapshot();
	const bool wrapPassed = TestTimestampWrap();
	const bool settingsPassed = TestSettingsCopyAndOverrange();
	const bool biasPassed = TestBiasSettings();
	const bool invalidSettingsPassed = TestInvalidSettings();
	Serial.println(diagnosticsPassed ? F("PASS: coherent diagnostics, timestamp zero and lifecycle.") : F("FAIL: coherent diagnostics, timestamp zero and lifecycle."));
	Serial.println(wrapPassed ? F("PASS: timestamp wrap.") : F("FAIL: timestamp wrap."));
	Serial.println(settingsPassed ? F("PASS: copied settings control overrange recovery.") : F("FAIL: copied settings control overrange recovery."));
	Serial.println(biasPassed ? F("PASS: copied bias settings control learning.") : F("FAIL: copied bias settings control learning."));
	Serial.println(invalidSettingsPassed ? F("PASS: invalid settings rejected.") : F("FAIL: invalid settings rejected."));
	Serial.println(samplePeriodsPassed && overrangeRecoveryPassed && resumedGyroscopeUpdatesPassed
		&& diagnosticsPassed && wrapPassed && settingsPassed && biasPassed && invalidSettingsPassed
		? F("Xio template driver tests PASSED.") : F("Xio template driver tests FAILED."));
}

void loop()
{}