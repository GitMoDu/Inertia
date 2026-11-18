#ifndef _INERTIA_VL6180X_DRIVER_TESTS_h
#define _INERTIA_VL6180X_DRIVER_TESTS_h

#include <Arduino.h>
#include <Wire.h>
#include <InertiaModel.h>
#include <Drivers/Distance/VL6180X/Calibration.h>
#include <Drivers/HardwareInterface/I2c/I2cInterface.h>

static uint32_t TestMillis = 0;
static uint32_t TestMicros = 0;
static uint32_t Vl6180xTestMillis() { return TestMillis; }
static uint32_t Vl6180xTestMicros() { return TestMicros; }

// Compile the actual driver against deterministic time, not a second implementation.
#define millis Vl6180xTestMillis
#define micros Vl6180xTestMicros
#include <Drivers/Distance/VL6180X/Driver.h>
#undef millis
#undef micros

namespace Vl6180xTests
{
	using Inertia::Drivers::Distance::Vl6180x::ErrorEnum;
	using Inertia::Drivers::Distance::Vl6180x::RangeStatusEnum;
	static_assert(sizeof(RangeStatusEnum) == 1 && sizeof(ErrorEnum) == 1, "Status codes must remain bytes.");
	static_assert(static_cast<uint8_t>(RangeStatusEnum::Valid) == 0
		&& static_cast<uint8_t>(RangeStatusEnum::VcselContinuityTest) == 1
		&& static_cast<uint8_t>(RangeStatusEnum::VcselWatchdogTest) == 2
		&& static_cast<uint8_t>(RangeStatusEnum::VcselWatchdog) == 3
		&& static_cast<uint8_t>(RangeStatusEnum::Pll1Lock) == 4
		&& static_cast<uint8_t>(RangeStatusEnum::Pll2Lock) == 5
		&& static_cast<uint8_t>(RangeStatusEnum::EarlyConvergenceEstimate) == 6
		&& static_cast<uint8_t>(RangeStatusEnum::MaxConvergence) == 7
		&& static_cast<uint8_t>(RangeStatusEnum::NoTargetIgnore) == 8
		&& static_cast<uint8_t>(RangeStatusEnum::Reserved9) == 9
		&& static_cast<uint8_t>(RangeStatusEnum::Reserved10) == 10
		&& static_cast<uint8_t>(RangeStatusEnum::MaxSignalToNoiseRatio) == 11
		&& static_cast<uint8_t>(RangeStatusEnum::RawRangingUnderflow) == 12
		&& static_cast<uint8_t>(RangeStatusEnum::RawRangingOverflow) == 13
		&& static_cast<uint8_t>(RangeStatusEnum::RangingUnderflow) == 14
		&& static_cast<uint8_t>(RangeStatusEnum::RangingOverflow) == 15
		&& static_cast<uint8_t>(RangeStatusEnum::NoReading) == 255, "Hardware status decoder contract changed.");
	static_assert(static_cast<uint8_t>(ErrorEnum::None) == 0
		&& static_cast<uint8_t>(ErrorEnum::InvalidConfiguration) == 1
		&& static_cast<uint8_t>(ErrorEnum::I2c) == 2
		&& static_cast<uint8_t>(ErrorEnum::WrongModel) == 3
		&& static_cast<uint8_t>(ErrorEnum::NotReady) == 4
		&& static_cast<uint8_t>(ErrorEnum::Timeout) == 5
		&& static_cast<uint8_t>(ErrorEnum::SensorStatus) == 6
		&& static_cast<uint8_t>(ErrorEnum::Interrupt) == 7
		&& static_cast<uint8_t>(ErrorEnum::UnexpectedReset) == 8
		&& static_cast<uint8_t>(ErrorEnum::AddressChangeWhileRunning) == 9, "Driver log code contract changed.");
	using Sample = Inertia::Model::timestamped_quality_range16_t;

	struct FakeWire
	{
		struct WriteEntry { uint16_t Register; uint8_t Value; uint8_t Address; };
		uint8_t Registers[2][0x300]{};
		uint8_t Addresses[2] = { 0x29, 0x29 };
		bool Enabled[2] = { true, false };
		WriteEntry Writes[128]{};
		uint16_t WriteCount = 0;
		uint16_t Transactions = 0;
		uint32_t Clock = 0;
		uint32_t ExpectedClock = 400000;
		bool ClockSelected = false;
		bool ClockCorrect = true;
		bool RepeatedStartCorrect = true;
		uint8_t NackNext = 0;
		uint16_t FailRegister = 0xFFFF;
		bool ShortRead = false;
		bool MissingAvailable = false;
		bool NegativeRead = false;
		bool ShortWrite = false;
		bool ShortWriteStopped = false;
		uint8_t Buffer[3]{};
		uint8_t Length = 0;
		uint8_t Target = 0;
		uint16_t Index = 0;
		bool HasByte = false;

		FakeWire()
		{
			for (uint8_t i = 0; i < 2; ++i)
			{
				Registers[i][0] = 0xB4;
				Registers[i][0x16] = 1;
				Registers[i][0x4D] = 1;
			}
		}

