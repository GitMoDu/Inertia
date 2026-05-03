#ifndef _INERTIA_COMPONENTS_LIGHT_MODEL_h
#define _INERTIA_COMPONENTS_LIGHT_MODEL_h

#include "../../Components/Core/Primitives.h"
#include "../../Components/Core/Lifecycle/Model.h"
#include "../../Components/Core/DataSource/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Light
		{
			static constexpr uint32_t LOG_TAG = 2785477403; // Random unique tag for Light logs.

			using light_intensity_t = uint16_t;

			using light_color_t = uint32_t; // RGB color represented as 0xRRGGBB.

			struct ILightIntensityDriver : Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~ILightIntensityDriver() = default;

				virtual void SetLightIntensities(const Inertia::Components::Light::light_intensity_t* intensities, const uint16_t count) = 0;
			};

			struct ILightColorDriver : Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~ILightColorDriver() = default;

				virtual bool SetLightColors(const Inertia::Components::Light::light_color_t* colors, const uint16_t count) = 0;
			};
		}
	}
}
#endif