#include <Arduino.h>
#include <InertiaModel.h>

// Select exactly one driver. The active default targets Arduino AVR.
#define STRIP_DRIVER_AVR_WS2812B_BITBANG
//#define STRIP_DRIVER_AVR_WS2812B_SPI // Selects 4 or 8 MHz SPI based on F_CPU.
// #define STRIP_DRIVER_AVR_SK6812_RGB_BITBANG
// #define STRIP_DRIVER_AVR_SK6812_RGBW_BITBANG
// #define STRIP_DRIVER_AVR_SK6812_RGB_SPI_2MHZ
// #define STRIP_DRIVER_AVR_SK6812_RGBW_SPI_2MHZ
// #define STRIP_DRIVER_AVR_SK6812_RGB_SPI_8MHZ // ATmega at 16 MHz only.
// #define STRIP_DRIVER_AVR_SK6812_RGBW_SPI_8MHZ // ATmega at 16 MHz only.
// #define STRIP_DRIVER_STM32_SK6812_RGB_BITBANG
// #define STRIP_DRIVER_STM32_SK6812_RGBW_BITBANG
// #define STRIP_DRIVER_STM32F1_SK6812_RGB_SPI_DMA // Requires a core with SPIClass::dmaSendAsync.
// #define STRIP_DRIVER_STM32F1_SK6812_RGBW_SPI_DMA // Requires a core with SPIClass::dmaSendAsync.
// #define STRIP_DRIVER_RP2040_WS2812B_PIO

#if (defined(STRIP_DRIVER_AVR_WS2812B_BITBANG) \
	+ defined(STRIP_DRIVER_AVR_WS2812B_SPI) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGB_BITBANG) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGBW_BITBANG) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_2MHZ) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_2MHZ) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_8MHZ) \
	+ defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_8MHZ) \
	+ defined(STRIP_DRIVER_STM32_SK6812_RGB_BITBANG) \
	+ defined(STRIP_DRIVER_STM32_SK6812_RGBW_BITBANG) \
	+ defined(STRIP_DRIVER_STM32F1_SK6812_RGB_SPI_DMA) \
	+ defined(STRIP_DRIVER_STM32F1_SK6812_RGBW_SPI_DMA) \
	+ defined(STRIP_DRIVER_RP2040_WS2812B_PIO)) != 1
#error "Select exactly one LED strip driver above."
#endif

#if (defined(STRIP_DRIVER_AVR_WS2812B_BITBANG) \
	|| defined(STRIP_DRIVER_AVR_WS2812B_SPI) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGB_BITBANG) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGBW_BITBANG) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_2MHZ) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_2MHZ) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_8MHZ) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_8MHZ)) && !defined(ARDUINO_ARCH_AVR)
#error "The selected LED strip driver requires an AVR board."
#endif

#if (defined(STRIP_DRIVER_STM32_SK6812_RGB_BITBANG) \
	|| defined(STRIP_DRIVER_STM32_SK6812_RGBW_BITBANG)) \
	&& !defined(ARDUINO_ARCH_STM32F1) && !defined(ARDUINO_ARCH_STM32)
#error "The selected SK6812 bitbang driver requires an STM32 board."
#endif

#if (defined(STRIP_DRIVER_STM32F1_SK6812_RGB_SPI_DMA) \
	|| defined(STRIP_DRIVER_STM32F1_SK6812_RGBW_SPI_DMA)) \
	&& !defined(ARDUINO_ARCH_STM32F1)
#error "The selected SK6812 DMA driver requires an STM32F1 board."
#endif

#if defined(STRIP_DRIVER_RP2040_WS2812B_PIO) && !defined(ARDUINO_ARCH_RP2040)
#error "The selected WS2812B PIO driver requires an RP2040 board."
#endif

#if (defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_8MHZ) \
	|| defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_8MHZ)) && F_CPU != 16000000UL
#error "The SK6812 AVR 8 MHz SPI drivers require a 16 MHz CPU clock."
#endif

#if defined(STRIP_DRIVER_AVR_WS2812B_SPI) && F_CPU != 8000000UL && F_CPU != 16000000UL
#error "The WS2812B AVR SPI driver requires an 8 MHz or 16 MHz CPU clock."
#endif

#include <InertiaDrivers.h>
#include "Tests.h"

namespace
{
	using SourceColorModel = Inertia::Components::Light::ColorModel::Rgb888;
	static constexpr uint16_t LedCount = 15;

#if defined(STRIP_DRIVER_AVR_WS2812B_BITBANG)
	static constexpr uint8_t OutputPin = 11;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::BitBangDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_WS2812B_SPI)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Ws2812b::Avr::SpiDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGB_BITBANG)
	static constexpr uint8_t OutputPin = 11;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::BitBangRgbDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGBW_BITBANG)
	static constexpr uint8_t OutputPin = 11;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgbw;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::BitBangRgbwDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_2MHZ)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi2MhzRgbDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_2MHZ)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgbw;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi2MhzRgbwDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGB_SPI_8MHZ)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi8MhzRgbDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_AVR_SK6812_RGBW_SPI_8MHZ)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgbw;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Avr::Spi8MhzRgbwDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_STM32_SK6812_RGB_BITBANG)
	static constexpr uint8_t OutputPin = 5;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::BitBangRgbDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_STM32_SK6812_RGBW_BITBANG)
	static constexpr uint8_t OutputPin = 5;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgbw;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::BitBangRgbwDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_STM32F1_SK6812_RGB_SPI_DMA)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::SpiDmaRgbDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_STM32F1_SK6812_RGBW_SPI_DMA)
	using StripOutput = Inertia::Drivers::Light::LedStrip::Sk6812::OutputFormat::Rgbw;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Sk6812::Stm32::SpiDmaRgbwDriver<LedCount>;
	StripDriver Strip{};
#elif defined(STRIP_DRIVER_RP2040_WS2812B_PIO)
	static constexpr uint8_t OutputPin = 2;
	using StripOutput = Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb;
	using StripDriver = Inertia::Drivers::Light::LedStrip::Ws2812b::RaspberryPiPico::PioDriver<OutputPin, LedCount>;
	StripDriver Strip{};
#else
#error "The selected LED strip driver has no matching implementation."
#endif

	using Tests = LightStripTester::Tests<SourceColorModel, StripOutput, StripDriver, LedCount>;
}

void setup()
{
	Serial.begin(115200);
	if (!Strip.Start())
	{
		Serial.println(F("Strip driver failed to start."));
		return;
	}

	Serial.println(F("Light strip driver tester started."));
	Tests::PushTestFrame(Strip, 0);
}

void loop()
{
	static uint8_t phase = 0;
	static uint32_t lastUpdate = 0;
	const uint32_t now = millis();
	if (now - lastUpdate >= 1000)
	{
		lastUpdate = now;
		Tests::PushTestFrame(Strip, phase++);
	}
}
