#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_DETAIL_PIO_PROGRAM_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_DETAIL_PIO_PROGRAM_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include <hardware/pio.h>

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
					namespace RaspberryPiPico
					{
						namespace Detail
						{
							template<uint8_t Pin, uint16_t LedCount>
							struct PioProgram
							{
								static const uint16_t Instructions[4];
								static const pio_program_t Value;
							};

							// Keep each symbol at 10 PIO cycles while making a zero high for 3 cycles.
							template<uint8_t Pin, uint16_t LedCount>
							const uint16_t PioProgram<Pin, LedCount>::Instructions[4] =
							{
								0x6221,
								0x1223,
								0x1300,
								0xa342
							};

							template<uint8_t Pin, uint16_t LedCount>
							const pio_program_t PioProgram<Pin, LedCount>::Value =
							{
								PioProgram<Pin, LedCount>::Instructions,
								4,
								-1
							};
						}
					}
				}
			}
		}
	}
}

#endif
#endif
