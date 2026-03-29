#ifndef _VG6328A_TEST_H
#define _VG6328A_TEST_H

#include "VG6328ADriver.h"

/// @brief Hardware validation test task for the VG6328A BLE module.
/// Runs staged diagnostics: link check, chip identity, then full config.
/// Aborts early with clear output if link or identity checks fail.
/// @tparam BleType VG6328ADriver instantiation type.
/// @tparam TestIntervalMillis Delay between test steps.
template<typename BleType,
	uint32_t TestIntervalMillis = 1000>
class Vg6328ATest : private TS::Task
{
private:
	enum class TestStep : uint8_t
	{
		Begin = 0,
		LinkCheck,
		ChipVersion,
		ChipAddress,
		SetBleName,
		SetSppName,
		EnableBleBcast,
		QueryConn,
		ExitCmd,
		Summary,
		Done
	};

	static constexpr uint8_t QueryBufferSize = 32;

private:
	BleType& Ble;
	Print& Log;

	TestStep CurrentStep = TestStep::Begin;

	uint8_t PassCount = 0;
	uint8_t FailCount = 0;

public:
	Vg6328ATest(TS::Scheduler& scheduler, BleType& ble, Print& log)
		: TS::Task(TestIntervalMillis, TASK_FOREVER, &scheduler, false)
		, Ble(ble)
		, Log(log)
	{}

	/// @brief Start the test suite.
	void Start()
	{
		CurrentStep = TestStep::Begin;
		PassCount = 0;
		FailCount = 0;
		TS::Task::enable();
	}

	/// @brief Stop and disable the test task.
	void Stop()
	{
		TS::Task::disable();
	}

	/// @brief Check if the test suite has completed.
	bool IsDone() const
	{
		return CurrentStep == TestStep::Done;
	}

	/// @brief Get the number of passed tests.
	uint8_t GetPassCount() const { return PassCount; }

	/// @brief Get the number of failed tests.
	uint8_t GetFailCount() const { return FailCount; }

private:
	bool Callback() final
	{
		switch (CurrentStep)
		{
		case TestStep::Begin:
			Log.println(F("VG6328A"));
			CurrentStep = TestStep::LinkCheck;
			break;

			// Phase 1: Link alive?
		case TestStep::LinkCheck:
			CurrentStep = TestLinkCheck()
				? TestStep::ChipVersion
				: TestStep::Summary;
			break;

			// Phase 2: Right chip?
		case TestStep::ChipVersion:
			CurrentStep = TestChipVersion()
				? TestStep::ChipAddress
				: TestStep::Summary;
			break;

		case TestStep::ChipAddress:
			TestChipAddress();
			CurrentStep = TestStep::SetBleName;
			break;

			// Phase 3: Config sequence.
		case TestStep::SetBleName:
			TestSetBleName();
			CurrentStep = TestStep::SetSppName;
			break;

		case TestStep::SetSppName:
			TestSetSppName();
			CurrentStep = TestStep::EnableBleBcast;
			break;

		case TestStep::EnableBleBcast:
			TestEnableBleBcast();
			CurrentStep = TestStep::QueryConn;
			break;

		case TestStep::QueryConn:
			TestQueryConn();
			CurrentStep = TestStep::ExitCmd;
			break;

		case TestStep::ExitCmd:
			TestExitCmd();
			CurrentStep = TestStep::Summary;
			break;

		case TestStep::Summary:
			PrintSummary();
			CurrentStep = TestStep::Done;
			break;

		case TestStep::Done:
			Ble.enterDataMode(); // Ensure we leave command mode at the end of the test.
			TS::Task::disable();
			break;
		}

		return true;
	}

	// Phase 1: Link check.

	bool TestLinkCheck()
	{
		Log.print(F("Link..."));

		const bool ok = Ble.enterCommandMode();
		if (ok)
		{
			PassCount++;
			Log.println(F(" OK"));
		}
		else
		{
			FailCount++;
			Log.println(F(" FAIL"));
			Log.println(F("NO REPLY"));
		}

		return ok;
	}

	// Phase 2: Chip identity.

	bool TestChipVersion()
	{
		char buffer[QueryBufferSize]{};
		const bool ok = Ble.getVersionLine(buffer, QueryBufferSize);

		if (ok)
		{
			PassCount++;
			Log.print(F("v:"));
			Log.println(buffer);
		}
		else
		{
			FailCount++;
			Log.println(F("v:FAIL"));
			Log.println(F("WRONG CHIP?"));
		}

		return ok;
	}

	void TestChipAddress()
	{
		char buffer[QueryBufferSize]{};
		const bool ok = Ble.getBLEAddress(buffer, QueryBufferSize);

		if (ok)
		{
			PassCount++;
			Log.print(F("a:"));
			Log.println(buffer);
		}
		else
		{
			FailCount++;
			Log.println(F("a:FAIL"));
		}
	}

	// Phase 3: Configuration.

	void TestSetBleName()
	{
		const bool ok = Ble.setBLEName("XGV2");

		LogResult(F("Name"), ok);
	}

	void TestSetSppName()
	{
		const bool ok = Ble.setSPPName("XGV2");

		LogResult(F("SPP"), ok);
	}

	void TestEnableBleBcast()
	{
		const bool ok = Ble.enableBLEBroadcast(true);

		LogResult(F("BLE On"), ok);
	}

	void TestQueryConn()
	{
		typename BleType::ConnStatus status = BleType::ConnStatus::Unknown;
		const bool ok = Ble.getConnStatus(status);

		LogResult(F("Conn"), ok);
		if (ok)
		{
			Log.println(static_cast<uint8_t>(status), HEX);
		}
	}

	void TestExitCmd()
	{
		const bool ok = Ble.enterDataMode();

		LogResult(F("Data"), ok);
	}

	// Logging.

	void LogResult(const __FlashStringHelper* name, const bool passed)
	{
		if (passed)
		{
			PassCount++;
			Log.print(F("+ "));
		}
		else
		{
			FailCount++;
			Log.print(F("! "));
		}
		Log.println(name);
	}

	void PrintSummary()
	{
		Log.print(F("P:"));
		Log.print(PassCount);
		Log.print(F(" F:"));
		Log.println(FailCount);

		if (FailCount == 0)
		{
			Log.println(F("All OK"));
		}
		else
		{
			Log.println(F("HW ISSUES"));
		}
	}
};

#endif

