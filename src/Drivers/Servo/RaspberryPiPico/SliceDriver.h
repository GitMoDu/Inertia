#ifndef _INERTIA_DRIVERS_SERVO_RASPBERRY_PI_PICO_SLICE_DRIVER_h
#define _INERTIA_DRIVERS_SERVO_RASPBERRY_PI_PICO_SLICE_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../../../Components/PowerTrain/Servo/Model.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Servo
		{
			namespace RaspberryPiPico
			{
				static constexpr uint8_t InvalidPin = UINT8_MAX;

				template<uint8_t FirstPin
					, uint8_t SecondPin = InvalidPin
					, uint32_t PwmUpdatePeriodNanos = 20000000 // 20ms (50Hz) typical servo period.
				>
				class SliceDriver : public Inertia::Components::Lifecycle::ILifecycleDriver
				{
				public:
					static constexpr uint8_t OutputCount = (SecondPin == InvalidPin) ? 1 : 2;

				private:
					static constexpr uint32_t NanosPerSecond = 1000000000;

				private:
					uint32_t PulseWidthNanos[OutputCount] = {};
					bool Enabled[OutputCount] = {};
					bool Started = false;

					uint SliceNumber = 0;
					uint ChannelNumbers[OutputCount] = {};
					uint32_t CounterCyclesPerPeriod = 0;

				public:
					SliceDriver() : Inertia::Components::Lifecycle::ILifecycleDriver()
					{}

					~SliceDriver()
					{
						Stop();
					}

					bool Start() override
					{
						if constexpr (FirstPin == InvalidPin)
						{
							return false;
						}

						const uint32_t systemClockHz = clock_get_hz(clk_sys);
						if (systemClockHz == 0)
						{
							return false;
						}

						const uint64_t clockCyclesPerPeriod = (static_cast<uint64_t>(systemClockHz) * PwmUpdatePeriodNanos) / NanosPerSecond;
						if (clockCyclesPerPeriod == 0 || clockCyclesPerPeriod > static_cast<uint64_t>(UINT16_MAX) * (255 * 16 + 15) / 16)
						{
							return false;
						}

						const uint32_t divider16 = static_cast<uint32_t>((clockCyclesPerPeriod * 16ULL + UINT16_MAX - 1ULL) / UINT16_MAX);
						if (divider16 < 16 || divider16 >(255 * 16 + 15))
						{
							return false;
						}

						CounterCyclesPerPeriod = static_cast<uint32_t>((clockCyclesPerPeriod * 16ULL) / divider16);
						if (CounterCyclesPerPeriod == 0 || CounterCyclesPerPeriod > UINT16_MAX)
						{
							return false;
						}

						SliceNumber = pwm_gpio_to_slice_num(FirstPin);
						ChannelNumbers[0] = pwm_gpio_to_channel(FirstPin);
						gpio_set_function(FirstPin, GPIO_FUNC_PWM);

						if constexpr (SecondPin != InvalidPin)
						{
							if (pwm_gpio_to_slice_num(SecondPin) != SliceNumber)
							{
								return false;
							}

							ChannelNumbers[1] = pwm_gpio_to_channel(SecondPin);
							gpio_set_function(SecondPin, GPIO_FUNC_PWM);
						}

						pwm_config config = pwm_get_default_config();
						pwm_config_set_clkdiv_int_frac(&config,
							static_cast<uint8_t>(divider16 / 16),
							static_cast<uint8_t>(divider16 % 16));
						pwm_config_set_wrap(&config, static_cast<uint16_t>(CounterCyclesPerPeriod - 1));
						pwm_init(SliceNumber, &config, true);

						Started = true;

						for (uint8_t i = 0; i < OutputCount; i++)
						{
							ApplyOutputLevel(i);
						}

						return true;
					}

					void Stop() override
					{
						if (Started)
						{
							for (uint8_t i = 0; i < OutputCount; i++)
							{
								pwm_set_chan_level(SliceNumber, static_cast<pwm_chan>(ChannelNumbers[i]), 0);
							}
							pwm_set_enabled(SliceNumber, false);
						}

						CounterCyclesPerPeriod = 0;
						Started = false;
					}

					bool SetPulseWidth(const uint8_t index, const uint32_t pulseWidthNanos)
					{
						if (index >= OutputCount)
						{
							return false;
						}

						PulseWidthNanos[index] = pulseWidthNanos;

						if (Started && Enabled[index])
						{
							ApplyOutputLevel(index);
						}

						return true;
					}

					bool EnableOutput(const uint8_t index)
					{
						if (index >= OutputCount)
						{
							return false;
						}

						Enabled[index] = true;

						if (Started)
						{
							ApplyOutputLevel(index);
						}

						return true;
					}

					bool DisableOutput(const uint8_t index)
					{
						if (index >= OutputCount)
						{
							return false;
						}

						Enabled[index] = false;

						if (Started)
						{
							ApplyOutputLevel(index);
						}

						return true;
					}

					bool IsOutputEnabled(const uint8_t index) const
					{
						return index < OutputCount && Enabled[index];
					}

					uint32_t GetPulseWidth(const uint8_t index) const
					{
						return index < OutputCount ? PulseWidthNanos[index] : 0;
					}

					uint8_t GetPin(const uint8_t index) const
					{
						if (index == 0)
						{
							return FirstPin;
						}

						if constexpr (SecondPin != InvalidPin)
						{
							if (index == 1)
							{
								return SecondPin;
							}
						}

						return InvalidPin;
					}

				private:
					void ApplyOutputLevel(const uint8_t index)
					{
						pwm_set_chan_level(SliceNumber,
							static_cast<pwm_chan>(ChannelNumbers[index]),
							Enabled[index] ? PulseWidthToLevel(PulseWidthNanos[index]) : 0);
					}

					uint16_t PulseWidthToLevel(const uint32_t pulseWidthNanos) const
					{
						return static_cast<uint16_t>(
							(static_cast<uint64_t>(pulseWidthNanos) * CounterCyclesPerPeriod) / PwmUpdatePeriodNanos
							);
					}
				};
			}
		}
	}
}
#endif
#endif