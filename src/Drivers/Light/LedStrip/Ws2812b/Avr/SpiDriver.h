#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_AVR_SPI_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_AVR_SPI_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "../Model.h"

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
						namespace Detail
						{
							inline void SendSpiByte(const uint8_t value)
							{
								SPDR = value;
								while (!(SPSR & _BV(SPIF)))
								{
								}
								const uint8_t received = SPDR;
								(void)received;
							}

							struct Spi4MhzEncoding
							{
								static constexpr uint32_t CpuClock = 8000000UL;
								static constexpr uint8_t EncodedZero = 0b1100;
								static constexpr uint8_t EncodedOne = 0b1110;

								static void Configure()
								{
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = _BV(SPI2X);
								}

								static void SendEncodedByte(const uint8_t value)
								{
									for (uint8_t pair = 0; pair < 4; ++pair)
									{
										const uint8_t firstMask = static_cast<uint8_t>(0x80U >> (pair * 2));
										const uint8_t firstSymbol = (value & firstMask) ? EncodedOne : EncodedZero;
										const uint8_t secondSymbol = (value & (firstMask >> 1)) ? EncodedOne : EncodedZero;
										SendSpiByte(static_cast<uint8_t>((firstSymbol << 4) | secondSymbol));
									}
								}
							};

							struct Spi8MhzEncoding
							{
								static constexpr uint32_t CpuClock = 16000000UL;
								static constexpr uint8_t EncodedZero = 0b11000000;
								static constexpr uint8_t EncodedOne = 0b11111000;

								static void Configure()
								{
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = _BV(SPI2X);
								}

								static void SendEncodedByte(const uint8_t value)
								{
									for (uint8_t mask = 0x80; mask; mask >>= 1)
									{
										if (value & mask)
										{
											SendSpiByte(EncodedOne);
											asm volatile("nop\n\t" "nop\n\t");
										}
										else
										{
											SendSpiByte(EncodedZero);
										}
									}
								}
							};

							template<uint16_t LedCount, typename EncodingType>
							class SpiTransport : public Inertia::Components::Light::ILightColorDriver<Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb>
							{
								using OutputFormat = Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb;
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(sizeof(OutputFormat::color_t) == OutputFormat::ChannelCount,
									"WS2812B color values must be tightly packed for linear transmission.");
								static_assert(EncodingType::CpuClock == F_CPU,
									"The selected WS2812B AVR SPI variant requires its matching CPU clock.");

							private:
								static constexpr uint16_t ResetMicros = 80;
								OutputFormat::color_t Colors[LedCount]{};
								bool Started = false;

							public:
								~SpiTransport()
								{
									Stop();
								}

								bool Start() override
								{
									if (Started) return true;

									pinMode(SS, OUTPUT);
									digitalWrite(SS, HIGH);
									pinMode(SCK, OUTPUT);
									pinMode(MOSI, OUTPUT);
									digitalWrite(SCK, LOW);
									digitalWrite(MOSI, LOW);

									EncodingType::Configure();
									const uint8_t status = SPSR;
									const uint8_t received = SPDR;
									(void)status;
									(void)received;
									Started = true;
									return true;
								}

								void Stop() override
								{
									if (!Started) return;
									SPCR &= static_cast<uint8_t>(~_BV(SPE));
									digitalWrite(MOSI, LOW);
									digitalWrite(SCK, LOW);
									Started = false;
								}

								bool SetLightColor(const uint16_t index, const OutputFormat::color_t color) override
								{
									if (!Started || index >= LedCount) return false;
									Colors[index] = color;
									return true;
								}

								bool Push() override
								{
									if (!Started) return false;

									const uint8_t sregPrevious = SREG;
									const uint8_t* const colorBytes = reinterpret_cast<const uint8_t*>(Colors);
									cli();
									for (size_t byteIndex = 0; byteIndex < sizeof(Colors); ++byteIndex)
									{
										EncodingType::SendEncodedByte(colorBytes[byteIndex]);
									}
									SREG = sregPrevious;
									delayMicroseconds(ResetMicros);
									return true;
								}

							};
						}

						template<uint16_t LedCount>
						using Spi4MhzDriver = Detail::SpiTransport<LedCount, Detail::Spi4MhzEncoding>;

						template<uint16_t LedCount>
						using Spi8MhzDriver = Detail::SpiTransport<LedCount, Detail::Spi8MhzEncoding>;

#if F_CPU == 8000000UL
						template<uint16_t LedCount>
						using SpiDriver = Spi4MhzDriver<LedCount>;
#elif F_CPU == 16000000UL
						template<uint16_t LedCount>
						using SpiDriver = Spi8MhzDriver<LedCount>;
#else
#error "WS2812B AVR SPI supports 8 MHz and 16 MHz CPU clocks."
#endif
					}
				}
			}
		}
	}
}
#endif
#endif
