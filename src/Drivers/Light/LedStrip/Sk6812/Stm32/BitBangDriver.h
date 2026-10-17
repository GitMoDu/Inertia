#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_STM32_BIT_BANG_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_STM32_BIT_BANG_DRIVER_h

#if defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32)
#include <Arduino.h>
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
							static constexpr uint32_t DwtControlAddress = 0xE0001000;
							static constexpr uint32_t CycleCountAddress = 0xE0001004;
							static constexpr uint32_t DebugControlAddress = 0xE000EDFC;

							constexpr uint32_t NanosecondsToCycles(const uint32_t nanoseconds)
							{
								return static_cast<uint32_t>((uint64_t(F_CPU) * nanoseconds + 500000000ULL) / 1000000000ULL);
							}

							struct ProtocolTiming
							{
								static constexpr uint32_t ZeroHighNs = 350;
								static constexpr uint32_t OneHighNs = 900;
								static constexpr uint32_t BitPeriodNs = 1250;
								static constexpr uint32_t ZeroHighCycles = NanosecondsToCycles(ZeroHighNs);
								static constexpr uint32_t OneHighCycles = NanosecondsToCycles(OneHighNs);
								static constexpr uint32_t BitPeriodCycles = NanosecondsToCycles(BitPeriodNs);
							};

							static_assert(ProtocolTiming::ZeroHighCycles > 0, "SK6812 STM32 bitbang: F_CPU is too low.");
							static_assert(ProtocolTiming::BitPeriodNs > ProtocolTiming::OneHighNs, "SK6812 STM32 bitbang: invalid timing constants.");

							struct DwtCycleTimer
							{
								static volatile uint32_t& DwtControl() { return *reinterpret_cast<volatile uint32_t*>(DwtControlAddress); }
								static volatile uint32_t& CycleCount() { return *reinterpret_cast<volatile uint32_t*>(CycleCountAddress); }
								static volatile uint32_t& DebugControl() { return *reinterpret_cast<volatile uint32_t*>(DebugControlAddress); }

								static void Enable()
								{
									DebugControl() |= (1UL << 24);
									CycleCount() = 0;
									DwtControl() |= 1UL;
								}

								static uint32_t Read() { return CycleCount(); }

								static void WaitFor(const uint32_t target)
								{
									while (int32_t(Read() - target) < 0) {}
								}
							};

							struct GpioWriter
							{
								volatile uint32_t* SetResetRegister;
								uint32_t SetValue;
								uint32_t ResetValue;

								GpioWriter() : SetResetRegister(nullptr), SetValue(0), ResetValue(0) {}
								GpioWriter(volatile uint32_t* reg, const uint32_t set, const uint32_t reset)
									: SetResetRegister(reg), SetValue(set), ResetValue(reset) {}

								void High() const { *SetResetRegister = SetValue; }
								void Low() const { *SetResetRegister = ResetValue; }
							};

							template<uint8_t Pin>
							GpioWriter ResolveGpio()
							{
								pinMode(Pin, OUTPUT);
#if defined(STM32_CORE_VERSION)
								GPIO_TypeDef* port = digitalPinToPort(Pin);
								const uint32_t mask = digitalPinToBitMask(Pin);
								return GpioWriter(&port->BSRR, mask, mask << 16);
#elif defined(ARDUINO_ARCH_STM32F1)
								gpio_dev* port = PIN_MAP[Pin].gpio_device;
								const uint32_t mask = 1UL << PIN_MAP[Pin].gpio_bit;
								return GpioWriter(&port->regs->BSRR, mask, mask << 16);
#else
#error "SK6812 STM32 bitbang: unsupported STM32 Arduino core."
#endif
							}

							static inline void Transmit(const uint8_t* data, const uint16_t length, const GpioWriter& output)
							{
								uint16_t remaining = length;
								const uint32_t interruptState = __get_PRIMASK();
								noInterrupts();
								while (remaining--)
								{
									const uint8_t value = *data++;
									for (uint8_t mask = 0x80; mask; mask >>= 1)
									{
										const uint32_t start = DwtCycleTimer::Read();
										const uint32_t highCycles = (value & mask) ? ProtocolTiming::OneHighCycles : ProtocolTiming::ZeroHighCycles;
										output.High();
										DwtCycleTimer::WaitFor(start + highCycles);
										output.Low();
										DwtCycleTimer::WaitFor(start + ProtocolTiming::BitPeriodCycles);
									}
								}
								if (interruptState == 0)
								{
									interrupts();
								}
							}
						}

						namespace Detail
						{
							template<uint8_t OutputPin, uint16_t LedCount, typename StripColorModelType>
							class BitBangTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
							{
								static constexpr size_t NativeByteCount = size_t(LedCount) * StripColorModelType::ChannelCount;
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
									"SK6812 supports RGB and RGBW formats.");
								static_assert(NativeByteCount <= UINT16_MAX, "SK6812 STM32 bitbang frame exceeds its supported byte count.");

								Detail::GpioWriter Output;
								typename StripColorModelType::color_t Colors[LedCount]{};
								bool Started = false;

							public:
								bool Start() override
								{
									if (Started)
									{
										return true;
									}
									DwtCycleTimer::Enable();
									Output = ResolveGpio<OutputPin>();
									Output.Low();
									Started = true;
									return true;
								}

								void Stop() override
								{
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
									if (!Started)
									{
										return false;
									}
									Transmit(reinterpret_cast<const uint8_t*>(Colors), static_cast<uint16_t>(NativeByteCount), Output);
									return true;
								}
							};
						}

						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangRgbDriver = Detail::BitBangTransport<OutputPin, LedCount, OutputFormat::Rgb>;

						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangRgbwDriver = Detail::BitBangTransport<OutputPin, LedCount, OutputFormat::Rgbw>;
					}
				}
			}
		}
	}
}

#endif
#endif
