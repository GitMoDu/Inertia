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
							/// <summary>
							/// Uses a 4 MHz SPI clock with four-clock symbols.
							/// Significantly offspec from the SK6812 timing, but works with most strips.
							/// Real waveform is ~250ns on for zero and ~750ns on for one,
							/// with a nominal ~1000ns symbol period, which can stretch to ~1500 or ~2000ns due to SPI reload spacing.
							/// </summary>
							struct Spi4MhzEncoding
							{
								static constexpr uint8_t EncodedZero = 0b1000;
								static constexpr uint8_t EncodedOne = 0b1110;
								static constexpr uint16_t ResetMicros = 60;
								static constexpr bool UsesNonBlockingReset = false;

								static void Configure()
								{
									static_assert(F_CPU == 8000000UL || F_CPU == 16000000UL,
										"SK6812 AVR 4 MHz SPI supports 8 MHz and 16 MHz CPU clocks.");
#if F_CPU == 16000000UL
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = 0;
#else
									SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPHA);
									SPSR = _BV(SPI2X);
#endif
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

#if F_CPU == 16000000UL
							/// Uses an 8 MHz SPI clock with eight-clock symbols.
							/// Slightly offspec from the SK6812 timing, but within the tolerance of most strips.
							/// Real waveform is ~250ns on for zero and ~625ns on for one,
							/// with a nominal ~1400ns symbol period, which can stretch to ~1500ns due to SPI reload spacing.
							/// </summary>
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

									while (!(SPSR & _BV(SPIF))) {}
									const uint8_t received = SPDR;
									(void)received;
								}

							private:
							};
#endif

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

#if F_CPU == 16000000UL
							template<>
							struct SpiBufferSender<Spi8MhzEncoding>
							{
								static void Send(const uint8_t* buffer, const size_t count)
								{
									Spi8MhzEncoding::SendEncodedBuffer(buffer, count);
								}
							};
#endif

							template<>
							struct SpiBufferSender<Spi4MhzEncoding>
							{
								static void Send(const uint8_t* buffer, const size_t count)
								{
									Spi4MhzEncoding::SendEncodedBuffer(buffer, count);
								}
							};

							template<uint16_t LedCount, typename StripColorModelType, typename EncodingType>
							class SpiTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
							{
							private:
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
									"SK6812 supports RGB and RGBW formats.");
								static_assert(sizeof(typename StripColorModelType::color_t) == StripColorModelType::ChannelCount,
									"SK6812 color values must be tightly packed for linear transmission.");
								static constexpr size_t NativeByteCount = size_t(LedCount) * StripColorModelType::ChannelCount;

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

								bool Push(const uint8_t* buffer, const size_t size) override
								{
									if (!Started || !IsReady() || buffer == nullptr || size != NativeByteCount)
									{
										return false;
									}

									const uint8_t sregPrevious = SREG;
									cli();
									SpiBufferSender<EncodingType>::Send(buffer, NativeByteCount);
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
						using Spi4MhzRgbDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgb, Detail::Spi4MhzEncoding>;

						template<uint16_t LedCount>
						using Spi4MhzRgbwDriver = Detail::SpiTransport<LedCount, OutputFormat::Rgbw, Detail::Spi4MhzEncoding>;

#if F_CPU == 8000000UL
						template<uint16_t LedCount>
						using SpiRgbDriver = Spi4MhzRgbDriver<LedCount>;

						template<uint16_t LedCount>
						using SpiRgbwDriver = Spi4MhzRgbwDriver<LedCount>;
#elif F_CPU == 16000000UL
						template<uint16_t LedCount>
						using SpiRgbDriver = Spi8MhzRgbDriver<LedCount>;

						template<uint16_t LedCount>
						using SpiRgbwDriver = Spi8MhzRgbwDriver<LedCount>;
#else
#error "SK6812 AVR SPI supports 8 MHz and 16 MHz CPU clocks."
#endif
					}
				}
			}
		}
	}
}

#endif
#endif