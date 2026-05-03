#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_MODEL_h
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_MODEL_h

#include "../../Core/DataSource/Model.h"
#include "../../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace Servo
			{
				static constexpr uint32_t LOG_TAG = 557526181; // Random unique tag for Servo hardware logs.

				enum class LogCodeEnum : uint8_t
				{
					CalibrationLoadFailed,
					CalibrationSaveFailed,
					ServoSetFailed
				};

				static const uint32_t MAX_NANOS = 2000000;
				static const uint32_t MIN_NANOS = 1000000;
				static const uint32_t RANGE_NANOS = MAX_NANOS - MIN_NANOS;
				static const uint16_t MAX_MICROS = MAX_NANOS / 1000;
				static const uint16_t MIN_MICROS = MIN_NANOS / 1000;
				static const uint16_t RANGE_MICROS = RANGE_NANOS / 1000;

				template<uint32_t minNanos = MIN_NANOS
					, uint32_t maxNanos = MAX_NANOS
				>
				static uint32_t LimitNanoseconds(const uint32_t nanos)
				{
					if (nanos > maxNanos)
					{
						return maxNanos;
					}
					else if (nanos < minNanos)
					{
						return minNanos;
					}
					else
					{
						return nanos;
					}
				}

				struct IServoDriver
				{
					~IServoDriver() = default;

					virtual bool SetServo(const uint8_t index, const uint32_t pulseWidthNanos, const bool enabled) = 0;

					virtual uint8_t GetServoCount() const = 0;
				};
			}
		}
	}
}
#endif