		void setClock(uint32_t clock) { Clock = clock; ClockSelected = true; }
		void Transaction()
		{
			++Transactions;
			ClockCorrect &= ClockSelected && Clock == ExpectedClock;
			ClockSelected = false;
		}
		void beginTransmission(uint8_t address)
		{
			Transaction(); Target = address; Length = 0;
		}
		size_t write(uint8_t value)
		{
			if (ShortWrite) return 0;
			if (Length >= sizeof(Buffer)) return 0;
			Buffer[Length++] = value;
			return 1;
		}
		int Find(uint8_t address)
		{
			for (uint8_t i = 0; i < 2; ++i)
				if (Enabled[i] && Addresses[i] == address) return i;
			return -1;
		}
		uint8_t endTransmission(bool stop = true)
		{
			if (NackNext) { --NackNext; return 2; }
			const int device = Find(Target);
			if (Length < 2) { ShortWriteStopped = stop; return 2; }
			if (device < 0) return 2;
			Index = static_cast<uint16_t>((Buffer[0] << 8) | Buffer[1]);
			if (Index >= 0x300 || Index == FailRegister) return 3;
			if (Length == 2)
			{
				RepeatedStartCorrect &= !stop;
				return 0;
			}
			RepeatedStartCorrect &= stop;
			if (WriteCount < 128) Writes[WriteCount] = { Index, Buffer[2], Target };
			++WriteCount;
			Registers[device][Index] = Buffer[2];
			if (Index == 0x212) Addresses[device] = Buffer[2];
			if (Index == 0x15) Registers[device][0x4F] = 0;
			if (Index == 0x18) Registers[device][0x4D] &= 0xFE;
			return 0;
		}
		uint8_t requestFrom(uint8_t address, uint8_t count)
		{
			Transaction(); Target = address;
			HasByte = !ShortRead && !MissingAvailable && Find(address) >= 0 && count == 1;
			return ShortRead ? 0 : 1;
		}
		int available() { return HasByte ? 1 : 0; }
		int read()
		{
			if (!HasByte || NegativeRead) { HasByte = false; return -1; }
			HasByte = false;
			return Registers[Find(Target)][Index];
		}
		void Complete(uint8_t distance, uint8_t status = 0, uint8_t device = 0, uint8_t interrupt = 4)
		{
			Registers[device][0x62] = distance;
			Registers[device][0x4D] = static_cast<uint8_t>((status << 4) | 1);
			Registers[device][0x4F] = interrupt;
		}
	};

	using Driver = Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire>;
	struct Log : Inertia::Model::ILogListener
	{
		uint16_t Count = 0;
		Inertia::Model::LogEntryStruct Last{};
		void OnLog(const Inertia::Model::LogEntryStruct& entry) final { ++Count; Last = entry; }
	};

	static void Advance(uint32_t millis)
	{
		TestMillis += millis;
		TestMicros += millis * 1000;
	}

