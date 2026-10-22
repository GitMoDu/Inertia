#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_RASPBERRY_PI_PICO_DETAIL_PIO_PROGRAM_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_RASPBERRY_PI_PICO_DETAIL_PIO_PROGRAM_h

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
				namespace Sk6812
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

							// 10 PIO cycles per symbol: 3-cycle zero-high and 5-cycle one-high.
							template<uint8_t Pin, uint16_t LedCount>
							const uint16_t PioProgram<Pin, LedCount>::Instructions[4] =
							{
								0x6421,
								0x1223,
								0x1100,
								0xa142
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
