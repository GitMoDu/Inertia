#ifndef _INERTIA_COMPONENTS_BOOT_COUNTER_MODEL_h
#define _INERTIA_COMPONENTS_BOOT_COUNTER_MODEL_h

#include <IntegerSignal.h>

#include "../../Framework/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace BootCounter
		{
			static constexpr uint32_t LOG_TAG = 890053290; // Random unique tag for Boot Counter.

			enum class LogCodeEnum : uint8_t
			{
				Booted,
				ErrorNoCounterSource
			};
		}
	}
}
#endif