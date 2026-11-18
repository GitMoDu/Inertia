#include <InertiaModel.h>
#include "CalibratedRangeTests.h"

static uint16_t Passed = 0;
static uint16_t Failed = 0;

void Check(const char* name, bool passed)
{
	Serial.print(passed ? F("PASS: ") : F("FAIL: "));
	Serial.println(name);
	if (passed) ++Passed; else ++Failed;
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	CalibratedRangeTests::Run(Check);
	Serial.print(F("Passed: ")); Serial.println(Passed);
	Serial.print(F("Failed: ")); Serial.println(Failed);
}

void loop() {}
