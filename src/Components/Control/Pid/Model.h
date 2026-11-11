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
				using gain_t = IntegerSignal::FixedPoint::FactorScale::ufactorscale32_t;

				static constexpr gain_t GainUnity = IntegerSignal::FixedPoint::FactorScale::UFactorScale32::SCALE_1X;
				static constexpr gain_t GainZero = 0;

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

				struct quad_pid_gains_t
				{
					euler_pid_gain_t OrientationGains;
					euler_pid_gain_t AngularVelocityGains;
				};

				struct altitude_hold_pid_gains_t
				{
					gain_t Kp;
					gain_t Ki;
					gain_t Kd;
				};

				static constexpr pid_gain_t PidGainUnity = pid_gain_t{ GainUnity, GainZero, GainZero };
				static constexpr pid_gain_t PidGainZero = pid_gain_t{ GainZero, GainZero, GainZero };
				static constexpr euler_pid_gain_t EulerPidGainUnity = euler_pid_gain_t{ PidGainUnity, PidGainUnity, PidGainUnity };
				static constexpr euler_pid_gain_t EulerPidGainZero = euler_pid_gain_t{ PidGainZero, PidGainZero, PidGainZero };


				static constexpr gain_t GetPidGain(const uint16_t numerator, const uint16_t denominator)
				{
					return ::IntegerSignal::FixedPoint::FactorScale::GetFactorU32<uint16_t>(numerator, denominator);
				}

				static constexpr pid_gain_t GetPidGains(const gain_t kp, const gain_t ki, const gain_t kd)
				{
					return pid_gain_t{ kp, ki, kd };
				}

				static constexpr euler_pid_gain_t GetEulerPidGains(const pid_gain_t pitch, const pid_gain_t roll, const pid_gain_t yaw)
				{
					return euler_pid_gain_t{ pitch, roll, yaw };
				}

			}
		}
	}
}

#endif
