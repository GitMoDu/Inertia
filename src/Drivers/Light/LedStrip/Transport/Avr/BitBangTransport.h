#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_TRANSPORT_AVR_BIT_BANG_TRANSPORT_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_TRANSPORT_AVR_BIT_BANG_TRANSPORT_h

#if defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include "../../../../../Components/Light/Model.h"

#define INERTIA_LED_STRIP_AVR_W1_NOPS (((((F_CPU / 1000) * 350) / 1000000) - 3) > 0 ? ((((F_CPU / 1000) * 350) / 1000000) - 3) : 0)
#define INERTIA_LED_STRIP_AVR_W2_NOPS ((((((F_CPU / 1000) * 900) + 500000) / 1000000) - 6 - (((F_CPU / 1000) * 350) / 1000000 - 3)) > 0 ? (((((F_CPU / 1000) * 900) + 500000) / 1000000) - 6 - (((F_CPU / 1000) * 350) / 1000000 - 3)) : 0)
#define INERTIA_LED_STRIP_AVR_W3_NOPS (((((((F_CPU / 1000) * 1250) + 500000) / 1000000) - 10 - (((F_CPU / 1000) * 350) / 1000000 - 3) - (((((F_CPU / 1000) * 900) + 500000) / 1000000) - 6 - (((F_CPU / 1000) * 350) / 1000000 - 3))) > 0) ? (((((F_CPU / 1000) * 1250) + 500000) / 1000000) - 10 - (((F_CPU / 1000) * 350) / 1000000 - 3) - (((((F_CPU / 1000) * 900) + 500000) / 1000000) - 6 - (((F_CPU / 1000) * 350) / 1000000 - 3))) : 0)
#define INERTIA_LED_STRIP_AVR_NOP1 "nop      \n\t"
#define INERTIA_LED_STRIP_AVR_NOP2 "rjmp .+0 \n\t"
#define INERTIA_LED_STRIP_AVR_NOP4 INERTIA_LED_STRIP_AVR_NOP2 INERTIA_LED_STRIP_AVR_NOP2
#define INERTIA_LED_STRIP_AVR_NOP8 INERTIA_LED_STRIP_AVR_NOP4 INERTIA_LED_STRIP_AVR_NOP4
#define INERTIA_LED_STRIP_AVR_NOP16 INERTIA_LED_STRIP_AVR_NOP8 INERTIA_LED_STRIP_AVR_NOP8

namespace Inertia
{
	namespace Drivers
	{
		namespace Light
		{
			namespace LedStrip
			{
				namespace Transport
				{
					namespace Avr
					{
						namespace Detail
						{
							constexpr uint16_t Cycles(const uint16_t nanoseconds, const bool round)
							{
								return (((F_CPU / 1000) * nanoseconds) + (round ? 500000 : 0)) / 1000000;
							}

							constexpr uint8_t ClampedNops(const int16_t cycles)
							{
								return cycles > 0 ? cycles : 0;
							}

							struct Timing
							{
								static constexpr uint16_t ZeroPulseNs = 350;
								static constexpr uint16_t OnePulseNs = 900;
								static constexpr uint16_t TotalPeriodNs = 1250;
								static constexpr uint8_t FixedLowCycles = 3;
								static constexpr uint8_t FixedHighCycles = 6;
								static constexpr uint8_t FixedTotalCycles = 10;
								static constexpr uint16_t ZeroCycles = Cycles(ZeroPulseNs, false);
								static constexpr uint16_t OneCycles = Cycles(OnePulseNs, true);
								static constexpr uint16_t TotalCycles = Cycles(TotalPeriodNs, true);
								static constexpr int16_t W1 = ZeroCycles - FixedLowCycles;
								static constexpr int16_t W2 = OneCycles - FixedHighCycles - W1;
								static constexpr int16_t W3 = TotalCycles - FixedTotalCycles - W1 - W2;
								static constexpr uint8_t W1Nops = ClampedNops(W1);
								static constexpr uint8_t W2Nops = ClampedNops(W2);
								static constexpr uint8_t W3Nops = ClampedNops(W3);
								static constexpr uint16_t LowTimeNs = ((W1Nops + FixedLowCycles) * 1000000) / (F_CPU / 1000);
							};

							static_assert(Timing::LowTimeNs <= 550, "AVR LED strip bitbang: clock speed is too low.");

