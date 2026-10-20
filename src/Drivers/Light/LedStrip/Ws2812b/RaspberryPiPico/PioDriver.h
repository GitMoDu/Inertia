#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../Model.h"

#include "Detail/PioProgram.h"

#include <Arduino.h>

#include <hardware/clocks.h>
#include <hardware/pio.h>

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
					namespace RaspberryPiPico
					{
						template<uint8_t Pin, uint16_t LedCount>
						class PioDriver : public Inertia::Components::Light::ILightColorDriver<OutputFormat::Grb>
						{
						private:
							static constexpr uint8_t BitsPerPixel = OutputFormat::Grb::ChannelCount * 8;
							static constexpr uint8_t MicrosPerLed = 30;
							static constexpr uint8_t MaxQueuedPixelsAfterSubmit = 9;
							static constexpr uint8_t InvalidStateMachine = UINT8_MAX;
							static constexpr uint8_t InvalidPin = UINT8_MAX;
							static constexpr uint8_t CyclesPerBit = 10;
							static constexpr uint8_t GpioPinsPerBase = 32;
							static constexpr uint8_t DefaultGpioBase = 0;
							static constexpr uint8_t HighGpioBase = 16;

							pio_hw_t* PioInstance = nullptr;
							uint32_t StateMachine = InvalidStateMachine;
							uint32_t ProgramOffset = 0;
							uint32_t BusyUntilMicros = 0;
							bool Started = false;

						public:
							PioDriver() : Inertia::Components::Light::ILightColorDriver<OutputFormat::Grb>()
							{}

							~PioDriver()
							{
								Stop();
							}

							bool Start() override
							{
								static_assert(Pin != InvalidPin, "Pin must be a valid GPIO pin.");
								static_assert(LedCount > 0, "LedCount must be greater than zero.");

								if (Started)
								{
									return true;
								}

								if (TryStart(pio0) || TryStart(pio1))
								{
									BusyUntilMicros = 0;
									Started = true;
									return true;
								}

								return false;
							}

							void Stop() override
							{
								if (!Started)
								{
									return;
								}

								if (PioInstance != nullptr)
								{
									pio_sm_set_enabled(PioInstance, StateMachine, false);
									pio_sm_unclaim(PioInstance, StateMachine);
									pio_remove_program(PioInstance, &Detail::PioProgram<Pin, LedCount>::Value, ProgramOffset);
								}

								PioInstance = nullptr;
								StateMachine = InvalidStateMachine;
								ProgramOffset = 0;
								BusyUntilMicros = 0;
								Started = false;
							}

							bool Push(const uint8_t* buffer, const size_t size) override
							{
								if (!Started || !IsReady() || buffer == nullptr || size != size_t(LedCount) * OutputFormat::Grb::ChannelCount)
								{
									return false;
								}

								for (uint16_t i = 0; i < LedCount; i++)
								{
									const size_t byteIndex = size_t(i) * OutputFormat::Grb::ChannelCount;
									const uint32_t color = (uint32_t(buffer[byteIndex]) << 16)
										| (uint32_t(buffer[byteIndex + 1]) << 8)
										| buffer[byteIndex + 2];
									pio_sm_put_blocking(PioInstance, StateMachine, color << 8u);
								}

								BusyUntilMicros = micros()
									+ Ws2812b::ResetDelayMicros
									+ (static_cast<uint32_t>(MaxQueuedPixelsAfterSubmit) * MicrosPerLed);
								return true;
							}

						private:
							bool TryStart(pio_hw_t* const& pio)
							{
								if ((pio == nullptr) || !pio_can_add_program(pio, &Detail::PioProgram<Pin, LedCount>::Value))
								{
									return false;
								}

								if (!ConfigureGpioBase(pio))
								{
									return false;
								}

								const int claimedStateMachine = pio_claim_unused_sm(pio, false);
								if (claimedStateMachine < 0)
								{
									return false;
								}

								const uint32_t offset = pio_add_program(pio, &Detail::PioProgram<Pin, LedCount>::Value);
								pio_gpio_init(pio, Pin);
								pio_sm_set_consecutive_pindirs(pio, static_cast<uint32_t>(claimedStateMachine), Pin, 1, true);

								pio_sm_config config = GetDefaultConfig(offset);
								sm_config_set_sideset_pins(&config, Pin);
								sm_config_set_out_shift(&config, false, true, BitsPerPixel);
								sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

								const float clockDivider = static_cast<float>(clock_get_hz(clk_sys))
									/ (static_cast<float>(Ws2812b::BitRate) * CyclesPerBit);
								sm_config_set_clkdiv(&config, clockDivider);

								pio_sm_init(pio, static_cast<uint32_t>(claimedStateMachine), offset, &config);
								pio_sm_set_enabled(pio, static_cast<uint32_t>(claimedStateMachine), true);

								PioInstance = pio;
								StateMachine = static_cast<uint32_t>(claimedStateMachine);
								ProgramOffset = offset;
								return true;
							}

							static bool ConfigureGpioBase(pio_hw_t* const& pio)
							{
#if defined(PICO_RP2350)
								const uint32_t gpioBase = GetRequiredGpioBase();
								if (pio_get_gpio_base(pio) == gpioBase)
								{
									return true;
								}
								if (pio->ctrl & (PIO_CTRL_SM_ENABLE_BITS | PIO_CTRL_CLKDIV_RESTART_BITS))
								{
									return false;
								}
								pio_set_gpio_base(pio, gpioBase);
#endif
								return true;
							}

							static constexpr uint32_t GetRequiredGpioBase()
							{
								return Pin >= GpioPinsPerBase ? HighGpioBase : DefaultGpioBase;
							}

							static pio_sm_config GetDefaultConfig(const uint32_t offset)
							{
								pio_sm_config config = pio_get_default_sm_config();
								sm_config_set_wrap(&config, offset, offset + 3);
								sm_config_set_sideset(&config, 1, false, false);
								return config;
							}

							bool IsReady() const
							{
								return static_cast<int32_t>(micros() - BusyUntilMicros) >= 0;
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
