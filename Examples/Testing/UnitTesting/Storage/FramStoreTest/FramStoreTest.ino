#define SERIAL_BAUD_RATE 115200

#include <Arduino.h>
#include <InertiaModel.h>

#include "CircularFileStoreTest.h"
#include "StructStoreTest.h"

namespace I2c1
{
	enum class Pin : uint8_t
	{
		Clock = 9,
		Data = 8
	};

	using WireType = TwoWire;
	static WireType& I2cInterface = Wire;

	inline bool SetupHardwareInterface()
	{
#if defined(ARDUINO_ARCH_RP2040)
		I2cInterface.end(); // Ensure I2C is not already running before configuring pins.
		const bool setupOk = I2cInterface.setSDA((pin_size_t)Pin::Data)
			&& I2cInterface.setSCL((pin_size_t)Pin::Clock);

		if (setupOk)
		{
			I2cInterface.begin();
		}

		return setupOk;
#else
		return false;
#endif
	}
}


Inertia::Components::Storage::Fram::Drivers::Mb85Rc::Mb85Rc256v FramDriver(I2c1::I2cInterface);

inline void PrintPlatform();

void setup()
{
	Serial.begin(SERIAL_BAUD_RATE);
	while (!Serial)
		;
	delay(1000);
	Wire.begin();

	Serial.println();
	Serial.println();
	Serial.println(F("FRAM Storage Unit Testing"));
	Serial.print('\t');
	PrintPlatform();
	Serial.println();
	Serial.println();

	I2c1::SetupHardwareInterface();

	const bool framStarted = FramDriver.Start();
	if (!framStarted)
	{
		Serial.println(F("FRAM driver start FAILED."));
		Serial.println();
		Serial.flush();
		return;
	}

	const bool circularPass = Inertia::Test::Storage::Fram::CircularStoreTest::RunTests(FramDriver);
	Serial.println();
	const bool structPass = Inertia::Test::Storage::Fram::StructStoreTest::RunTests(FramDriver);
	const bool pass = circularPass && structPass;
	FramDriver.Stop();

	Serial.println();
	Serial.println(pass
		? F("FRAM storage tests PASSED.")
		: F("FRAM storage tests FAILED."));
	Serial.println();
	Serial.flush();
}

void loop()
{}

void PrintPlatform()
{
#if defined(ARDUINO_ARCH_AVR)
	Serial.println(F("AVR"));
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32)
	Serial.println(F("STM32 F1"));
#elif defined(ARDUINO_ARCH_STM32F4)
	Serial.println(F("STM32 F4"));
#elif defined(ARDUINO_ARCH_RP2040)
#if defined(PICO_RP2350)
	Serial.println(F("RP2350"));
#else
	Serial.println(F("RP2040"));
#endif
#elif defined(ARDUINO_ARCH_NRF52)
	Serial.println(F("NRF52840"));
#elif defined(ARDUINO_ARCH_ESP32)
	Serial.println(F("ESP32"));
#elif defined(ARDUINO_ARCH_ESP8266)
	Serial.println(F("ESP8266"));
#else
	Serial.println(F("Unknown Platform"));
#endif
}