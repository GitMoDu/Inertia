#ifndef _INERTIA_BATTERY_MODEL_h
#define _INERTIA_BATTERY_MODEL_h

#include "../../Components/Core/Lifecycle/Model.h"

#include <stdint.h>

namespace Inertia
{
	namespace Components
	{
		namespace Battery
		{
			struct calibration_t
			{
				uint16_t CriticalVoltage;
				uint16_t TopVoltage;
				uint16_t Overvoltage;
			};

			struct ICalibrationRepository : public Lifecycle::ILifecycleDriver
			{
				~ICalibrationRepository() = default;

				virtual bool GetCalibration(calibration_t& calibration) = 0;
				virtual bool SetCalibration(const calibration_t& calibration) = 0;
				virtual bool ClearCalibration() = 0;
			};
		}
	}
}

#endif