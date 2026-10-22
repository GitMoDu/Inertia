#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_STM32_BIT_BANG_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_STM32_BIT_BANG_DRIVER_h

#if defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32)
#include <Arduino.h>
#if defined(ARDUINO_ARCH_STM32F1) && !defined(STM32_CORE_VERSION)
#include <util/atomic.h>
#endif
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
						namespace Detail
						{
							static constexpr uint32_t DwtControlAddress = 0xE0001000;
							static constexpr uint32_t CycleCountAddress = 0xE0001004;
							static constexpr uint32_t DebugControlAddress = 0xE000EDFC;

							constexpr uint32_t NanosecondsToCycles(const uint32_t nanoseconds)
							{
								return static_cast<uint32_t>((uint64_t(F_CPU) * nanoseconds + 500000000ULL) / 1000000000ULL);
							}

							/// Real waveform is ~300ns on for zero and ~875ns on for one,
							/// with a nominal ~1210ns symbol period, which can stretch to ~1290ns due to instruction timing.
							/// Within the tolerances of WS2812/WS2812B timing.
							/// </summary>
							struct ProtocolTiming
							{
								static constexpr uint32_t ZeroHighNs = Timing::BitBang::ZeroPulseNs - 140;
								static constexpr uint32_t OneHighNs = Timing::BitBang::OnePulseNs - 105;
								static constexpr uint32_t BitPeriodNs = Timing::BitBang::TotalPeriodNs - 270;
								static constexpr uint32_t ZeroHighCycles = NanosecondsToCycles(ZeroHighNs);
								static constexpr uint32_t OneHighCycles = NanosecondsToCycles(OneHighNs);
								static constexpr uint32_t BitPeriodCycles = NanosecondsToCycles(BitPeriodNs);
							};

							static_assert(ProtocolTiming::ZeroHighCycles > 0, "WS2812B STM32 bitbang: F_CPU is too low.");
							static_assert(ProtocolTiming::BitPeriodNs > ProtocolTiming::OneHighNs, "WS2812B STM32 bitbang: invalid timing constants.");

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
#error "WS2812B STM32 bitbang: unsupported STM32 Arduino core."
#endif
							}

							static inline void Transmit(const uint8_t* data, const uint16_t length, const GpioWriter& output)
							{
								uint16_t remaining = length;
#if defined(STM32_CORE_VERSION)
								const uint32_t interruptState = __get_PRIMASK();
								__disable_irq();
#elif defined(ARDUINO_ARCH_STM32F1)
								const uint32_t interruptState = __get_primask();
								__set_primask(1);
#else
#error "WS2812B STM32 bitbang: unsupported STM32 interrupt API."
#endif
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
#if defined(STM32_CORE_VERSION)
								if (interruptState == 0)
									__enable_irq();
#elif defined(ARDUINO_ARCH_STM32F1)
								__set_primask(interruptState);
#endif
							}
						}

						namespace Detail
						{
							template<uint8_t OutputPin, uint16_t LedCount>
							class BitBangDriver : public Inertia::Components::Light::ILightColorDriver<OutputFormat::Grb>
							{
							private:
								static constexpr size_t NativeByteCount = size_t(LedCount) * OutputFormat::Grb::ChannelCount;
								static_assert(LedCount > 0, "LedCount must be greater than zero.");
								static_assert(NativeByteCount <= UINT16_MAX, "WS2812B STM32 bitbang frame exceeds its supported byte count.");

								GpioWriter Output;
								bool Started = false;

							public:
								bool Start() override
								{
									if (Started)
										return true;
									DwtCycleTimer::Enable();
									Output = ResolveGpio<OutputPin>();
									Output.Low();
									Started = true;
									return true;
								}

								void Stop() override
								{
									if (Started)
										Output.Low();
									Started = false;
								}

								bool Push(const uint8_t* buffer, const size_t size) override
								{
									if (!Started || buffer == nullptr || size != NativeByteCount)
										return false;
									Transmit(buffer, static_cast<uint16_t>(NativeByteCount), Output);
									delayMicroseconds(Timing::BitBang::ResetMicros);
									return true;
								}
							};
						}

						template<uint8_t OutputPin, uint16_t LedCount>
						using BitBangDriver = Detail::BitBangDriver<OutputPin, LedCount>;
					}
				}
			}
		}
	}
}

#endif
#endif
