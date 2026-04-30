#ifndef _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_MODEL_h
#define _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_MODEL_h

#include "../Model.h"
#include "../../../Components/PowerTrain/Pwm/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace PwmActuator
			{
				static constexpr uint32_t LOG_TAG = 795646911; // Random unique tag for PWM Actuator.

				enum class LogCodeEnum : uint8_t
				{
					CalibrationLoadFailed,
					CalibrationSaveFailed
				};
			}
		}
	}
}
#endif