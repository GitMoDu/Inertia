#ifndef _INERTIA_DRIVERS_DISTANCE_VL6180X_MODEL_h
#define _INERTIA_DRIVERS_DISTANCE_VL6180X_MODEL_h

#include <stdint.h>
#include "Registers.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Distance
		{
			namespace Vl6180x
			{
				static constexpr uint8_t DEFAULT_ADDRESS = 0x29;
				static constexpr uint32_t LOG_TAG = 1791638307;
				static constexpr uint8_t MODEL_ID = 0xB4;
				static constexpr uint8_t RANGE_READY_MASK = 0x01;
				static constexpr uint8_t RANGE_STATUS_SHIFT = 4;
				static constexpr uint8_t INTERRUPT_ERROR_MASK = 0xC0;
				static constexpr uint8_t RANGE_INTERRUPT_MASK = 0x07;
				static constexpr uint8_t RANGE_INTERRUPT_READY = 0x04;
				static constexpr uint8_t CLEAR_ALL_INTERRUPTS = 0x07;
				static constexpr uint8_t START_SINGLE_SHOT = 0x01;

				// ST RESULT_RANGE_STATUS (0x004D), bits 7:4. Fixed values for client decoders.
				// Source: stm32duino/VL6180X src/vl6180x_def.h, RangeError_u.
				enum class RangeStatusEnum : uint8_t
				{
					Valid = 0,
					VcselContinuityTest = 1,
					VcselWatchdogTest = 2,
					VcselWatchdog = 3,
					Pll1Lock = 4,
					Pll2Lock = 5,
					EarlyConvergenceEstimate = 6,
					MaxConvergence = 7,
					NoTargetIgnore = 8,
					Reserved9 = 9,
					Reserved10 = 10,
					MaxSignalToNoiseRatio = 11, // SNR check failed; ST's hardware status name.
					RawRangingUnderflow = 12,
					RawRangingOverflow = 13,
					RangingUnderflow = 14,
					RangingOverflow = 15,
					NoReading = 255 // Software sentinel, not a hardware register status.
				};

				struct Vl6180xDiagnostics
				{
					uint8_t RangeStatus = static_cast<uint8_t>(RangeStatusEnum::NoReading);
					uint8_t RawRangeMm = 0; // Register 0x0064, before hardware compensation.
					uint8_t RangeValMm = 0; // Register 0x0062, after hardware compensation.
					uint16_t ReturnRate = 0; // Unsigned 9.7 Mcps; 128 = 1 Mcps.
					uint32_t TimestampMicros = 0;
				};

				// Log Code uses ErrorEnum; for SensorStatus, Log Value uses RangeStatusEnum.
				enum class ErrorEnum : uint8_t
				{
					None = 0,
					InvalidConfiguration = 1,
					I2c = 2,
					WrongModel = 3,
					NotReady = 4,
					Timeout = 5,
					SensorStatus = 6,
					Interrupt = 7,
					UnexpectedReset = 8,
					AddressChangeWhileRunning = 9
				};
			}
		}
	}
}

#endif