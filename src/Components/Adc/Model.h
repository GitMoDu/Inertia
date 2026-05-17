#ifndef _INERTIA_COMPONENTS_ADC_MODEL_h
#define _INERTIA_COMPONENTS_ADC_MODEL_h

#include "../Core/DataSource/Model.h"
#include "../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Adc
		{
			static constexpr uint32_t LOG_TAG = 2918095640; // Random unique tag for ADC hardware logs.

			using analog_value_t = uint16_t;
			static constexpr analog_value_t ANALOG_VALUE_MAX = UINT16_MAX;

			enum class LogCodeEnum : uint8_t
			{};
		}
	}
}
#endif
