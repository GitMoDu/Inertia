#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_AVR_SPI_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_AVR_SPI_DRIVER_h

#if defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include <SPI.h>
#include "../Model.h"

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
						namespace Detail
						{
							struct Spi2MhzEncoding
							{
								static constexpr uint8_t EncodedZero = 0b10000000;
								static constexpr uint8_t EncodedOne = 0b11000000;
								static constexpr uint16_t ResetMicros = 60;
								static constexpr bool UsesNonBlockingReset = false;

								static void Configure()
								{
									static_assert(F_CPU == 8000000UL || F_CPU == 16000000UL,
										"SK6812 AVR 2 MHz SPI supports 8 MHz and 16 MHz CPU clocks.");
#if F_CPU == 16000000UL
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA) | _BV(SPR0);
#else
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
#endif
									SPSR = _BV(SPI2X);
								}

								static void SendEncodedByte(const uint8_t value)
								{
									for (uint8_t mask = 0x80; mask; mask >>= 1)
									{
										SendEncodedBit((value & mask) != 0);
									}
								}

							private:
								static void SendEncodedBit(const bool value)
								{
									SendSpiByte(value ? EncodedOne : EncodedZero);
								}

								static void SendSpiByte(const uint8_t value)
								{
									SPDR = value;
									while (!(SPSR & _BV(SPIF)))
									{
									}
									const uint8_t received = SPDR;
									(void)received;
								}
							};

#if F_CPU == 16000000UL
							struct Spi8MhzEncoding
							{
								static constexpr uint8_t EncodedZero = 0b11000000;
								static constexpr uint8_t EncodedOne = 0b11111000;
								static constexpr uint16_t ResetMicros = 80;
								static constexpr bool UsesNonBlockingReset = true;

								static void Configure()
								{
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = _BV(SPI2X);
								}

								static void SendEncodedByte(const uint8_t value)
								{
									for (uint8_t mask = 0x80; mask; mask >>= 1)
									{
										SendEncodedBit((value & mask) != 0);
									}
								}

							private:
								static void SendEncodedBit(const bool value)
								{
									SendSpiByte(value ? EncodedOne : EncodedZero);
									if (value)
									{
										asm volatile("nop\n\t" "nop\n\t");
									}
								}

								static void SendSpiByte(const uint8_t value)
								{
									SPDR = value;
									while (!(SPSR & _BV(SPIF)))
									{
									}
									const uint8_t received = SPDR;
									(void)received;
								}
							};
#endif

							template<uint16_t LedCount, typename StripColorModelType, typename EncodingType>
							class SpiTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
							{
							private:
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
									"SK6812 supports RGB and RGBW formats.");
								static_assert(sizeof(typename StripColorModelType::color_t) == StripColorModelType::ChannelCount,
									"SK6812 color values must be tightly packed for linear transmission.");

								typename StripColorModelType::color_t Colors[LedCount]{};
								uint32_t WaitStart = 0;
								bool Busy = false;
								bool Started = false;

							public:
								bool Start() override
								{
									if (Started)
									{
										return true;
									}

									pinMode(SS, OUTPUT);
									digitalWrite(SS, HIGH);
									digitalWrite(SCK, LOW);
									digitalWrite(MOSI, LOW);
									pinMode(SCK, OUTPUT);
									pinMode(MOSI, OUTPUT);

									EncodingType::Configure();
									const uint8_t status = SPSR;
									const uint8_t received = SPDR;
									(void)status;
									(void)received;
									Busy = false;
									Started = true;
									return true;
								}

								void Stop() override
								{
									SPCR &= static_cast<uint8_t>(~_BV(SPE));
									Busy = false;
									Started = false;
								}

								bool SetLightColor(const uint16_t index, const typename StripColorModelType::color_t color) override
								{
									if (!Started || index >= LedCount)
									{
										return false;
									}

									Colors[index] = color;
									return true;
								}

								bool Push() override
								{
									if (!Started || !IsReady())
									{
										return false;
									}

									const uint8_t sregPrevious = SREG;
									const uint8_t* const colorBytes = reinterpret_cast<const uint8_t*>(Colors);
									cli();
									for (size_t byteIndex = 0; byteIndex < sizeof(Colors); ++byteIndex)
									{
										EncodingType::SendEncodedByte(colorBytes[byteIndex]);
									}
									SREG = sregPrevious;

									if (EncodingType::UsesNonBlockingReset)
									{
										Busy = true;
										WaitStart = micros();
									}
									else
									{
										delayMicroseconds(EncodingType::ResetMicros);
									}
									return true;
								}

							private:
								bool IsReady()
								{
									if (EncodingType::UsesNonBlockingReset && Busy && micros() - WaitStart > EncodingType::ResetMicros)
									{
										Busy = false;
									}
									return !Busy;
								}
							};
						}

#if F_CPU == 16000000UL
						template<uint16_t LedCount>
						using Spi8MhzRgbDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgb, Detail::Spi8MhzEncoding>;

						template<uint16_t LedCount>
						using Spi8MhzRgbwDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgbw, Detail::Spi8MhzEncoding>;
#endif

						template<uint16_t LedCount>
						using Spi2MhzRgbDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgb, Detail::Spi2MhzEncoding>;

						template<uint16_t LedCount>
						using Spi2MhzRgbwDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgbw, Detail::Spi2MhzEncoding>;
					}
				}
			}
		}
	}
}

#endif
#endif