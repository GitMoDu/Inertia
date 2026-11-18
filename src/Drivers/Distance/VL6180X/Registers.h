#ifndef _INERTIA_DRIVERS_DISTANCE_VL6180X_REGISTERS_h
#define _INERTIA_DRIVERS_DISTANCE_VL6180X_REGISTERS_h

#include <stdint.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Distance
		{
			namespace Vl6180x
			{
				namespace Register
				{
					static constexpr uint16_t ModelId = 0x0000;
					static constexpr uint16_t ModeGpio1 = 0x0011;
					static constexpr uint16_t InterruptConfigGpio = 0x0014;
					static constexpr uint16_t InterruptClear = 0x0015;
					static constexpr uint16_t FreshOutOfReset = 0x0016;
					static constexpr uint16_t RangeStart = 0x0018;
					static constexpr uint16_t MaxConvergenceTime = 0x001C;
					static constexpr uint16_t CrosstalkRateHigh = 0x001E;
					static constexpr uint16_t CrosstalkRateLow = 0x001F;
					static constexpr uint16_t PartToPartOffset = 0x0024;
					static constexpr uint16_t RangeVhvRecalibrate = 0x002E;
					static constexpr uint16_t VhvRepeatRate = 0x0031;
					static constexpr uint16_t RangeStatus = 0x004D;
					static constexpr uint16_t InterruptStatusGpio = 0x004F;
					static constexpr uint16_t RangeValue = 0x0062;
					static constexpr uint16_t RawRange = 0x0064;
					static constexpr uint16_t ReturnRateHigh = 0x0066;
					static constexpr uint16_t ReturnRateLow = 0x0067;
					static constexpr uint16_t AveragingSamplePeriod = 0x010A;
					static constexpr uint16_t I2cAddress = 0x0212;
				}
			}
		}
	}
}

#endif
