#ifndef _INERTIA_COMPONENTS_CONTROL_PID_MODEL_h
#define _INERTIA_COMPONENTS_CONTROL_PID_MODEL_h

#include "../../../Components/Core/Primitives.h"

namespace Inertia
{
	namespace Components
	{
		namespace Control
		{
			namespace Pid
			{
				struct gain_t
				{
					IntegerSignal::FixedPoint::FactorScale::scale32_t value;
					int8_t polarity;
				};

				static constexpr gain_t GainUnity = gain_t{ IntegerSignal::FixedPoint::FactorScale::scale32_t(1), 1 };
				static constexpr gain_t GainUnityNegative = gain_t{ IntegerSignal::FixedPoint::FactorScale::scale32_t(1), -1 };
				static constexpr gain_t GainZero = gain_t{ IntegerSignal::FixedPoint::FactorScale::scale32_t(0), 1 };

				struct pid_gain_t
				{
					gain_t Kp;
					gain_t Ki;
					gain_t Kd;
				};

				static constexpr pid_gain_t PidGainUnity = pid_gain_t{ GainUnity, GainZero, GainZero };
				static constexpr pid_gain_t PidGainZero = pid_gain_t{ GainZero, GainZero, GainZero };

			}
		}
	}
}

#endif
