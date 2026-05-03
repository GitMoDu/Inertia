#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_RASPBERRY_PI_PICO_PIO_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../../../../../Components/Light/Model.h"

#include <Arduino.h>

#include <hardware/clocks.h>
#include <hardware/pio.h>

namespace Inertia
{
    namespace Drivers
    {
        namespace LedStrip
        {
            namespace Ws2812b
            {
                namespace DriverInternals
                {
                    static constexpr uint32_t Ws2812Frequency = 800000;
                    static constexpr uint8_t BitsPerPixel = 24;
                    static constexpr uint16_t ResetDelayMicros = 80;
                    static constexpr uint8_t InvalidStateMachine = UINT8_MAX;

                    // Generated from the Pico SDK ws2812 PIO example.
                    static const uint16_t Ws2812ProgramInstructions[] =
                    {
                        0x6221, // out x, 1 side 0 [2]
                        0x1123, // jmp !x, 3 side 1 [1]
                        0x1400, // jmp 0 side 1 [4]
                        0xa442  // nop side 0 [4]
                    };

                    static const pio_program_t Ws2812Program =
                    {
                        .instructions = Ws2812ProgramInstructions,
                        .length = 4,
                        .origin = -1
                    };

                    static constexpr uint Ws2812ProgramWrapTarget = 0;
                    static constexpr uint Ws2812ProgramWrap = 3;

                    static inline pio_sm_config GetDefaultProgramConfig(const uint offset)
                    {
                        pio_sm_config config = pio_get_default_sm_config();
                        sm_config_set_wrap(&config, offset + Ws2812ProgramWrapTarget, offset + Ws2812ProgramWrap);
                        sm_config_set_sideset(&config, 1, false, false);
                        return config;
                    }

                    static inline uint32_t RgbToGrb(const Inertia::Components::Light::light_color_t color)
                    {
                        const uint32_t red = (color >> 16) & 0xFFu;
                        const uint32_t green = (color >> 8) & 0xFFu;
                        const uint32_t blue = color & 0xFFu;

                        return (green << 16) | (red << 8) | blue;
                    }
                }

                class Ws2812bDriver : public Inertia::Components::Light::ILightColorDriver
                {
                private:
                    const uint8_t Pin;
                    const uint16_t LedCount;

                private:
                    PIO PioInstance = nullptr;
                    uint StateMachine = DriverInternals::InvalidStateMachine;
                    uint ProgramOffset = 0;
                    bool Started = false;
                    Inertia::Components::Light::light_color_t* PixelColors = nullptr;

                public:
                    Ws2812bDriver(const uint8_t pin, const uint16_t ledCount)
                        : Pin(pin), LedCount(ledCount)
                    {
                        if (LedCount > 0)
                        {
                            PixelColors = new Inertia::Components::Light::light_color_t[LedCount]{};
                        }
                    }

                    ~Ws2812bDriver()
                    {
                        Stop();

                        if (PixelColors != nullptr)
                        {
                            delete[] PixelColors;
                            PixelColors = nullptr;
                        }
                    }

                public:
                    bool Start()
                    {
                        if (Started)
                        {
                            return true;
                        }

                        if ((Pin == UINT8_MAX)
                            || (LedCount == 0)
                            || (PixelColors == nullptr))
                        {
                            return false;
                        }

                        if (TryStartOnPio(pio0) || TryStartOnPio(pio1))
                        {
                            Started = true;
                            Clear();
                            Show();
                            return true;
                        }

                        return false;
                    }

                    void Stop()
                    {
                        if (!Started)
                        {
                            return;
                        }

                        Clear();
                        Show();
                        delayMicroseconds(DriverInternals::ResetDelayMicros);

                        if (PioInstance != nullptr)
                        {
                            pio_sm_set_enabled(PioInstance, StateMachine, false);
                            pio_sm_unclaim(PioInstance, StateMachine);
                            pio_remove_program(PioInstance, &DriverInternals::Ws2812Program, ProgramOffset);
                        }

                        PioInstance = nullptr;
                        StateMachine = DriverInternals::InvalidStateMachine;
                        ProgramOffset = 0;
                        Started = false;
                    }

                    void Clear()
                    {
                        if (PixelColors == nullptr)
                        {
                            return;
                        }

                        for (uint16_t i = 0; i < LedCount; i++)
                        {
                            PixelColors[i] = 0;
                        }
                    }

                    void Show()
                    {
                        if (!Started)
                        {
                            return;
                        }

                        for (uint16_t i = 0; i < LedCount; i++)
                        {
                            const uint32_t grb = DriverInternals::RgbToGrb(PixelColors[i]);
                            pio_sm_put_blocking(PioInstance, StateMachine, grb << 8u);
                        }

                        delayMicroseconds(DriverInternals::ResetDelayMicros);
                    }

                    virtual void SetLightColor(const uint8_t index, const Inertia::Components::Light::light_color_t color) override
                    {
                        if ((index >= LedCount)
                            || (PixelColors == nullptr))
                        {
                            return;
                        }

                        if (!Started && !Start())
                        {
                            return;
                        }

                        PixelColors[index] = color;
                        Show();
                    }

                private:
                    bool TryStartOnPio(PIO pio)
                    {
                        if (pio == nullptr)
                        {
                            return false;
                        }

                        if (!pio_can_add_program(pio, &DriverInternals::Ws2812Program))
                        {
                            return false;
                        }

                        const int claimedStateMachine = pio_claim_unused_sm(pio, false);
                        if (claimedStateMachine < 0)
                        {
                            return false;
                        }

                        const uint offset = pio_add_program(pio, &DriverInternals::Ws2812Program);

                        pio_gpio_init(pio, Pin);
                        pio_sm_set_consecutive_pindirs(pio, static_cast<uint>(claimedStateMachine), Pin, 1, true);

                        pio_sm_config config = DriverInternals::GetDefaultProgramConfig(offset);
                        sm_config_set_sideset_pins(&config, Pin);
                        sm_config_set_out_shift(&config, false, true, DriverInternals::BitsPerPixel);
                        sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

                        static constexpr uint8_t CyclesPerBit = 10;
                        const float clockDivider = static_cast<float>(clock_get_hz(clk_sys))
                            / (static_cast<float>(DriverInternals::Ws2812Frequency) * CyclesPerBit);

                        sm_config_set_clkdiv(&config, clockDivider);
                        pio_sm_init(pio, static_cast<uint>(claimedStateMachine), offset, &config);
                        pio_sm_set_enabled(pio, static_cast<uint>(claimedStateMachine), true);

                        PioInstance = pio;
                        StateMachine = static_cast<uint>(claimedStateMachine);
                        ProgramOffset = offset;
                        return true;
                    }
                };
            }
        }
    }
}

#endif
#endif