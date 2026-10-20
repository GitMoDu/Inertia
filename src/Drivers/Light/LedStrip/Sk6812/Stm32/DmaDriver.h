#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_STM32_DMA_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_STM32_DMA_DRIVER_h

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
				namespace Sk6812
				{
					namespace Stm32
					{
						namespace Detail
						{
							template<uint16_t LedCount, typename StripColorModelType>
							class SpiDmaTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
							{
							private:
								struct SpiEncoding
								{
									static constexpr uint8_t BitsPerNativeBit = 3;
									static constexpr uint8_t EncodedZero = 0b100;
									static constexpr uint8_t EncodedOne = 0b110;
									static constexpr uint16_t ResetPaddingBytes = 1;
									static constexpr uint32_t SpiBitRate = 2250000;
								};

								static constexpr size_t NativeByteCount = size_t(LedCount) * StripColorModelType::ChannelCount;
								static constexpr size_t EncodedByteCount = (NativeByteCount * SpiEncoding::BitsPerNativeBit) + (SpiEncoding::ResetPaddingBytes * 2);
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
									"SK6812 supports RGB and RGBW formats.");
								static_assert(sizeof(typename StripColorModelType::color_t) == StripColorModelType::ChannelCount,
									"SK6812 color values must be tightly packed for linear encoding.");

								uint8_t EncodedBuffer[EncodedByteCount]{};
								volatile bool TransferActive = false;
								SPIClass& Spi;

							public:
								explicit SpiDmaTransport(SPIClass& spi = SPI)
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
									*encoded = 0;

									TransferActive = true;
									Spi.dmaSendAsync(EncodedBuffer, EncodedByteCount);
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
									for (uint8_t mask = 0x80; mask; mask >>= 1)
									{
										encoded <<= SpiEncoding::BitsPerNativeBit;
										encoded |= (value & mask) ? SpiEncoding::EncodedOne : SpiEncoding::EncodedZero;
									}
									*output++ = uint8_t(encoded >> 16);
									*output++ = uint8_t(encoded >> 8);
									*output++ = uint8_t(encoded);
								}
							};
						}

						template<uint16_t LedCount>
						using SpiDmaDriver = Detail::SpiDmaTransport<LedCount, OutputFormat::Rgb>;

						template<uint16_t LedCount>
						using SpiDmaRgbDriver = Detail::SpiDmaTransport<LedCount, OutputFormat::Rgb>;

						template<uint16_t LedCount>
						using SpiDmaRgbwDriver = Detail::SpiDmaTransport<LedCount, OutputFormat::Rgbw>;
					}
				}
			}
		}
	}
}

#endif
#endif
