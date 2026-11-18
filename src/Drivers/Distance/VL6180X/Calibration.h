#ifndef _INERTIA_DRIVERS_DISTANCE_VL6180X_CALIBRATION_h
#define _INERTIA_DRIVERS_DISTANCE_VL6180X_CALIBRATION_h

#include <stdint.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Distance
		{
			namespace Vl6180x
			{
				// AN4545 section 5.1.1. Use averaged valid RangeValMm with crosstalk disabled.
				// Return rate and result are unsigned 9.7 Mcps; distances are millimetres.
				static constexpr uint16_t CalculateCrosstalkCompensation(const uint16_t returnRate,
					const uint16_t trueDistMm, const uint16_t measuredDistMm)
				{
					return measuredDistMm >= trueDistMm ? 0 : static_cast<uint16_t>(
						static_cast<uint32_t>(returnRate) * (trueDistMm - measuredDistMm) / trueDistMm);
				}
			}
		}
	}
}

#endif
