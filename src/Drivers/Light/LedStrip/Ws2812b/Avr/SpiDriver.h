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

							/// Uses a 4 MHz SPI clock with four-clock symbols.
							/// Slightly offspec from the WS2812/WS2812B timing, but within the tolerance of most strips.
							/// Real waveform is ~250ns on for zero and ~750ns on for one,
							/// with a nominal ~1000ns symbol period, which can stretch to ~1500ns due to SPI reload spacing.
							/// </summary>
							struct Spi4MhzEncoding
							{
								static constexpr uint8_t EncodedZero = 0b1000;
								static constexpr uint8_t EncodedOne = 0b1110;

								static void Configure()
								{
									static_assert(F_CPU == 8000000UL || F_CPU == 16000000UL,
										"WS2812B AVR 4 MHz SPI supports 8 MHz and 16 MHz CPU clocks.");
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
#if F_CPU == 16000000UL
									SPSR = 0;
#else
									SPSR = _BV(SPI2X);
#endif
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

								static void SendEncodedBuffer(const uint8_t* values, const size_t count)
								{
									if (count == 0)
									{
										return;
									}

									size_t valueIndex = 0;
									uint8_t mask = 0x80;
									SPDR = EncodePair(values[valueIndex], mask);
									mask >>= 2;
									for (;;)
									{
										if (mask == 0)
										{
											if (++valueIndex == count)
											{
												break;
											}
											mask = 0x80;
										}

										const uint8_t encodedNext = EncodePair(values[valueIndex], mask);
										asm volatile
											(
												"1: in __tmp_reg__, %[status]\n\t"
												"sbrs __tmp_reg__, %[complete]\n\t"
												"rjmp 1b\n\t"
												"in __tmp_reg__, %[data]\n\t"
												"out %[data], %[next]\n\t"
												:
										: [status] "I" (_SFR_IO_ADDR(SPSR)),
											[complete] "I" (SPIF),
											[data] "I" (_SFR_IO_ADDR(SPDR)),
											[next] "r" (encodedNext)
											: "r0", "memory"
											);
										mask >>= 2;
									}

									while (!(SPSR & _BV(SPIF)))
									{
									}
									const uint8_t received = SPDR;
									(void)received;
								}

							private:
								static uint8_t EncodePair(const uint8_t value, const uint8_t mask)
								{
									const uint8_t encodedFirst = (value & mask) ? EncodedOne : EncodedZero;
									const uint8_t encodedSecond = (value & (mask >> 1)) ? EncodedOne : EncodedZero;
									return (encodedFirst << 4) | encodedSecond;
								}
							};

							/// Uses an 8 MHz SPI clock with eight-clock symbols.
							/// Within tolerances for WS2812 and WS2812B +-100ns.
							/// Real waveform is ~375ns on for zero and ~750ns on for one,
							/// with a stable ~1500ns symbol period.
							/// </summary>
							struct Spi8MhzEncoding
							{
								static constexpr uint32_t CpuClock = 16000000UL;
								static constexpr uint8_t EncodedZero = 0b11100000;
								static constexpr uint8_t EncodedOne = 0b11111100;

								template<uint32_t Clock = F_CPU>
								static void Configure()
								{
									static_assert(Clock == CpuClock,
										"WS2812B AVR 8 MHz SPI requires a 16 MHz CPU clock.");
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = _BV(SPI2X);
								}

								static void SendEncodedByte(const uint8_t value)
								{
									SendEncodedBuffer(&value, 1);
								}

								static void SendEncodedBuffer(const uint8_t* values, const size_t count)
								{
									if (count == 0)
									{
										return;
									}

									size_t valueIndex = 0;
									uint8_t mask = 0x80;
									SPDR = (values[valueIndex] & mask) ? EncodedOne : EncodedZero;
									mask >>= 1;
									for (;;)
									{
										if (mask == 0)
										{
											if (++valueIndex == count)
											{
												break;
											}
											mask = 0x80;
										}

										const uint8_t encodedNext = (values[valueIndex] & mask) ? EncodedOne : EncodedZero;
										asm volatile
											(
												"1: in __tmp_reg__, %[status]\n\t"
												"sbrs __tmp_reg__, %[complete]\n\t"
												"rjmp 1b\n\t"
												"in __tmp_reg__, %[data]\n\t"
												"out %[data], %[next]\n\t"
												:
										: [status] "I" (_SFR_IO_ADDR(SPSR)),
											[complete] "I" (SPIF),
											[data] "I" (_SFR_IO_ADDR(SPDR)),
											[next] "r" (encodedNext)
											: "r0", "memory"
											);
										mask >>= 1;
									}

									while (!(SPSR & _BV(SPIF)))
									{
									}
									const uint8_t received = SPDR;
									(void)received;
								}
							};

							template<typename EncodingType>
							struct SpiBufferSender
							{
								static void Send(const uint8_t* buffer, const size_t count)
								{
									for (size_t index = 0; index < count; ++index)
									{
										EncodingType::SendEncodedByte(buffer[index]);
									}
								}
							};

							template<>
							struct SpiBufferSender<Spi4MhzEncoding>
							{
								static void Send(const uint8_t* buffer, const size_t count)
								{
#if F_CPU == 16000000UL
									Spi4MhzEncoding::SendEncodedBuffer(buffer, count);
#else
									for (size_t index = 0; index < count; ++index)
									{
										Spi4MhzEncoding::SendEncodedByte(buffer[index]);
									}
#endif
								}
							};

							template<>
							struct SpiBufferSender<Spi8MhzEncoding>
							{
								static void Send(const uint8_t* buffer, const size_t count)
								{
									Spi8MhzEncoding::SendEncodedBuffer(buffer, count);
								}
							};

							template<uint16_t LedCount, typename EncodingType>
							class SpiTransport : public Inertia::Components::Light::ILightColorDriver<Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb>
							{
							private:
								using OutputFormat = Inertia::Drivers::Light::LedStrip::Ws2812b::OutputFormat::Grb;
								static constexpr size_t NativeByteCount = size_t(LedCount) * OutputFormat::ChannelCount;
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(sizeof(OutputFormat::color_t) == OutputFormat::ChannelCount,
									"WS2812B color values must be tightly packed for linear transmission.");
								static_assert(F_CPU == 8000000UL || F_CPU == 16000000UL,
									"WS2812B AVR SPI supports 8 MHz and 16 MHz CPU clocks.");

							private:
								static constexpr uint16_t ResetMicros = 80;
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

								bool Push(const uint8_t* buffer, const size_t size) override
								{
									if (!Started || buffer == nullptr || size != NativeByteCount) return false;

									const uint8_t sregPrevious = SREG;
									cli();
									SpiBufferSender<EncodingType>::Send(buffer, NativeByteCount);
									SREG = sregPrevious;
									delayMicroseconds(ResetMicros);
									return true;
								}

							};
						}

						template<uint16_t LedCount>
						using Spi4MhzRgbDriver = Detail::SpiTransport<LedCount, Detail::Spi4MhzEncoding>;

#if F_CPU == 16000000UL
						template<uint16_t LedCount>
						using Spi8MhzRgbDriver = Detail::SpiTransport<LedCount, Detail::Spi8MhzEncoding>;
#endif

						template<uint16_t LedCount>
						using Spi4MhzDriver = Spi4MhzRgbDriver<LedCount>;

#if F_CPU == 16000000UL
						template<uint16_t LedCount>
						using Spi8MhzDriver = Spi8MhzRgbDriver<LedCount>;
#endif

#if F_CPU == 8000000UL
						template<uint16_t LedCount>
						using SpiRgbDriver = Spi4MhzRgbDriver<LedCount>;

						template<uint16_t LedCount>
						using SpiDriver = SpiRgbDriver<LedCount>;
#elif F_CPU == 16000000UL
						template<uint16_t LedCount>
						using SpiRgbDriver = Spi8MhzRgbDriver<LedCount>;

						template<uint16_t LedCount>
						using SpiDriver = SpiRgbDriver<LedCount>;
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
