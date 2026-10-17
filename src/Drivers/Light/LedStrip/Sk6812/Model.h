#ifndef _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_MODEL_h
#define _INERTIA_DRIVERS_LIGHT_LED_STRIP_SK6812_MODEL_h

#include "../../../../Components/Light/Strip/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Light
		{
			namespace LedStrip
			{
				namespace Sk6812
				{
					namespace OutputFormat
					{
						using Rgb = Inertia::Components::Light::Strip::Model::Color::Grb;
						using Rgbw = Inertia::Components::Light::Strip::Model::Color::Grbw;

						template<uint8_t WarmWhiteRatio>
						using Rgbww = Inertia::Components::Light::Strip::Model::Color::Grbww<WarmWhiteRatio>;
					}
				}
			}
		}
	}
}

#endif
