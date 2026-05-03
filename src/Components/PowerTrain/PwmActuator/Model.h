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

				struct pwm_calibration_t
				{
					uint32_t bottom;
					uint32_t top;
				};

				struct IPwmCalibration
				{
					~IPwmCalibration() = default;

					virtual pwm_calibration_t GetPwmCalibration(const uint8_t index) = 0;
				};

				struct IPwmCalibrationRepository : Inertia::Components::Lifecycle::ILifecycleDriver
				{
					~IPwmCalibrationRepository() = default;

					virtual bool GetPwmCalibration(const uint8_t index, pwm_calibration_t& calibration) = 0;
					virtual bool SetPwmCalibration(const uint8_t index, const pwm_calibration_t& calibration) = 0;

					virtual bool ClearPwmCalibrations() = 0;
				};

				struct DefaultCalibration : IPwmCalibration
				{
					~DefaultCalibration() = default;

					pwm_calibration_t GetPwmCalibration(const uint8_t /*index*/) override
					{
						return { 0, UINT32_MAX };
					}
				};
			}
		}
	}
}
#endif