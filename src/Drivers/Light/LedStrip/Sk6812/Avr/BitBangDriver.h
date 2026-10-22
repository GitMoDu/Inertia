#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_AVR_BIT_BANG_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_AVR_BIT_BANG_DRIVER_h

#if defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include "../Model.h"
#include "../../Transport/Avr/BitBangTransport.h"

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
					namespace Avr
					{
						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangRgbDriver = Inertia::Drivers::Light::LedStrip::Transport::Avr::BitBangTransport<
							OutputPin, LedCount,
							OutputFormat::Rgb,
							Inertia::Drivers::Light::LedStrip::Sk6812::Timing::BitBang>;

						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangRgbwDriver = Inertia::Drivers::Light::LedStrip::Transport::Avr::BitBangTransport<
							OutputPin, LedCount,
							OutputFormat::Rgbw,
							Inertia::Drivers::Light::LedStrip::Sk6812::Timing::BitBang>;
					}
				}
			}
		}
	}
}

#endif
#endif