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
					EnumCount
				};

				/// <summary>
				/// Represents calibration values for a servo motor.
				/// Values in nanoseconds.
				/// 3-point calibration allows basic linear mapping of input drive magnitude to output pulse width,
				/// with separate values for bottom (full reverse), center (neutral), and top (full forward) positions.
				/// </summary>
				struct servo_calibration_t
				{
					uint32_t bottom;
					uint32_t center;
					uint32_t top;
				};

				struct IServoCalibration
				{
					~IServoCalibration() = default;

					virtual servo_calibration_t GetServoCalibration(const uint8_t index) = 0;
				};

				struct IServoCalibrationRepository : Inertia::Components::Lifecycle::ILifecycleDriver
				{
					~IServoCalibrationRepository() = default;

					virtual bool GetServoCalibration(const uint8_t index, servo_calibration_t& calibration) = 0;
					virtual bool SetServoCalibration(const uint8_t index, const servo_calibration_t& calibration) = 0;

					virtual bool ClearServoCalibrations() = 0;
				};

				struct DefaultCalibration : Inertia::Components::PowerTrain::ServoActuator::IServoCalibration
				{
					~DefaultCalibration() = default;

					Inertia::Components::PowerTrain::ServoActuator::servo_calibration_t GetServoCalibration(const uint8_t /*index*/) override
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