							static inline void SendArrayMask(const uint8_t* data, const uint16_t length, const uint8_t pinMask, volatile uint8_t* port)
							{
								uint8_t currentByte;
								uint8_t counter;
								uint8_t maskLow = ~pinMask & *port;
								uint8_t maskHigh = pinMask | *port;
								uint8_t sregPrevious = SREG;
								uint16_t remaining = length;

								cli();

								while (remaining--)
								{
									currentByte = *data++;

									asm volatile(
										"       ldi   %0,8  \n\t"
										"loop%=:            \n\t"
										"       st    X,%3 \n\t"
#if (INERTIA_LED_STRIP_AVR_W1_NOPS & 1)
										INERTIA_LED_STRIP_AVR_NOP1
#endif
#if (INERTIA_LED_STRIP_AVR_W1_NOPS & 2)
										INERTIA_LED_STRIP_AVR_NOP2
#endif
#if (INERTIA_LED_STRIP_AVR_W1_NOPS & 4)
										INERTIA_LED_STRIP_AVR_NOP4
#endif
#if (INERTIA_LED_STRIP_AVR_W1_NOPS & 8)
										INERTIA_LED_STRIP_AVR_NOP8
#endif
#if (INERTIA_LED_STRIP_AVR_W1_NOPS & 16)
										INERTIA_LED_STRIP_AVR_NOP16
#endif
										"       sbrs  %1,7  \n\t"
										"       st    X,%4 \n\t"
										"       lsl   %1    \n\t"
#if (INERTIA_LED_STRIP_AVR_W2_NOPS & 1)
										INERTIA_LED_STRIP_AVR_NOP1
#endif
#if (INERTIA_LED_STRIP_AVR_W2_NOPS & 2)
										INERTIA_LED_STRIP_AVR_NOP2
#endif
#if (INERTIA_LED_STRIP_AVR_W2_NOPS & 4)
										INERTIA_LED_STRIP_AVR_NOP4
#endif
#if (INERTIA_LED_STRIP_AVR_W2_NOPS & 8)
										INERTIA_LED_STRIP_AVR_NOP8
#endif
#if (INERTIA_LED_STRIP_AVR_W2_NOPS & 16)
										INERTIA_LED_STRIP_AVR_NOP16
#endif
										"       brcc skipone%= \n\t"
										"       st   X,%4      \n\t"
										"skipone%=:               "
#if (INERTIA_LED_STRIP_AVR_W3_NOPS & 1)
										INERTIA_LED_STRIP_AVR_NOP1
#endif
#if (INERTIA_LED_STRIP_AVR_W3_NOPS & 2)
										INERTIA_LED_STRIP_AVR_NOP2
#endif
#if (INERTIA_LED_STRIP_AVR_W3_NOPS & 4)
										INERTIA_LED_STRIP_AVR_NOP4
#endif
#if (INERTIA_LED_STRIP_AVR_W3_NOPS & 8)
										INERTIA_LED_STRIP_AVR_NOP8
#endif
#if (INERTIA_LED_STRIP_AVR_W3_NOPS & 16)
										INERTIA_LED_STRIP_AVR_NOP16
#endif
										"       dec   %0    \n\t"
										"       brne  loop%=\n\t"
										: "=&d" (counter)
										: "r" (currentByte), "x" (port), "r" (maskHigh), "r" (maskLow));
								}

								SREG = sregPrevious;
							}
						}

						template<uint8_t OutputPin, uint16_t LedCount, typename StripColorModelType>
						class BitBangTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
						{
							static constexpr uint8_t InvalidPin = UINT8_MAX;
							static constexpr size_t NativeByteCount = size_t(LedCount) * StripColorModelType::ChannelCount;
							static_assert(LedCount > 0, "LedCount must be greater than zero.");
							static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
								"AVR LED strip bitbang supports RGB and RGBW formats.");
							static_assert(NativeByteCount <= UINT16_MAX, "AVR LED strip bitbang frame exceeds its supported byte count.");

							volatile uint8_t* Port = nullptr;
							uint8_t PinMask = 0;
							typename StripColorModelType::color_t Colors[LedCount]{};
							bool Started = false;

						public:
							bool Start() override
							{
								static_assert(OutputPin != InvalidPin, "Pin must be a valid GPIO pin.");
								if (Started) return true;

								PinMask = digitalPinToBitMask(OutputPin);
								Port = portOutputRegister(digitalPinToPort(OutputPin));
								if (Port == nullptr || PinMask == 0) return false;
								pinMode(OutputPin, OUTPUT);
								*Port &= static_cast<uint8_t>(~PinMask);
								Started = true;
								return true;
							}

							void Stop() override
							{
								Started = false;
								if (Port != nullptr && PinMask != 0)
								{
									*Port &= static_cast<uint8_t>(~PinMask);
								}
							}

							bool SetLightColor(const uint16_t index, const typename StripColorModelType::color_t color) override
							{
								if (!Started || index >= LedCount) return false;
								Colors[index] = color;
								return true;
							}

							typename StripColorModelType::color_t* GetColorBuffer() override { return Colors; }

							const typename StripColorModelType::color_t* GetColorBuffer() const override { return Colors; }

							uint16_t GetColorCount() const override { return LedCount; }

							bool Push() override
							{
								if (!Started) return false;
								Detail::SendArrayMask(reinterpret_cast<const uint8_t*>(Colors),
									static_cast<uint16_t>(NativeByteCount), PinMask, Port);
								return true;
							}
						};
					}
				}
			}
		}
	}
}

#undef INERTIA_LED_STRIP_AVR_W1_NOPS
#undef INERTIA_LED_STRIP_AVR_W2_NOPS
#undef INERTIA_LED_STRIP_AVR_W3_NOPS
#undef INERTIA_LED_STRIP_AVR_NOP1
#undef INERTIA_LED_STRIP_AVR_NOP2
#undef INERTIA_LED_STRIP_AVR_NOP4
#undef INERTIA_LED_STRIP_AVR_NOP8
#undef INERTIA_LED_STRIP_AVR_NOP16

#endif
#endif
