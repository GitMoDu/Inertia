#include <InertiaModel.h>
#include "Vl6180xDriverTests.h"

// Run on an Arduino board; no physical sensor is needed.
// Each failed GetData must leave output unchanged.
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
	Vl6180xTests::Run(Check);
	Serial.print(F("Passed: ")); Serial.println(Passed);
	Serial.print(F("Failed: ")); Serial.println(Failed);
}

void loop() {}