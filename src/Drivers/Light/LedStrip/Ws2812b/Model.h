#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_MODEL_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_WS2812B_MODEL_h

#include "../../../../Components/Light/Strip/Model.h"
#include "../../../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Light
		{
			namespace LedStrip
			{
				namespace Ws2812b
				{
					static constexpr uint32_t LOG_TAG = 1272184249;

					namespace OutputFormat
					{
						using Grb = Inertia::Components::Light::Strip::Model::Color::Grb;
					}

					static constexpr uint32_t BitRate = 800000;
					static constexpr uint8_t BitsPerChannel = 8;
					static constexpr uint16_t ResetDelayMicros = 80;
				}
			}
		}
	}
}

#endif