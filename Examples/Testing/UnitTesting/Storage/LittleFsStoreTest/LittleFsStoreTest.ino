#define SERIAL_BAUD_RATE 115200

#include <Arduino.h>
#include <InertiaModel.h>

#include "CircularFileStoreTest.h"
#include "StructStoreTest.h"

inline void PrintPlatform();

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial)
        ;
    delay(1000);

    Serial.println();
    Serial.println();
    Serial.println(F("LittleFS Storage Unit Testing"));
    Serial.print('\t');
    PrintPlatform();
    Serial.println();
    Serial.println();

    const bool circularPass = Inertia::Test::Storage::LittleFs::CircularFileStoreTest::RunTests();
    Serial.println();
    const bool structPass = Inertia::Test::Storage::LittleFs::StructStoreTest::RunTests();
    const bool pass = circularPass && structPass;

    Serial.println();
    Serial.println(pass
        ? F("LittleFS storage tests PASSED.")
        : F("LittleFS storage tests FAILED."));
    Serial.println();
    Serial.flush();
}

void loop()
{
}

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