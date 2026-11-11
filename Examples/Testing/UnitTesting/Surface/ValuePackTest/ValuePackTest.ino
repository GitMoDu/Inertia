#include <Arduino.h>
#include "ValuePackTests.h"

static bool PrintTestGroup(const __FlashStringHelper* name, bool (*run)())
{
	Serial.print(name);
	Serial.print(F(": "));
	Serial.flush();
	const bool pass = run();
	Serial.println(pass ? F("PASSED") : F("FAILED"));
	Serial.flush();
	return pass;
}

void setup()
{
	Serial.begin(115200);
	Serial.println();
	delay(1000);
	Serial.println(F("Running ValuePack tests..."));
	delay(200);

	bool pass = true;
	pass &= PrintTestGroup(F("Signed 8-bit full range"), ValuePackTests::RunSigned8FullRangeTests);
	pass &= PrintTestGroup(F("Asymmetric signed ranges"), ValuePackTests::RunAsymmetricSignedTests);
	pass &= PrintTestGroup(F("Negative and unsigned ranges"), ValuePackTests::RunNegativeAndUnsignedTests);
	pass &= PrintTestGroup(F("Signed packed ranges"), ValuePackTests::RunSignedPackedRangeTests);
	pass &= PrintTestGroup(F("Rounding and clamping"), ValuePackTests::RunRoundingAndClampTests);
	pass &= PrintTestGroup(F("Wide signed/unsigned boundaries"), ValuePackTests::RunWideSignedBoundaryTests);
	pass &= PrintTestGroup(F("Two's-complement limited/masked codecs"), ValuePackTests::RunTwoComplementCodecTests);
	pass &= PrintTestGroup(F("Surface masks"), ValuePackTests::RunMaskBoundaryTests);
	Serial.println(pass ? F("ValuePack tests PASSED.") : F("ValuePack tests FAILED."));
	Serial.flush();
}

void loop()
{}
