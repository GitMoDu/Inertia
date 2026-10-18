#include <Arduino.h>
#include <InertiaDrivers.h>

/// <summary>STM32F1 Maple-core internal flash driver exercised by this sketch.</summary>
using Flash = Inertia::Drivers::Storage::Flash::Stm32::Maple::InternalFlash;

/// <summary>Flash required by this test; the driver rounds this up to whole pages and finds a safe region.</summary>
static constexpr uint32_t TestStorageSizeBytes = Flash::PageSizeBytes;

/// <summary>Halfword written by the destructive test before the test page is erased.</summary>
static constexpr uint16_t TestPattern = 0xA55A;

/// <summary>Start time of the pending destructive-test confirmation window.</summary>
uint32_t DestructiveConfirmationStartedAtMillis = 0;

/// <summary>True after a destructive test is requested and until it is confirmed or cancelled.</summary>
bool AwaitingDestructiveConfirmation = false;

/// <summary>Automatically selected flash region awaiting confirmation.</summary>
uint32_t PendingDestructiveTestPageAddress = 0;

/// <summary>Print the elapsed microseconds and result for a flash operation.</summary>
void PrintDuration(const __FlashStringHelper* operation, const uint32_t started, const bool succeeded)
{
	Serial.print(operation);
	Serial.print(F(" duration us: "));
	Serial.print(static_cast<unsigned long>(micros() - started));
	Serial.print(F(" result: "));
	Serial.println(succeeded ? F("PASS") : F("FAIL"));
}

/// <summary>Run address-validation checks that do not modify flash contents.</summary>
void RunSafeChecks()
{
	const uint32_t base = Flash::GetFlashBaseAddress();
	const bool baseRejected = !Flash::IsStorageRegionAvailable(base, base + Flash::PageSizeBytes);
	const bool reversedRangeRejected = !Flash::IsStorageRegionAvailable(base + Flash::PageSizeBytes, base);
	const bool unalignedRangeRejected = !Flash::IsStorageRegionAvailable(base + 1, base + Flash::PageSizeBytes + 1);

	Serial.println(baseRejected ? F("PASS: firmware/base page rejected.") : F("FAIL: firmware/base page validation."));
	Serial.println(reversedRangeRejected ? F("PASS: reversed range rejected.") : F("FAIL: reversed range validation."));
	Serial.println(unalignedRangeRejected ? F("PASS: unaligned range rejected.") : F("FAIL: unaligned range validation."));
}

/// <summary>Find a safe blank test page and request serial confirmation before changing it.</summary>
void RunDestructiveTest()
{
	if (!Flash::TryGetStorageRegion(TestStorageSizeBytes, PendingDestructiveTestPageAddress))
	{
		Serial.println(F("Refused: no safe flash region is available after the program image."));
		return;
	}

	for (uint32_t offset = 0; offset < TestStorageSizeBytes; offset += sizeof(uint16_t))
	{
		if (Flash::ReadHalfWord(PendingDestructiveTestPageAddress + offset) != 0xFFFF)
		{
			Serial.println(F("Refused: test page is not blank; no data was changed."));
			return;
		}
	}

	Serial.print(F("About to program and erase the blank flash test page at 0x"));
	Serial.println(PendingDestructiveTestPageAddress, HEX);
	Serial.println(F("Send y within 30 seconds to continue."));
	AwaitingDestructiveConfirmation = true;
	DestructiveConfirmationStartedAtMillis = millis();
}

/// <summary>Run the program/readback/erase/readback sequence after a timely serial confirmation.</summary>
void ConfirmDestructiveTest()
{
	if (!AwaitingDestructiveConfirmation)
		return;
	AwaitingDestructiveConfirmation = false;
	if (millis() - DestructiveConfirmationStartedAtMillis > 30000UL)
	{
		Serial.println(F("Refused: destructive test confirmation expired."));
		return;
	}

	uint32_t testPageAddress = 0;
	if (!Flash::TryGetStorageRegion(TestStorageSizeBytes, testPageAddress)
		|| testPageAddress != PendingDestructiveTestPageAddress)
	{
		Serial.println(F("Refused: the flash test region is no longer safe."));
		return;
	}

	const uint32_t testAddress = testPageAddress + Flash::PageSizeBytes - sizeof(uint16_t);
	uint32_t started = micros();
	const bool programmed = Flash::ProgramHalfWord(testAddress, TestPattern);
	PrintDuration(F("Program halfword"), started, programmed);
	const bool readBack = Flash::ReadHalfWord(testAddress) == TestPattern;
	Serial.println(readBack ? F("PASS: programmed value read back.") : F("FAIL: programmed value readback."));

	started = micros();
	const bool erased = Flash::ErasePage(testPageAddress);
	PrintDuration(F("Erase test page"), started, erased);
	const bool erasedReadBack = Flash::ReadHalfWord(testAddress) == 0xFFFF;
	Serial.println(erasedReadBack ? F("PASS: erased value read back.") : F("FAIL: erased value readback."));
	Serial.println(programmed && readBack && erased && erasedReadBack
		? F("ALL DESTRUCTIVE TESTS PASS")
		: F("DESTRUCTIVE TEST FAILED"));
}

/// <summary>Start serial output and run the non-destructive address checks.</summary>
void setup()
{
	Serial.begin(115200);
	delay(1000);
	while (!Serial)
		;
	delay(1000);

	Serial.println(F("Maple STM32 internal flash driver tester."));
	RunSafeChecks();
	uint32_t testPageAddress = 0;
	if (Flash::TryGetStorageRegion(TestStorageSizeBytes, testPageAddress))
	{
		Serial.print(F("Automatically selected test page: 0x"));
		Serial.println(testPageAddress, HEX);
		Serial.println(F("The destructive test still requires a blank page and serial confirmation."));
	}
	else
	{
		Serial.println(F("No safe flash test page is available; destructive test requests will be refused."));
	}
	Serial.println(F("Serial commands: d requests destructive test; y confirms; n cancels."));
}

/// <summary>Read serial commands and handle the destructive-test confirmation window.</summary>
void loop()
{
	if (!Serial.available())
		return;

	const char command = static_cast<char>(Serial.read());
	if (AwaitingDestructiveConfirmation)
	{
		if (command == '\r' || command == '\n')
			return;
		if (command == 'y')
			ConfirmDestructiveTest();
		else if (command == 'n')
		{
			AwaitingDestructiveConfirmation = false;
			Serial.println(F("Destructive test cancelled."));
		}
		return;
	}
	if (command == 'd')
		RunDestructiveTest();
}