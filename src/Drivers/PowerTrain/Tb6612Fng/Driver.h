#ifndef _INERTIA_DRIVERS_POWERTRAIN_TB6612FNG_DRIVER_h
#define _INERTIA_DRIVERS_POWERTRAIN_TB6612FNG_DRIVER_h

#include <Arduino.h>

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)
#include <hardware/gpio.h>
#endif

#include "../../../Components/PowerTrain/Model.h"
#include "../../../Components/PowerTrain/Pwm/Model.h"
#include "../../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace PowerTrain
		{
			namespace Tb6612Fng
			{
				using namespace IntegerSignal::FixedPoint::FactorScale;

				template<uint8_t Input1Pin, uint8_t Input2Pin>
				class Driver : public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Inertia::Components::PowerTrain::IActuatorDriver
				{
				private:
					Inertia::Components::PowerTrain::Pwm::IPwmDriver& PwmDriver;
					const uint8_t PwmIndex;

				private:
					scale32_t PwmScaleFactor = 0;
					bool Started = false;

				public:
					Driver(Inertia::Components::PowerTrain::Pwm::IPwmDriver& pwmDriver, const uint8_t pwmIndex)
						: Inertia::Components::Lifecycle::ILifecycleDriver()
						, Inertia::Components::PowerTrain::IActuatorDriver()
						, PwmDriver(pwmDriver)
						, PwmIndex(pwmIndex)
					{}

					~Driver() = default;

					bool Start() override
					{
						if (Started)
						{
							return true;
						}

						if (PwmIndex >= PwmDriver.GetPwmCount())
						{
							return false;
						}

#if defined(ARDUINO_ARCH_RP2040)
						gpio_init(Input1Pin);
						gpio_init(Input2Pin);
						gpio_set_function(Input1Pin, GPIO_FUNC_SIO);
						gpio_set_function(Input2Pin, GPIO_FUNC_SIO);
						gpio_set_dir(Input1Pin, GPIO_OUT);
						gpio_set_dir(Input2Pin, GPIO_OUT);
#else
						pinMode(Input1Pin, OUTPUT);
						pinMode(Input2Pin, OUTPUT);
#endif

						const uint16_t pwmRange = PwmDriver.GetPwmRange();
						PwmScaleFactor = pwmRange > 0
							? GetFactor32(static_cast<uint32_t>(pwmRange), static_cast<uint32_t>(Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE))
							: 0;

						DisableDrive();
						Started = true;
						return true;
					}

					void Stop() override
					{
						if (!Started)
						{
							return;
						}

						DisableDrive();
						Started = false;
					}

					void SetActuator(const uint8_t index, const Inertia::Components::PowerTrain::actuate_t value, const Inertia::Components::PowerTrain::ActuationModeEnum mode) override
					{
						if (!Started || index != 0)
						{
							return;
						}

						switch (mode)
						{
						case Inertia::Components::PowerTrain::ActuationModeEnum::Disabled:
							DisableDrive();
							return;
						case Inertia::Components::PowerTrain::ActuationModeEnum::Forward:
							SetDirectionPins(true, false);
							break;
						case Inertia::Components::PowerTrain::ActuationModeEnum::Backward:
							SetDirectionPins(false, true);
							break;
						case Inertia::Components::PowerTrain::ActuationModeEnum::Custom:
						default:
							DisableDrive();
							return;
						}

						PwmDriver.SetPwm(PwmIndex, GetPwmLevel(value), true);
					}


				private:
					void SetDirectionPins(const bool input1High, const bool input2High)
					{
#if defined(ARDUINO_ARCH_RP2040)
						gpio_put(Input1Pin, input1High);
						gpio_put(Input2Pin, input2High);
#else
						digitalWrite(Input1Pin, input1High ? HIGH : LOW);
						digitalWrite(Input2Pin, input2High ? HIGH : LOW);
#endif
					}

					void DisableDrive()
					{
						PwmDriver.SetPwm(PwmIndex, 0, false);
						SetDirectionPins(false, false);
					}

					uint16_t GetPwmLevel(const Inertia::Components::PowerTrain::actuate_t value) const
					{
						const uint32_t scaledLevel = Scale(PwmScaleFactor, value);
						return static_cast<uint16_t>(IntegerSignal::LimitValue<uint32_t>(scaledLevel, 0, PwmDriver.GetPwmRange()));
					}

				};
			}
		}
	}
}
#endif

