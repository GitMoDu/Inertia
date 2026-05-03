#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../Model.h"

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
					template<uint8_t Pin, uint16_t LedCount>
					class PioDriver : public Inertia::Components::Light::ILightColorDriver
					{
					private:
						static constexpr uint32_t Ws2812Frequency = 800000;
						static constexpr uint8_t BitsPerPixel = 24;
						static constexpr uint16_t ResetDelayMicros = 80;
						static constexpr uint8_t MicrosPerLed = 30;
						static constexpr uint8_t MaxQueuedPixelsAfterSubmit = 9;
						static constexpr uint8_t InvalidStateMachine = UINT8_MAX;
						static constexpr uint8_t InvalidPin = UINT8_MAX;
						static constexpr uint8_t CyclesPerBit = 10;

						static const uint16_t ProgramInstructions[4];
						static const pio_program_t Program;

					private:
						PIO PioInstance = nullptr;
						uint StateMachine = InvalidStateMachine;
						uint ProgramOffset = 0;
						uint32_t BusyUntilMicros = 0;
						bool Started = false;

					public:
						PioDriver() : Inertia::Components::Light::ILightColorDriver()
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
								pio_remove_program(PioInstance, &Program, ProgramOffset);
							}

							PioInstance = nullptr;
							StateMachine = InvalidStateMachine;
							ProgramOffset = 0;
							BusyUntilMicros = 0;
							Started = false;
						}

						bool SetLightColors(const Inertia::Components::Light::light_color_t* colors, const uint16_t count) override
						{
							if (!Started || (colors == nullptr) || (count != LedCount))
							{
								return false;
							}

							if (!IsReady())
							{
								return false;
							}

							for (uint16_t i = 0; i < count; i++)
							{
								pio_sm_put_blocking(PioInstance, StateMachine, EncodeColor(colors[i]) << 8u);
							}

							BusyUntilMicros = micros()
								+ ResetDelayMicros
								+ (static_cast<uint32_t>(MaxQueuedPixelsAfterSubmit) * MicrosPerLed);
							return true;
						}

					private:
						bool TryStart(PIO pio)
						{
							if ((pio == nullptr) || !pio_can_add_program(pio, &Program))
							{
								return false;
							}

							const int claimedStateMachine = pio_claim_unused_sm(pio, false);
							if (claimedStateMachine < 0)
							{
								return false;
							}

							const uint offset = pio_add_program(pio, &Program);

							pio_gpio_init(pio, Pin);
							pio_sm_set_consecutive_pindirs(pio, static_cast<uint>(claimedStateMachine), Pin, 1, true);

							pio_sm_config config = GetDefaultConfig(offset);
							sm_config_set_sideset_pins(&config, Pin);
							sm_config_set_out_shift(&config, false, true, BitsPerPixel);
							sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

							const float clockDivider = static_cast<float>(clock_get_hz(clk_sys))
								/ (static_cast<float>(Ws2812Frequency) * CyclesPerBit);
							sm_config_set_clkdiv(&config, clockDivider);

							pio_sm_init(pio, static_cast<uint>(claimedStateMachine), offset, &config);
							pio_sm_set_enabled(pio, static_cast<uint>(claimedStateMachine), true);

							PioInstance = pio;
							StateMachine = static_cast<uint>(claimedStateMachine);
							ProgramOffset = offset;
							return true;
						}

						static pio_sm_config GetDefaultConfig(const uint offset)
						{
							pio_sm_config config = pio_get_default_sm_config();
							sm_config_set_wrap(&config, offset, offset + 3);
							sm_config_set_sideset(&config, 1, false, false);
							return config;
						}

						static uint32_t EncodeColor(const Inertia::Components::Light::light_color_t color)
						{
							const uint32_t red = (color >> 16) & 0xFFu;
							const uint32_t green = (color >> 8) & 0xFFu;
							const uint32_t blue = color & 0xFFu;

							return (green << 16) | (red << 8) | blue;
						}

						bool IsReady() const
						{
							return static_cast<int32_t>(micros() - BusyUntilMicros) >= 0;
						}
					};

					template<uint8_t Pin, uint16_t LedCount>
					inline const uint16_t PioDriver<Pin, LedCount>::ProgramInstructions[4] =
					{
						0x6221,
						0x1123,
						0x1400,
						0xa442
					};

					template<uint8_t Pin, uint16_t LedCount>
					inline const pio_program_t PioDriver<Pin, LedCount>::Program =
					{
						.instructions = ProgramInstructions,
						.length = 4,
						.origin = -1
					};
				}
			}
		}
	}
}

#endif
#endif