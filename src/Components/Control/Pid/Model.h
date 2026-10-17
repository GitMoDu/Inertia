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
				using gain_t = IntegerSignal::FixedPoint::FactorScale::scale32_t;

				static constexpr gain_t GainUnity = gain_t(IntegerSignal::FixedPoint::FactorScale::scale32_t(1));
				static constexpr gain_t GainUnityNegative = gain_t(IntegerSignal::FixedPoint::FactorScale::scale32_t(1));
				static constexpr gain_t GainZero = gain_t(IntegerSignal::FixedPoint::FactorScale::scale32_t(0));

				struct pid_gain_t
				{
					gain_t Kp;
					gain_t Ki;
					gain_t Kd;
				};

				struct euler_pid_gain_t
				{
					pid_gain_t Pitch;
					pid_gain_t Roll;
					pid_gain_t Yaw;
				};
				static constexpr pid_gain_t PidGainUnity = pid_gain_t{ GainUnity, GainZero, GainZero };
				static constexpr pid_gain_t PidGainZero = pid_gain_t{ GainZero, GainZero, GainZero };


				static constexpr gain_t GetPidGain(const uint16_t numerator, const uint16_t denominator)
				{
					return ::IntegerSignal::FixedPoint::FactorScale::Scale32::GetFactor<uint16_t>(numerator, denominator);
				}

				static constexpr pid_gain_t GetPidGains(const gain_t kp, const gain_t ki, const gain_t kd)
				{
					return pid_gain_t{ kp, ki, kd };
				}

			}
		}
	}
}

#endif