	static void Run(void (*check)(const char*, bool))
	{
		TestMillis = 100;
		TestMicros = 100000;
		{
			FakeWire wire;
			Driver driver(wire);
			Sample out{};
			out.distance = 1234; out.timestamp = 4321; out.quality = 17;
			driver.Step();
			check("Stopped: no transactions; GetData preserves output", wire.Transactions == 0
				&& !driver.GetData(out) && out.distance == 1234 && out.timestamp == 4321 && out.quality == 17);
			check("Start initializes correct model", driver.Start() && driver.IsRunning());
			const uint16_t mandatoryRegisters[] = { 0x207, 0x208, 0x96, 0x97, 0xE3, 0xE4, 0xE5, 0xE6,
				0xE7, 0xF5, 0xD9, 0xDB, 0xDC, 0xDD, 0x9F, 0xA3, 0xB7, 0xBB, 0xB2, 0xCA,
				0x198, 0x1B0, 0x1AD, 0xFF, 0x100, 0x199, 0x1A6, 0x1AC, 0x1A7, 0x30 };
			const uint8_t mandatoryValues[] = { 1, 1, 0, 0xFD, 0, 4, 2, 1, 3, 2, 5, 0xCE, 3, 0xF8,
				0, 0x3C, 0, 0x3C, 9, 9, 1, 0x17, 0, 5, 5, 5, 0x1B, 0x3E, 0x1F, 0 };
			bool sequence = wire.WriteCount == 40;
			for (uint8_t i = 0; i < 30; ++i)
				sequence &= wire.Writes[i].Register == mandatoryRegisters[i] && wire.Writes[i].Value == mandatoryValues[i];
			check("Complete ST SR03 sequence, interrupt and reset marker", sequence
				&& wire.Registers[0][0x10A] == 0x30 && wire.Registers[0][0x1C] == 49
				&& wire.Registers[0][0x14] == 4 && wire.Registers[0][0x16] == 0);
			const uint16_t initialized = wire.Transactions;
			check("Start is idempotent and cache initially unavailable", driver.Start()
				&& wire.Transactions == initialized && !driver.GetData(out));
			driver.Step();
			check("First Step starts single shot, no fabricated sample", wire.Registers[0][0x18] == 1 && !driver.GetData(out));
			driver.Step();
			check("Not-ready returns without publishing or failing", driver.IsRunning()
				&& driver.GetLastError() == ErrorEnum::None && !driver.GetData(out));
			Advance(10); wire.Complete(74); driver.Step();
			check("Valid mm sample, acquisition timestamp, binary quality and interrupt clear", driver.GetData(out)
				&& out.distance == 74 && out.timestamp == TestMicros && out.quality == 255 && wire.Registers[0][0x4F] == 0);
			Sample again{};
			check("GetData is non-consuming", driver.GetData(again) && again.timestamp == out.timestamp && again.distance == 74);
			const uint16_t writes = wire.WriteCount;
			driver.Step(); Advance(39); driver.Step();
			check("Driver enforces 50ms minimum start period", wire.WriteCount == writes);
			Advance(1); driver.Step();
			check("Next shot starts at cadence and retains valid cache", wire.WriteCount == writes + 1 && driver.GetData(out));
			wire.Complete(0); driver.Step();
			check("Genuine status-valid zero is representable", driver.GetData(out) && out.distance == 0);
			Advance(150);
			out.distance = 1234;
			check("Stale sample unavailable without needing Step", !driver.GetData(out) && out.distance == 1234);
			check("Clock selected for EVERY transaction, 16-bit index repeated start", wire.ClockCorrect && wire.RepeatedStartCorrect);
			driver.Stop(); const uint16_t stopped = wire.Transactions; driver.Step();
			check("Stop invalidates and leaves shared bus alone", !driver.GetData(out) && wire.Transactions == stopped);
			check("Restart reinitializes", driver.Start() && !driver.GetData(out));
		}
		{
			FakeWire wire; Driver driver(wire); Log log; driver.LogListener = &log;
			driver.Start();
			for (uint8_t status = 1; status <= 5; ++status)
			{
				Advance(50); driver.Step(); wire.Complete(255, status); driver.Step();
				check("Each hardware fault logs one instance-tagged Error", log.Count == status
					&& log.Last.Type == Inertia::Model::LogTypeEnum::Error && log.Last.Code == 6
					&& log.Last.Value == status && driver.IsRunning());
			}
			for (uint16_t i = 0; i < 300; ++i)
			{
				Advance(50); driver.Step(); wire.Complete(255, static_cast<uint8_t>(1 + i % 5)); driver.Step();
			}
			check("Alternating persistent hardware faults never produce reminders", log.Count == 5);
			Advance(50); driver.Step(); wire.Complete(50); driver.Step();
			Sample out{};
			check("Single valid shot restores measurement immediately", driver.GetData(out)
				&& out.distance == 50 && driver.GetLastError() == ErrorEnum::None);
			Advance(50); driver.Step(); wire.Complete(255, 1); driver.Step();
			check("Single valid shot does not rearm hardware fault logging", log.Count == 5);
			for (uint8_t i = 0; i < 4; ++i)
			{
				Advance(50); driver.Step(); wire.Complete(50); driver.Step();
			}
			Advance(50); driver.Step(); wire.Complete(255, 6); driver.Step();
			Advance(50); driver.Step(); wire.Complete(255, 1); driver.Step();
			check("Routine rejection interrupts recovery without logging", log.Count == 5);
			for (uint8_t i = 0; i < 5; ++i)
			{
				Advance(50); driver.Step(); wire.Complete(50); driver.Step();
			}
			Advance(50); driver.Step(); wire.Complete(255, 1); driver.Step();
			check("Five valid shots rearm hardware fault reporting", log.Count == 6 && log.Last.Value == 1);
			driver.Stop(); driver.Start(); driver.Step(); wire.Complete(255, 1); driver.Step();
			check("Restart rearms hardware fault reporting", log.Count == 7 && log.Last.Type == Inertia::Model::LogTypeEnum::Error);
			wire.NackNext = 1; driver.Step();
			check("Transport failure after hardware status remains an Error", log.Count == 8
				&& log.Last.Code == static_cast<uint8_t>(ErrorEnum::I2c) && !driver.IsRunning());
		}
		{
			FakeWire wire;
			Driver driver(wire);
			Log log; driver.LogListener = &log; driver.InstanceId = 7;
			bool statuses = driver.Start();
			for (uint8_t status = 1; status <= 15; ++status)
			{
				Advance(50); driver.Step(); wire.Complete(99, status); driver.Step();
				Sample out{}; out.distance = 1234;
				statuses &= !driver.GetData(out) && out.distance == 1234 && driver.GetRangeStatus() == status
					&& driver.GetLastError() == ErrorEnum::SensorStatus && driver.IsRunning() && wire.Registers[0][0x4F] == 0;
			}
			check("All 15 nonzero sensor statuses publish no initial sample; interrupts cleared", statuses);
			check("Only five hardware statuses log Errors and remain instance-tagged", log.Count == 5 && log.Last.Instance == 7
				&& log.Last.Tag == Inertia::Drivers::Distance::Vl6180x::LOG_TAG && log.Last.Value == 5
				&& log.Last.Type == Inertia::Model::LogTypeEnum::Error);
			for (uint8_t i = 0; i < 4; ++i)
			{
				Advance(50); driver.Step(); wire.Complete(88); driver.Step();
			}
			Advance(50); driver.Step(); wire.Complete(88); driver.Step(); Sample out{};
			check("Valid measurement automatically recovers sensor-status error", driver.GetData(out)
				&& out.distance == 88 && driver.GetLastError() == ErrorEnum::None);
			const uint32_t timestamp = out.timestamp;
			Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			check("Occasional underflow preserves sample without logging", driver.GetData(out)
				&& out.distance == 88 && out.timestamp == timestamp && out.quality == 255
				&& driver.GetLastError() == ErrorEnum::SensorStatus && log.Count == 5);
			Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			check("Repeated underflow retains only the original unexpired sample", driver.GetData(out)
				&& out.distance == 88 && out.timestamp == timestamp && log.Count == 5);
			Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			out.distance = 1234;
			check("Invalid shots do not refresh freshness at the 150ms deadline", !driver.GetData(out)
				&& !driver.GetRawData(out) && out.distance == 1234 && driver.IsRunning());
			for (uint16_t i = 0; i < 300; ++i)
			{
				Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			}
			check("Persistent raw underflow emits neither escalation nor reminders", log.Count == 5
				&& driver.GetRangeStatus() == 12 && driver.GetLastError() == ErrorEnum::SensorStatus && driver.IsRunning());
			const uint8_t quietStatuses[] = { 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
			for (const uint8_t status : quietStatuses)
			{
				driver.Stop(); driver.Start();
				for (uint16_t i = 0; i < 300; ++i)
				{
					Advance(50); driver.Step(); wire.Complete(255, status); driver.Step();
				}
				check("Routine rejection stays quiet from startup through persistent reminders", log.Count == 5
					&& driver.GetRangeStatus() == status && driver.GetLastError() == ErrorEnum::SensorStatus && driver.IsRunning());
			}
			wire.NackNext = 1; driver.Step();
			check("Transport faults remain logged after suppressed underflow", log.Count == 6
				&& log.Last.Type == Inertia::Model::LogTypeEnum::Error
				&& log.Last.Code == static_cast<uint8_t>(ErrorEnum::I2c) && !driver.IsRunning());
		}
		{
			FakeWire wire;
			Driver driver(wire);
			driver.Start(); driver.Step(); Advance(100); wire.Complete(42); driver.Step(); Sample out{};
			check("Late-ready result rejected at deadline; requires explicit restart", !driver.IsRunning()
				&& !driver.GetData(out) && driver.GetLastError() == ErrorEnum::Timeout);
			check("Timeout recovery via Start", driver.Start());
			driver.Step(); Advance(100); driver.Step();
			check("Never-ready result times out without busy waiting", driver.GetLastError() == ErrorEnum::Timeout && !driver.IsRunning());
		}
		{
			FakeWire wire;
			Driver driver(wire);
			driver.Start(); wire.Registers[0][0x4D] = 0; driver.Step(); Advance(100); driver.Step();
			check("Readiness wait is nonblocking and bounded", driver.GetLastError() == ErrorEnum::Timeout && !driver.IsRunning());
		}
		{
			TestMillis = UINT32_MAX - 20; TestMicros = UINT32_MAX - 20000;
			FakeWire wire; Driver driver(wire); driver.Start(); driver.Step(); Advance(10);
			wire.Complete(63); driver.Step(); Sample out{}; Advance(40); driver.Step();
			check("Millis/micros wrap preserves cadence and freshness", driver.GetData(out) && out.distance == 63
				&& out.timestamp == UINT32_MAX - 10000 && wire.Registers[0][0x18] == 1);
			Advance(100); driver.Step();
			check("Timeout works across clock rollover", driver.GetLastError() == ErrorEnum::Timeout && !driver.GetData(out));
		}
		{
			FakeWire wire; Driver driver(wire); wire.Registers[0][0] = 0;
			check("Wrong model rejected before initialization", !driver.Start() && driver.GetLastError() == ErrorEnum::WrongModel && wire.WriteCount == 0);
			wire.Registers[0][0] = 0xB4; wire.Registers[0][0x4D] = 0;
			check("Busy boot fails without spinning", !driver.Start() && driver.GetLastError() == ErrorEnum::NotReady);
			wire.Registers[0][0x4D] = 1; wire.FailRegister = 0x1AD;
			check("Mandatory initialization write failure aborts", !driver.Start() && !driver.IsRunning()
				&& driver.GetLastError() == ErrorEnum::I2c && wire.Registers[0][0x16] == 1);
		}
		for (uint8_t fault = 0; fault < 7; ++fault)
		{
			FakeWire wire; Driver driver(wire); driver.Start(); driver.Step(); wire.Complete(66); driver.Step();
			if (fault == 0) wire.NackNext = 1;
			if (fault == 1) wire.ShortRead = true;
			if (fault == 2) wire.MissingAvailable = true;
			if (fault == 3) wire.NegativeRead = true;
			if (fault == 4) wire.ShortWrite = true;
			if (fault == 5) wire.Registers[0][0x16] = 1;
			if (fault == 6) { Advance(50); driver.Step(); wire.Complete(33, 0, 0, 0x44); }
			driver.Step(); Sample out{}; out.distance = 1234;
			const ErrorEnum expected = fault == 5 ? ErrorEnum::UnexpectedReset : fault == 6 ? ErrorEnum::Interrupt : ErrorEnum::I2c;
			check("Transport/reset/interrupt fault withdraws cache and stops", !driver.GetData(out)
				&& out.distance == 1234 && !driver.IsRunning() && driver.GetLastError() == expected);
			if (fault == 4) check("Short index write releases bus with STOP", wire.ShortWriteStopped);
		}
		for (uint8_t fault = 0; fault < 3; ++fault)
		{
			FakeWire wire; Driver driver(wire); driver.Start(); driver.Step(); wire.Complete(55);
			wire.FailRegister = fault == 0 ? 0x4D : fault == 1 ? 0x62 : 0x15;
			driver.Step(); Sample out{};
			check("Status/range/interrupt-clear failure cannot publish partial result", !driver.GetData(out)
				&& driver.GetLastError() == ErrorEnum::I2c && !driver.IsRunning());
		}
		{
			FakeWire wire; Driver driver(wire, 0x29, 100000); wire.ExpectedClock = 100000;
			check("SetAddress writes old address then verifies new address", driver.SetAddress(0x30)
				&& driver.GetAddress() == 0x30 && wire.Writes[0].Register == 0x212
				&& wire.Writes[0].Address == 0x29 && wire.Writes[0].Value == 0x30);
			wire.Enabled[1] = true;
			Driver second(wire, 0x29, 400000);
			wire.ExpectedClock = 400000;
			bool multiple = second.SetAddress(0x31) && second.Start();
			wire.ExpectedClock = 100000; multiple &= driver.Start(); driver.Step();
			wire.ExpectedClock = 400000; second.Step();
			wire.Complete(61); wire.Complete(92, 0, 1);
			wire.ExpectedClock = 100000; driver.Step();
			wire.ExpectedClock = 400000; second.Step(); Sample firstOut{}, secondOut{};
			check("Two units on shared bus retain independent address/data/clock", multiple && driver.GetData(firstOut)
				&& second.GetData(secondOut) && firstOut.distance == 61 && secondOut.distance == 92 && wire.ClockCorrect);
			driver.Stop(); wire.Addresses[0] = 0x29; wire.Registers[0][0x16] = 1; wire.ExpectedClock = 100000;
			check("Power-reset does not silently probe or alter stored address", !driver.Start() && driver.GetAddress() == 0x30);
			Driver reset(wire, 0x29, 100000);
			check("Reconstruct at boot address and reassign after CE reset", reset.SetAddress(0x30) && reset.Start());
		}
		{
			FakeWire a, b; Driver first(a), second(b);
			bool independent = first.Start() && second.Start(); first.Step(); second.Step(); a.Complete(12); b.Complete(34);
			first.Step(); second.Step(); Sample x{}, y{};
			check("Separate buses may use the same address", independent && first.GetData(x) && second.GetData(y)
				&& x.distance == 12 && y.distance == 34);
		}
		{
			FakeWire wire; Driver driver(wire); wire.FailRegister = 0x212;
			check("Address-write failure preserves local address", !driver.SetAddress(0x30)
				&& driver.GetAddress() == 0x29 && driver.GetLastError() == ErrorEnum::I2c);
			wire.FailRegister = 0; // ACK the address write, then fail verification at the new address.
			check("Verification failure records acknowledged new address but reports error", !driver.SetAddress(0x30)
				&& driver.GetAddress() == 0x30 && driver.GetLastError() == ErrorEnum::I2c);
		}
		{
			FakeWire wire;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 2> driver(wire);
			Sample filtered{}, raw{};
			filtered.distance = 1234; raw.distance = 4321;
			check("Filtered/raw cache unavailable before start without changing outputs", !driver.GetData(filtered)
				&& !driver.GetRawData(raw) && filtered.distance == 1234 && raw.distance == 4321);
			driver.Start(); driver.Step(); wire.Complete(20); driver.Step();
			check("First acquisition seeds filter without zero ramp", driver.GetData(filtered) && filtered.distance == 20
				&& driver.GetRawData(raw) && raw.distance == 20 && filtered.timestamp == raw.timestamp && filtered.quality == 255);
			Advance(50); driver.Step(); wire.Complete(100); driver.Step();
			check("Source filter alpha one quarter: 20 to 100 yields 40", driver.GetData(filtered)
				&& filtered.distance == 40 && driver.GetRawData(raw) && raw.distance == 100
				&& filtered.timestamp == TestMicros && raw.timestamp == filtered.timestamp && raw.quality == filtered.quality);
			const uint16_t transactions = wire.Transactions;
			const uint32_t timestamp = filtered.timestamp;
			bool getters = true;
			for (uint8_t read = 0; read < 100; ++read)
				getters &= driver.GetData(filtered) && filtered.distance == 40 && filtered.timestamp == timestamp
					&& driver.GetRawData(raw) && raw.distance == 100;
			check("Repeated Surface/getter reads do not refilter or access bus", getters && wire.Transactions == transactions);
			driver.Step(); Advance(50); driver.Step(); driver.Step();
			check("Pending measurement does not advance filter or timestamp", driver.GetData(filtered)
				&& filtered.distance == 40 && filtered.timestamp == timestamp);
			wire.Complete(100); driver.Step();
			check("New acquisition advances filter exactly once: 40 to 100 yields 55", driver.GetData(filtered) && filtered.distance == 55);
			const uint32_t validTimestamp = filtered.timestamp;
			Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			check("Underflow leaves filtered/raw readings and timestamps unchanged", driver.GetData(filtered)
				&& driver.GetRawData(raw) && filtered.distance == 55 && raw.distance == 100
				&& filtered.timestamp == validTimestamp && raw.timestamp == validTimestamp
				&& filtered.quality == 255 && raw.quality == 255);
			Advance(50); driver.Step(); wire.Complete(75); driver.Step();
			check("Recovery continues filter history: 55 to 75 yields 60", driver.GetData(filtered)
				&& filtered.distance == 60 && filtered.timestamp == TestMicros && driver.GetRawData(raw) && raw.distance == 75
				&& driver.GetLastError() == ErrorEnum::None);
			Advance(50); driver.Step(); wire.Complete(0); driver.Step();
			check("Valid zero participates in filtering and remains available raw", driver.GetData(filtered)
				&& filtered.distance == 45 && driver.GetRawData(raw) && raw.distance == 0);
			Advance(150);
			check("Stale filtered/raw samples remain unavailable", !driver.GetData(filtered) && !driver.GetRawData(raw));
			driver.Step(); wire.Complete(6); driver.Step();
			check("Fresh sample after stale gap seeds directly without old history", driver.GetData(filtered) && filtered.distance == 6);
			Advance(50); driver.Step(); wire.Complete(5); driver.Step();
			Advance(50); driver.Step(); wire.Complete(7); driver.Step();
			check("Millimetre noise retains fractional history", driver.GetData(filtered) && filtered.distance == 6
				&& driver.GetRawData(raw) && raw.distance == 7);
			driver.Stop();
			check("Stop invalidates filtered and raw caches", !driver.GetData(filtered) && !driver.GetRawData(raw));
			driver.Start(); driver.Step(); wire.Complete(90); driver.Step();
			check("Restart reseeds filter", driver.GetData(filtered) && filtered.distance == 90);
			wire.NackNext = 1; driver.Step();
			check("Transport fault cannot leak filtered history", !driver.GetData(filtered) && !driver.GetRawData(raw) && !driver.IsRunning());
		}
		{
			FakeWire wire;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 1> driver(wire, 0x29, 400000, 100, 50, 63, 0x60);
			Sample out{};
			driver.Start(); driver.Step(); wire.Complete(20); driver.Step();
			Advance(50); driver.Step(); wire.Complete(100); driver.Step();
			check("Vehicle alpha one half responds 20 to 100 with 60", driver.GetData(out)
				&& out.distance == 60 && out.timestamp == TestMicros);
			const uint32_t timestamp = out.timestamp;
			Advance(50); driver.Step(); wire.Complete(255, 12); driver.Step();
			check("Faster filter still skips sensor-invalid shots", driver.GetData(out)
				&& out.distance == 60 && out.timestamp == timestamp);
			Advance(50); driver.Step(); wire.Complete(100); driver.Step();
			check("Faster filter continues retained history with 80", driver.GetData(out)
				&& out.distance == 80 && out.timestamp == TestMicros);
		}
		{
			FakeWire a, b;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 2> first(a), second(b);
			first.Start(); second.Start(); first.Step(); second.Step();
			a.Complete(20); b.Complete(160); first.Step(); second.Step();
			Advance(50); first.Step(); second.Step();
			a.Complete(100); b.Complete(80); first.Step(); second.Step();
			Sample x{}, y{};
			check("Multiple sensor instances keep independent filter histories", first.GetData(x) && second.GetData(y)
				&& x.distance == 40 && y.distance == 140);
		}
		{
			TestMillis = UINT32_MAX - 20; TestMicros = UINT32_MAX - 20000;
			FakeWire wire;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 2> driver(wire);
			driver.Start(); driver.Step(); wire.Complete(20); driver.Step();
			Advance(50); driver.Step(); wire.Complete(100); driver.Step(); Sample out{};
			check("Filter history survives clock rollover", driver.GetData(out) && out.distance == 40 && out.timestamp == TestMicros);
		}
		{
			FakeWire wire;
			Driver driver(wire, 0x29, 400000, 100, 50, 63, 0x60);
			check("Surface-noise profile programs convergence and internal averaging", driver.Start()
				&& wire.Registers[0][0x1C] == 63 && wire.Registers[0][0x10A] == 0x60);
			driver.Stop(); wire.Registers[0][0x1C] = 2; wire.Registers[0][0x10A] = 0;
			check("Restart reapplies hardware tuning", driver.Start()
				&& wire.Registers[0][0x1C] == 63 && wire.Registers[0][0x10A] == 0x60);
			const uint16_t tuningRegisters[] = { 0x1C, 0x10A };
			for (uint16_t reg : tuningRegisters)
			{
				driver.Stop(); wire.FailRegister = reg;
				check("Tuning write failure stops startup explicitly", !driver.Start() && !driver.IsRunning()
					&& driver.GetLastError() == ErrorEnum::I2c);
			}
			FakeWire baselineWire;
			Driver baseline(baselineWire);
			check("Hardware tuning is per-instance, not shared through initialization table", baseline.Start()
				&& baselineWire.Registers[0][0x1C] == 49 && baselineWire.Registers[0][0x10A] == 0x30);
		}
		{
			FakeWire wire;
			Driver tooShort(wire, 0x29, 400000, 100, 50, 1);
			Driver tooLong(wire, 0x29, 400000, 100, 50, 64);
			check("Invalid convergence budget rejected without bus access", !tooShort.Start() && !tooLong.Start()
				&& tooShort.GetLastError() == ErrorEnum::InvalidConfiguration
				&& tooLong.GetLastError() == ErrorEnum::InvalidConfiguration && wire.Transactions == 0);
			Driver minimum(wire, 0x29, 400000, 100, 50, 2, 0);
			check("Minimum convergence and averaging settings are accepted", minimum.Start()
				&& wire.Registers[0][0x1C] == 2 && wire.Registers[0][0x10A] == 0);
		}
		{
			FakeWire wire;
			wire.Registers[0][0x24] = 0xE2;
			wire.Registers[0][0x1E] = 0xFF; wire.Registers[0][0x1F] = 0xFF;
			Driver driver(wire);
			int8_t offset = 0;
			check("Default calibration disables crosstalk and preserves signed factory offset", driver.Start()
				&& wire.Registers[0][0x1E] == 0 && wire.Registers[0][0x1F] == 0
				&& wire.Registers[0][0x24] == 0xE2 && driver.ReadFactoryOffset(offset) && offset == -30);
			bool untouched = true;
			for (uint16_t i = 0; i < wire.WriteCount; ++i) untouched &= wire.Writes[i].Register != 0x24;
			check("Default startup never writes the factory offset register", untouched);
		}
		{
			FakeWire wire; wire.Registers[0][0x24] = 7;
			Driver driver(wire, 0x29, 400000, 100, 50, 49, 0x30, 0x1234, -30, true);
			int8_t offset = 0;
			check("Custom calibration writes big-endian 9.7 rate and signed offset", driver.Start()
				&& wire.Registers[0][0x1E] == 0x12 && wire.Registers[0][0x1F] == 0x34
				&& wire.Registers[0][0x24] == 0xE2);
			check("Calibration applied after SR03 and before reset marker clear", wire.WriteCount == 41
				&& wire.Writes[37].Register == 0x1E && wire.Writes[38].Register == 0x1F
				&& wire.Writes[39].Register == 0x24 && wire.Writes[40].Register == 0x16);
			const uint16_t transactions = wire.Transactions;
			check("Factory offset remains inspectable after override without new bus access", driver.ReadFactoryOffset(offset)
				&& offset == 7 && wire.Transactions == transactions);
			driver.Stop(); wire.Registers[0][0x24] = 0; wire.Registers[0][0x1F] = 0;
			check("Restart reapplies calibration and preserves original factory snapshot", driver.Start()
				&& wire.Registers[0][0x24] == 0xE2 && wire.Registers[0][0x1F] == 0x34
				&& driver.ReadFactoryOffset(offset) && offset == 7);
		}
		{
			const int16_t offsets[] = { -128, -1, 0, 127 };
			bool boundaries = true;
			for (int16_t offset : offsets)
			{
				FakeWire wire; wire.Registers[0][0x24] = static_cast<uint8_t>(offset);
				Driver driver(wire, 0x29, 400000, 100, 50, 49, 0x30, UINT16_MAX, static_cast<int8_t>(offset), true);
				int8_t original = 0;
				boundaries &= driver.ReadFactoryOffset(original) && original == offset && driver.Start()
					&& wire.Registers[0][0x24] == static_cast<uint8_t>(offset)
					&& wire.Registers[0][0x1E] == 255 && wire.Registers[0][0x1F] == 255;
			}
			check("Offset signed boundaries, explicit zero and maximum compensation are representable", boundaries);
		}
		{
			const uint16_t registers[] = { 0x24, 0x1E, 0x1F, 0x16 };
			for (uint16_t reg : registers)
			{
				FakeWire wire; wire.FailRegister = reg; Driver driver(wire); Log log; driver.LogListener = &log;
				check("Calibration read/write failure aborts startup and logs I2c", !driver.Start() && !driver.IsRunning()
					&& driver.GetLastError() == ErrorEnum::I2c && wire.Registers[0][0x16] == 1
					&& log.Count == 1 && log.Last.Type == Inertia::Model::LogTypeEnum::Error);
			}
			FakeWire wire; Driver driver(wire, 0x29, 400000, 100, 50, 49, 0x30, 0, -1, true);
			int8_t original = 0; driver.ReadFactoryOffset(original); wire.FailRegister = 0x24;
			check("Offset override write failure cannot clear fresh-reset marker", !driver.Start()
				&& driver.GetLastError() == ErrorEnum::I2c && wire.Registers[0][0x16] == 1);
			FakeWire failed; Driver failedDriver(failed); failed.NackNext = 1; original = 42;
			check("Factory-offset read failure leaves caller output untouched", !failedDriver.ReadFactoryOffset(original)
				&& original == 42 && failedDriver.GetLastError() == ErrorEnum::I2c);
		}
		{
			using Diagnostics = Inertia::Drivers::Distance::Vl6180x::Vl6180xDiagnostics;
			FakeWire wire;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 1> driver(wire);
			Diagnostics diagnostics{}; diagnostics.RawRangeMm = 99;
			check("Diagnostics unavailable before Start without altering caller output", !driver.GetDiagnostics(diagnostics)
				&& diagnostics.RawRangeMm == 99 && wire.Transactions == 0);
			driver.Start(); driver.Step();
			check("Diagnostics unavailable before first completed shot", !driver.GetDiagnostics(diagnostics));
			wire.Registers[0][0x64] = 70; wire.Registers[0][0x66] = 0x12; wire.Registers[0][0x67] = 0x34;
			wire.Complete(50); driver.Step(); Sample sample{};
			check("Diagnostics distinguish uncompensated range, compensated range and big-endian rate", driver.GetDiagnostics(diagnostics)
				&& diagnostics.RawRangeMm == 70 && diagnostics.RangeValMm == 50 && diagnostics.ReturnRate == 0x1234
				&& diagnostics.RangeStatus == 0 && diagnostics.TimestampMicros == TestMicros
				&& driver.GetRawData(sample) && sample.distance == 50);
			const uint16_t transactions = wire.Transactions;
			const uint32_t timestamp = sample.timestamp;
			driver.GetDiagnostics(diagnostics); driver.GetDiagnostics(diagnostics);
			check("Diagnostic getters never touch the shared bus", wire.Transactions == transactions);
			Advance(50); driver.Step(); wire.Registers[0][0x64] = 3;
			wire.Registers[0][0x66] = 0xFF; wire.Registers[0][0x67] = 0xFE;
			wire.Complete(255, 12); driver.Step();
			check("Rejected shots publish diagnostics but not a distance or filter input", driver.GetDiagnostics(diagnostics)
				&& diagnostics.RawRangeMm == 3 && diagnostics.RangeValMm == 255 && diagnostics.ReturnRate == 65534
				&& diagnostics.RangeStatus == 12 && diagnostics.TimestampMicros == TestMicros
				&& driver.GetData(sample) && sample.distance == 50 && sample.timestamp == timestamp);
			Advance(50); driver.Step(); wire.Complete(100); driver.Step();
			check("Diagnostics do not alter valid filter history", driver.GetData(sample) && sample.distance == 75);
			Advance(150);
			check("Stale measurement unavailable while timestamped diagnostics remain inspectable", !driver.GetData(sample)
				&& driver.GetDiagnostics(diagnostics) && diagnostics.RangeValMm == 100);
			driver.Stop(); diagnostics.RawRangeMm = 99;
			check("Stop withdraws diagnostics without altering output", !driver.GetDiagnostics(diagnostics) && diagnostics.RawRangeMm == 99);
			driver.Start(); check("Restart clears diagnostics", !driver.GetDiagnostics(diagnostics));
			check("Diagnostic I2C transactions all select clock and use repeated starts", wire.ClockCorrect && wire.RepeatedStartCorrect);
		}
		{
			const uint16_t registers[] = { 0x64, 0x62, 0x66, 0x67, 0x15 };
			for (uint16_t reg : registers)
			{
				FakeWire wire; Driver driver(wire); driver.Start(); driver.Step(); wire.Complete(50); driver.Step();
				Advance(50); driver.Step(); wire.Complete(255, 12); wire.FailRegister = reg; driver.Step();
				Inertia::Drivers::Distance::Vl6180x::Vl6180xDiagnostics diagnostics{}; diagnostics.RangeValMm = 42;
				Sample sample{};
				check("Partial diagnostic read or interrupt-clear failure cannot publish or leak cache", !driver.GetDiagnostics(diagnostics)
					&& diagnostics.RangeValMm == 42 && !driver.GetData(sample) && !driver.IsRunning()
					&& driver.GetLastError() == ErrorEnum::I2c);
			}
		}
		{
			using Inertia::Drivers::Distance::Vl6180x::CalculateCrosstalkCompensation;
			check("Integer crosstalk uses 9.7 units and truncates fractional results", CalculateCrosstalkCompensation(128, 100, 75) == 32
				&& CalculateCrosstalkCompensation(513, 100, 90) == 51);
			check("Crosstalk is zero for zero target, equal/longer measured range or zero rate",
				CalculateCrosstalkCompensation(65535, 0, 0) == 0
				&& CalculateCrosstalkCompensation(128, 100, 100) == 0
				&& CalculateCrosstalkCompensation(128, 100, 101) == 0
				&& CalculateCrosstalkCompensation(0, 100, 50) == 0);
			check("Maximum integer product does not overflow or wrap", CalculateCrosstalkCompensation(65535, 65535, 0) == 65535
				&& CalculateCrosstalkCompensation(65535, 65535, 1) == 65534
				&& CalculateCrosstalkCompensation(65535, 1, 0) == 65535);
		}
		{
			FakeWire wire;
			Inertia::Drivers::Distance::Vl6180x::Driver<FakeWire, 1> driver(wire);
			Inertia::Components::Distance::PeriodicCalibratedRange<1> corrected(driver, { 14000, 1100, 12 }, 150000);
			Sample out{}, raw{};
			check("Software compensation starts independently", corrected.Start());
			driver.Start(); driver.Step();
			wire.Registers[0][0x66] = 0x6B; wire.Registers[0][0x67] = 0xBA;
			wire.Complete(1); driver.Step(); corrected.Step();
			check("Correction seeds software filter while driver remains unmodified", corrected.GetData(out)
				&& out.distance == 13 && driver.GetRawData(raw) && raw.distance == 1 && raw.timestamp == out.timestamp);
			Advance(50); driver.Step(); wire.Registers[0][0x66] = 0x6F; wire.Registers[0][0x67] = 0xDB;
			wire.Complete(3); driver.Step(); corrected.Step();
			check("Correction uses unfiltered paired input before alpha-half filtering", corrected.GetData(out)
				&& out.distance == 14 && driver.GetRawData(raw) && raw.distance == 3 && out.quality == raw.quality);
			corrected.Step(); corrected.Step();
			check("Repeated source timestamps do not advance software filter", corrected.GetData(out) && out.distance == 14);
			const uint32_t timestamp = out.timestamp;
			Advance(50); driver.Step(); wire.Registers[0][0x66] = 255; wire.Registers[0][0x67] = 255;
			wire.Complete(255, 12); driver.Step(); corrected.Step();
			Inertia::Drivers::Distance::Vl6180x::Vl6180xDiagnostics diagnostics{};
			Inertia::Components::Distance::timestamped_signal_range16_t paired{};
			check("Rejected diagnostics never replace accepted range/rate pair", corrected.GetData(out)
				&& out.distance == 14 && out.timestamp == timestamp && driver.GetData(paired)
				&& paired.distance == 3 && paired.ReturnRate == 28635 && paired.timestamp == timestamp
				&& driver.GetDiagnostics(diagnostics) && diagnostics.RangeStatus == 12 && diagnostics.ReturnRate == 65535);
			Advance(100);
			out.distance = 444;
			check("Expired source invalidates compensation without changing output", !corrected.GetData(out)
				&& out.distance == 444 && !driver.GetData(paired));
			corrected.Step();
			driver.Step(); wire.Complete(3); driver.Step(); corrected.Step();
			check("Software filter reseeds after stale gap", corrected.GetData(out) && out.distance == 15);
			corrected.Stop(); corrected.Step();
			check("Stopped compensation stays unavailable", !corrected.GetData(out));
			corrected.Start(); corrected.Step();
			check("Restart resets software filter", corrected.GetData(out) && out.distance == 15);
		}
		{
			FakeWire wire; Driver driver(wire);
			check("Reserved address rejected without bus access", !driver.SetAddress(0x07) && !driver.SetAddress(0x78) && wire.Transactions == 0);
			driver.Start(); const uint16_t transactions = wire.Transactions;
			check("Running address change rejected without transaction", !driver.SetAddress(0x30)
				&& driver.GetLastError() == ErrorEnum::AddressChangeWhileRunning && driver.GetAddress() == 0x29 && wire.Transactions == transactions);
			Driver badClock(wire, 0x29, 400001), badAddress(wire, 0x80), badTimeout(wire, 0x29, 400000, 0);
			Driver overflow(wire, 0x29, 400000, 0x7FFFFFFF, 1);
			check("Invalid clock/address/timeout/overflow rejected", !badClock.Start() && !badAddress.Start()
				&& !badTimeout.Start() && !overflow.Start() && wire.Transactions == transactions);
		}
	}
}

#endif