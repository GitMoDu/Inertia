#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_STM32_DMA_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_STM32_DMA_DRIVER_h

#if defined(ARDUINO_ARCH_STM32F1)
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
				namespace Ws2812b
				{
					namespace Stm32
					{
						template<uint16_t LedCount>
						class SpiDmaDriver : public Inertia::Components::Light::ILightColorDriver<OutputFormat::Grb>
						{
						private:
							/// Uses a 9 MHz SPI clock with 10-clock symbols.
							/// Within the tolerances of WS2812/WS2812B timing.
							/// Real waveform is ~350ns on for zero and ~900ns on for one,
							/// with a nominal ~1110ns symbol period.
							/// </summary>
							struct SpiEncoding
							{
								static constexpr uint8_t BitsPerNativeBit = 10;
								static constexpr uint16_t EncodedZero = 0b1110000000;
								static constexpr uint16_t EncodedOne = 0b1111111100;
								static constexpr uint16_t LeadingPaddingBytes = 1;
								static constexpr uint16_t ResetPaddingBytes = 90;
								static constexpr uint32_t SpiBitRate = 9000000UL;
							};

							static constexpr size_t NativeByteCount = size_t(LedCount) * OutputFormat::Grb::ChannelCount;
							static constexpr size_t EncodedByteCount = NativeByteCount * SpiEncoding::BitsPerNativeBit
								+ SpiEncoding::LeadingPaddingBytes + SpiEncoding::ResetPaddingBytes;
							static_assert(LedCount > 0, "LedCount must be greater than zero.");
							static_assert(EncodedByteCount <= UINT16_MAX, "WS2812B STM32 DMA frame exceeds its supported byte count.");
							static_assert(sizeof(typename OutputFormat::Grb::color_t) == OutputFormat::Grb::ChannelCount,
								"WS2812B color values must be tightly packed for linear encoding.");
							static_assert(SpiEncoding::ResetPaddingBytes * 8 * 1000000ULL
								>= SpiEncoding::SpiBitRate * Timing::BitBang::ResetMicros,
								"WS2812B SPI-DMA reset padding is too short.");

							uint8_t EncodedBuffer[EncodedByteCount]{};
							volatile bool TransferActive = false;
							SPIClass& Spi;

						public:
							explicit SpiDmaDriver(SPIClass& spi = SPI)
								: Spi(spi)
							{}

							bool Start() override
							{
								Spi.setClockDivider(ClockDivider(Spi));
								Spi.begin();
								TransferActive = false;
								return true;
							}

							bool IsReady() const override
							{
								return !TransferActive;
							}

							void Stop() override { TransferActive = false; }

							void HandleTransmitComplete() { TransferActive = false; }

							bool Push(const uint8_t* buffer, const size_t size) override
							{
								if (TransferActive || buffer == nullptr || size != NativeByteCount) return false;

								uint8_t* encoded = EncodedBuffer;
								*encoded++ = 0;
								for (size_t byteIndex = 0; byteIndex < NativeByteCount; ++byteIndex)
								{
									EncodeByte(encoded, buffer[byteIndex]);
								}

								TransferActive = true;
								if (Spi.dmaSendAsync(EncodedBuffer, EncodedByteCount) != 0)
								{
									TransferActive = false;
									return false;
								}
								return true;
							}

						private:
							static uint32_t PeripheralClock(SPIClass& spi)
							{
								switch (rcc_dev_clk(spi.c_dev()->clk_id))
								{
								case RCC_APB2: return STM32_PCLK2;
								case RCC_APB1: return STM32_PCLK1;
								default: return STM32_PCLK2;
								}
							}

							static uint32_t ClockDivider(SPIClass& spi)
							{
								uint32_t clock = PeripheralClock(spi) / 2;
								uint8_t index = 0;
								while (index < 7 && SpiEncoding::SpiBitRate < clock)
								{
									clock /= 2;
									++index;
								}
								return index << 3;
							}

							static void EncodeByte(uint8_t*& output, const uint8_t value)
							{
								uint32_t encoded = 0;
								uint8_t bufferedBits = 0;
								for (uint8_t mask = 0x80; mask; mask >>= 1)
								{
									encoded <<= SpiEncoding::BitsPerNativeBit;
									encoded |= (value & mask) ? SpiEncoding::EncodedOne : SpiEncoding::EncodedZero;
									bufferedBits += SpiEncoding::BitsPerNativeBit;
									while (bufferedBits >= 8)
									{
										bufferedBits -= 8;
										*output++ = uint8_t(encoded >> bufferedBits);
										encoded &= (1UL << bufferedBits) - 1UL;
									}
								}
							}
						};
					}
				}
			}
		}
	}
}

#endif
#endif
