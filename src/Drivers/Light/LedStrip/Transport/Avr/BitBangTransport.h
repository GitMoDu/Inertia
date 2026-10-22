#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_TRANSPORT_AVR_BIT_BANG_TRANSPORT_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_TRANSPORT_AVR_BIT_BANG_TRANSPORT_h

#if defined(ARDUINO_ARCH_AVR)

#include <Arduino.h>
#include "../../../../../Components/Light/Model.h"

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
							template<typename TimingPolicy>
							struct Timing
							{
								static constexpr uint8_t FixedLowCycles = 3;
								static constexpr uint8_t FixedHighCycles = 6;
								// Includes the two-cycle ST, bit-path instructions, DEC, and taken BRNE.
								static constexpr uint8_t FixedTotalCycles = 11;
								static constexpr uint16_t ZeroCycles = (((F_CPU / 1000) * TimingPolicy::ZeroPulseNs) / 1000000);
								static constexpr uint16_t OneCycles = (((F_CPU / 1000) * TimingPolicy::OnePulseNs + 500000) / 1000000);
								static constexpr uint16_t TotalCycles = (((F_CPU / 1000) * TimingPolicy::TotalPeriodNs + 500000) / 1000000);
								static constexpr int16_t W1 = ZeroCycles - FixedLowCycles;
								static constexpr int16_t W2 = OneCycles - FixedHighCycles - W1;
								static constexpr int16_t W3 = TotalCycles - FixedTotalCycles - W1 - W2;
								static_assert(W1 >= 0 && W2 >= 0 && W3 >= 0,
									"AVR LED strip bitbang: F_CPU cannot produce the requested timing profile.");
								static constexpr uint8_t W1Nops = W1 > 0 ? W1 : 0;
								static constexpr uint8_t W2Nops = W2 > 0 ? W2 : 0;
								static constexpr uint8_t W3Nops = W3 > 0 ? W3 : 0;
								static constexpr uint16_t LowTimeNs = ((W1Nops + FixedLowCycles) * 1000000) / (F_CPU / 1000);
							};

							template<typename TimingPolicy>
							static inline void SendArrayMask(const uint8_t* data, const uint16_t length, const uint8_t pinMask, volatile uint8_t* port)
							{
								uint8_t currentByte;
								uint8_t counter;
								uint8_t maskLow = ~pinMask & *port;
								uint8_t maskHigh = pinMask | *port;
								uint8_t sregPrevious = SREG;
								uint16_t remaining = length;
								if (remaining == 0) return;

								cli();

								asm volatile(
									"loadbyte%=:          \n\t"
									"       ld    %[byte],Z+ \n\t"
									"       ldi   %[counter],8 \n\t"
									"bitloop%=:           \n\t"
									"       st    X,%[high] \n\t"
									".rept %c[w1]\n\t"
									"nop\n\t"
									".endr\n\t"
									"       sbrs  %[byte],7  \n\t"
									"       st    X,%[low] \n\t"
									"       lsl   %[byte]    \n\t"
									".rept %c[w2]\n\t"
									"nop\n\t"
									".endr\n\t"
									"       brcc skipone%= \n\t"
									"       st   X,%[low]      \n\t"
									"skipone%=:               \n\t"
									".rept %c[w3]\n\t"
									"nop\n\t"
									".endr\n\t"
									"       dec   %[counter] \n\t"
									"       brne  bitloop%= \n\t"
									"       sbiw  %A[remaining],1 \n\t"
									"       breq  done%= \n\t"
									"       rjmp  loadbyte%= \n\t"
									"done%=:                \n\t"
									: [counter] "=&d" (counter), [byte] "=&r" (currentByte),
									[data] "+&z" (data), [remaining] "+&w" (remaining)
									: "x" (port), [high] "r" (maskHigh), [low] "r" (maskLow),
									[w1] "M" (Timing<TimingPolicy>::W1Nops),
									[w2] "M" (Timing<TimingPolicy>::W2Nops),
									[w3] "M" (Timing<TimingPolicy>::W3Nops)
									: "cc", "memory");

								SREG = sregPrevious;
							}
						}

						template<uint8_t OutputPin, uint16_t LedCount, typename StripColorModelType, typename TimingPolicy>
						class BitBangTransport : public Inertia::Components::Light::ILightColorDriver<StripColorModelType>
						{
						private:
							static constexpr uint8_t InvalidPin = UINT8_MAX;
							static constexpr size_t NativeByteCount = size_t(LedCount) * StripColorModelType::ChannelCount;
							static_assert(LedCount > 0, "LedCount must be greater than zero.");
							static_assert(StripColorModelType::ChannelCount == 3 || StripColorModelType::ChannelCount == 4,
								"AVR LED strip bitbang supports RGB and RGBW formats.");
							static_assert(NativeByteCount <= UINT16_MAX, "AVR LED strip bitbang frame exceeds its supported byte count.");

							volatile uint8_t* Port = nullptr;
							uint8_t PinMask = 0;
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

							bool Push(const uint8_t* buffer, const size_t size) override
							{
								if (!Started || buffer == nullptr || size != NativeByteCount) return false;
								Detail::SendArrayMask<TimingPolicy>(buffer,
									static_cast<uint16_t>(NativeByteCount), PinMask, Port);
								delayMicroseconds(TimingPolicy::ResetMicros);
								return true;
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
