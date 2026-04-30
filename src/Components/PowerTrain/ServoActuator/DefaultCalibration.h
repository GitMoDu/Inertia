#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_DEFAULT_CALIBRATION_h_
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_DEFAULT_CALIBRATION_h_

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace ServoActuator
			{
				struct DefaultCalibration : Inertia::Components::PowerTrain::Servo::IServoCalibration
				{
					~DefaultCalibration() = default;

					Inertia::Components::PowerTrain::Servo::servo_calibration_t GetServoCalibration(const uint8_t /*index*/) override
					{
						// Provide default calibration values for each servo index.
						return { 1000000, 1500000, 2000000 }; // Example values in nanoseconds.
					}
				};
			}
		}
	}
}
#endif