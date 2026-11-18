#ifndef _INERTIA_COMPONENTS_DISTANCE_MODEL_h
#define _INERTIA_COMPONENTS_DISTANCE_MODEL_h

#include "../Core/Primitives.h"

namespace Inertia
{
	namespace Components
	{
		namespace Distance
		{
			// Rate units are source-specific; calibration must use the same units.
			struct timestamped_signal_range16_t : Inertia::Model::timestamped_quality_range16_t
			{
				uint16_t ReturnRate;
			};

			struct return_rate_calibration_t
			{
				uint16_t Threshold;
				uint16_t RatePerMillimetre;
				uint16_t MaxCorrectionMillimetres;
			};
		}
	}
}

#endif
