#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_AVR_BIT_BANG_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_AVR_BIT_BANG_DRIVER_h

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
				namespace Ws2812b
				{
					namespace Avr
					{
						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangDriver = Inertia::Drivers::Light::LedStrip::Transport::Avr::BitBangTransport<
							OutputPin, LedCount, OutputFormat::Grb>;
					}
				}
			}
		}
	}
}

#endif
#endif

