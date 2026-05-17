#ifndef _INERTIA_DRIVERS_LIGHT_PWM_RASPBERRY_PI_PICO_LIGHT_INTENSITY_SLICE_DRIVER_h
#define _INERTIA_DRIVERS_LIGHT_PWM_RASPBERRY_PI_PICO_LIGHT_INTENSITY_SLICE_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../../../../Components/PowerTrain/Pwm/Model.h"
#include "../../../../Components/Light/Model.h"
#include "../../../Pwm/RaspberryPiPico/SliceDriver.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Light
		{
			namespace Pwm
			{
				namespace RaspberryPiPico
				{
					using namespace Inertia::Drivers::Pwm::RaspberryPiPico;

					static_assert(Inertia::Components::PowerTrain::Pwm::MAX_LEVEL == Inertia::Components::Light::LIGHT_INTENSITY_MAX, "PWM range must be equal to light intensity range for direct mapping in LightIntensitySliceDriver.");

					template<uint8_t FirstPin
						, uint8_t SecondPin = InvalidPin
						, uint32_t PwmUpdatePeriodNanos = 50000 // 50 microseconds default period, which corresponds to 20 kHz frequency, above the audible range for humans.
						, uint16_t PwmRange = Inertia::Components::PowerTrain::Pwm::MAX_LEVEL
					>
					class LightIntensitySliceDriver : public SliceDriver<FirstPin, SecondPin, PwmUpdatePeriodNanos, PwmRange>,
						public Inertia::Components::Light::ILightIntensityDriver
					{
					private:
						using BaseDriver = SliceDriver<FirstPin, SecondPin, PwmUpdatePeriodNanos, PwmRange>;

					public:
						bool Start() override
						{
							return BaseDriver::Start();
						}

						void Stop() override
						{
							BaseDriver::Stop();
						}

						void SetLightIntensities(const Inertia::Components::Light::light_intensity_t* intensities, const uint16_t count) override
						{
							if (intensities == nullptr)
							{
								for (uint8_t i = 0; i < BaseDriver::OutputCount; i++)
								{
									BaseDriver::DisableOutput(i);
								}
								return;
							}

							for (uint8_t i = 0; i < BaseDriver::OutputCount; i++)
							{
								if (i < count)
								{
									BaseDriver::SetDutyCycle(i, intensities[i]);
									BaseDriver::EnableOutput(i);
								}
								else
								{
									BaseDriver::DisableOutput(i);
								}
							}
						}
					};
				}
			}
		}
	}
}
#endif
#endif
