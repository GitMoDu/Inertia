#include <InertiaModel.h>
#include <InertiaDrivers.h>



using Inertia::Drivers::Distance::Vl6180x::Driver;

// Baseline acquisition settings; 63, 0x60 can be tested for more averaging.
static constexpr uint16_t CrosstalkCompensationRate = 0; // 9.7 Mcps; 128 = 1 Mcps.
static constexpr int8_t PartToPartOffsetMm = 0;
static constexpr bool UseCustomOffset = false; // Enable only after measuring the offset.
Driver<TwoWire, 1> GroundDistance(Wire, 0x29, 400000, 100, 50, 49, 0x30,
	CrosstalkCompensationRate, PartToPartOffsetMm, UseCustomOffset);

// The application owns Wire and the sensor's CE/GPIO0 pin. Connect a powered,
// voltage-compatible VL6180X breakout; keep CE high. The VL6180X is a short-range
// sensor (nominally up to 100 mm), not a drop-in replacement for a long-range ToF.
// Start may be retried after boot/busy failures. Transport/reset/timeouts stop
// sampling and require explicit recovery. Sensor-invalid results leave the last
// valid reading, filter and timestamp unchanged; that reading still expires
// after timeout + period without a valid replacement. Errors remain logged.
// Never use Sample when GetData returns false.
//
// Multiple sensors on one bus require independently controlled CE pins:
// 1. Hold all CE pins low; initialize Wire once.
// 2. Release only A, allow it to boot, then A.SetAddress(0x30) while stopped.
// 3. Release B, allow it to boot, then B.SetAddress(0x31) while stopped.
// 4. Start A and B; call each Step regularly. Both objects originally use 0x29.
// Addresses are volatile: CE/power reset returns the hardware to 0x29. Stop,
// reconstruct at 0x29, and repeat isolation/reassignment after a reset. Never
// release two default-address sensors simultaneously on one bus.
// Separate Wire/Wire1 buses can each use 0x29 without reassignment.
//
// Clock selection happens before EVERY transaction, not just at Start, so a
// shared bus may alternate 100/400 kHz devices. Other bus users must also select
// their clock. On platforms unsupported by SetClockIfSupported, configure Wire's
// clock explicitly. Serialize shared-bus access and set a finite Wire timeout.
//
// Board-run fake-bus regression sketch (no sensor required):
// Examples\Testing\UnitTesting\Distance\Vl6180xDriverTest
// Connect the sensor to the board's default Wire SDA/SCL pins.

void setup()
{
	Serial.begin(115200);
	delay(1000);
	Wire.begin();
#if defined(ARDUINO_ARCH_RP2040)
	Wire.setTimeout(5, false);
#endif
	delay(1000);
	Serial.println(F("Boot test"));

	delay(10);
	GroundDistance.InstanceId = 1;
	if (!GroundDistance.Start())
	{
		Serial.println(F("VL6180X start failed"));
		return;
	}
	int8_t factoryOffset = 0;
	if (GroundDistance.ReadFactoryOffset(factoryOffset))
	{
		Serial.print(F("Original hardware offset (mm): "));
		Serial.println(static_cast<int16_t>(factoryOffset));
	}
	else
	{
		Serial.println(F("VL6180X offset read failed"));
	}
}

void loop()
{
	GroundDistance.Step();
	static uint32_t lastPrintMillis = 0;
	if (static_cast<uint32_t>(millis() - lastPrintMillis) >= 100)
	{
		lastPrintMillis = millis();
		Inertia::Drivers::Distance::Vl6180x::Vl6180xDiagnostics diagnostics{};
		if (GroundDistance.GetDiagnostics(diagnostics))
		{
			Serial.print(F("Shot at ")); Serial.print(diagnostics.TimestampMicros);
			Serial.print(F(" us; status=")); Serial.print(diagnostics.RangeStatus);
			Serial.print(F(" uncompensated_mm=")); Serial.print(diagnostics.RawRangeMm);
			Serial.print(F(" compensated_mm=")); Serial.print(diagnostics.RangeValMm);
			Serial.print(F(" return_rate_9_7=")); Serial.println(diagnostics.ReturnRate);
		}
		Inertia::Model::timestamped_quality_range16_t sample{};
		if (GroundDistance.GetData(sample))
		{
			Inertia::Model::timestamped_quality_range16_t raw{};
			if (!GroundDistance.GetRawData(raw))
			{
				Serial.println(F("Range expired while reading raw sample"));
				return;
			}
			Serial.print(sample.distance); Serial.print(F(" mm filtered, "));
			Serial.print(raw.distance); Serial.print(F(" mm unfiltered compensated at "));
			Serial.print(sample.timestamp); Serial.println(F(" us (quality 255 = status valid)"));
		}
		else
		{
			Serial.print(F("Range unavailable; driver error="));
			Serial.print(static_cast<uint8_t>(GroundDistance.GetLastError()));
			Serial.print(F(" sensor status=")); Serial.println(GroundDistance.GetRangeStatus());
		}
	}
	delay(2);
}