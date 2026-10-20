#include <Arduino.h>
#include <InertiaModel.h>
#include <InertiaDrivers.h>
#include "LightStripColorTests.h"

#if defined(ARDUINO_ARCH_AVR)
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::BitBangRgbDriver<4, 1> avrBitBangDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::BitBangRgbwDriver<5, 1> avrBitBangRgbwDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi2MhzRgbDriver<1> avrSpi2MhzDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi2MhzRgbwDriver<1> avrSpi2MhzRgbwDriver{};
#if F_CPU == 8000000UL
Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::Spi4MhzDriver<1> ws2812bExplicitSpiDriver{};
#elif F_CPU == 16000000UL
Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::Spi8MhzDriver<1> ws2812bExplicitSpiDriver{};
#endif
Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::SpiDriver<1> ws2812bSpiDriver{};
Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::BitBangDriver<6, 1> ws2812bBitBangDriver{};
#if F_CPU == 16000000UL
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi8MhzRgbDriver<1> avrSpi8MhzDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi8MhzRgbwDriver<1> avrSpi8MhzRgbwDriver{};
#endif
#elif defined(ARDUINO_ARCH_RP2040)
Inertia::Drivers::Light::LedStrip::Ws2812b::RaspberryPiPico::PioDriver<2, 1> rp2040Ws2812bPioDriver{};
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32)
Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::BitBangRgbDriver<5, 1> stm32Sk6812BitBangRgbDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::BitBangRgbwDriver<6, 1> stm32Sk6812BitBangRgbwDriver{};
#if defined(ARDUINO_ARCH_STM32F1)
Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::SpiDmaRgbDriver<1> stm32Sk6812DmaRgbDriver{};
Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::SpiDmaRgbwDriver<1> stm32Sk6812DmaRgbwDriver{};
#endif
#endif

void setup()
{
	Serial.begin(115200);
	bool pass = LightStripColorTests::RunColorPreparationTests();

#if defined(ARDUINO_ARCH_AVR)
#if F_CPU == 16000000UL
	pass = LightStripColorTests::RunAvrSk6812Spi8MhzTests(avrSpi8MhzDriver, avrSpi8MhzRgbwDriver) && pass;
#endif
	pass = LightStripColorTests::RunAvrSk6812DriverTests(
		avrBitBangDriver, avrBitBangRgbwDriver, avrSpi2MhzDriver, avrSpi2MhzRgbwDriver) && pass;
	pass = LightStripColorTests::RunAvrWs2812bDriverTests(
		ws2812bExplicitSpiDriver, ws2812bSpiDriver, ws2812bBitBangDriver) && pass;
#elif defined(ARDUINO_ARCH_RP2040)
	pass = LightStripColorTests::RunRp2040Ws2812bDriverTests(rp2040Ws2812bPioDriver) && pass;
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32)
	pass = LightStripColorTests::RunStm32Sk6812BitBangDriverTests(
		stm32Sk6812BitBangRgbDriver, stm32Sk6812BitBangRgbwDriver) && pass;
#if defined(ARDUINO_ARCH_STM32F1)
	pass = LightStripColorTests::RunStm32Sk6812DmaDriverTests(
		stm32Sk6812DmaRgbDriver, stm32Sk6812DmaRgbwDriver) && pass;
#endif
#endif

	Serial.println(pass ? F("Light strip color tests PASSED.") : F("Light strip color tests FAILED."));
}

void loop()
{}