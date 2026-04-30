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
					EnumCount
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