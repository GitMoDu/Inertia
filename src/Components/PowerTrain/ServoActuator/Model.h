#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_MODEL_h
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_MODEL_h

#include "../Model.h"
#include "../../../Components/PowerTrain/Servo/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace ServoActuator
			{
				static constexpr uint32_t LOG_TAG = 1630377650; // Random unique tag for Servo Actuator.

				enum class LogCodeEnum : uint8_t
				{
					CalibrationLoadFailed,
					CalibrationSaveFailed,
					ServoSetFailed
				};
				

				struct IPwmDriver
				{
					~IPwmDriver() = default;

					virtual bool SetPwm(const uint8_t index, const uint16_t level, const bool enabled) = 0;

					virtual uint8_t GetPwmCount() const = 0;
					virtual uint16_t GetPwmRange() const = 0;
				};
			}
		}
	}
}
#endif