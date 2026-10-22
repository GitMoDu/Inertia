#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_RASPBERRY_PI_PICO_PIO_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_RASPBERRY_PI_PICO_PIO_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../Model.h"
#include "../../Transport/RaspberryPiPico/PioTransport.h"
#include "Detail/PioProgram.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Light
		{
			namespace LedStrip
			{
				namespace Sk6812
				{
					namespace RaspberryPiPico
					{
						/// Pico 1 scope capture: ~375ns zero-high, ~650ns one-high, and ~1250ns per symbol.
						template<uint8_t Pin, uint16_t LedCount>
						using PioRgbDriver = Inertia::Drivers::Light::LedStrip::Transport::RaspberryPiPico::PioTransport<
							Pin, LedCount, OutputFormat::Rgb, Timing::BitBang, Detail::PioProgram>;

						template<uint8_t Pin, uint16_t LedCount>
						using PioRgbwDriver = Inertia::Drivers::Light::LedStrip::Transport::RaspberryPiPico::PioTransport<
							Pin, LedCount, OutputFormat::Rgbw, Timing::BitBang, Detail::PioProgram>;
					}
				}
			}
		}
	}
}

#endif
#endif
