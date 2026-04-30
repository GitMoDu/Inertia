#ifndef _INERTIA_COMPONENTS_POWERTRAIN_PWM_MODEL_h
#define _INERTIA_COMPONENTS_POWERTRAIN_PWM_MODEL_h

#include "../../Core/DataSource/Model.h"
#include "../../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace Pwm
			{
				static constexpr uint32_t LOG_TAG = 3648794794; // Random unique tag for PWM hardware logs.

				enum class LogCodeEnum : uint8_t
				{};

				static constexpr uint16_t MIN_LEVEL = 0;
				static constexpr uint16_t MAX_LEVEL = UINT16_MAX;

				template<uint16_t maxLevel = MAX_LEVEL>
				static constexpr uint16_t LimitLevel(const uint32_t level)
				{
					return level > maxLevel
						? maxLevel
						: static_cast<uint16_t>(level);
				}
			}
		}
	}
}
#endif
