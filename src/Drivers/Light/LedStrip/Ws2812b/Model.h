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
					static constexpr uint32_t LOG_TAG = 3580714258;

					namespace OutputFormat
					{
						using Grb = Inertia::Components::Light::Strip::Model::Color::Grb;
					}

					static constexpr uint32_t BitRate = 800000;
					static constexpr uint8_t BitsPerChannel = 8;
					static constexpr uint16_t ResetDelayMicros = 80;

					namespace Timing
					{
						struct BitBang
						{
							static constexpr uint16_t ZeroPulseNs = 350;
							static constexpr uint16_t OnePulseNs = 900;
							static constexpr uint16_t TotalPeriodNs = 1250;
							static constexpr uint16_t ResetMicros = ResetDelayMicros;
						};
					}
				}
			}
		}
	}
}

#endif