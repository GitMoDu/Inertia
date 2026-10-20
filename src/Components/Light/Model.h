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

			namespace ColorModel
			{
				struct Rgb888
				{
					using color_t = uint32_t;
					using component_t = uint8_t;
					static constexpr component_t COMPONENT_MAX = UINT8_MAX;

					static constexpr component_t Red(const color_t color) { return static_cast<component_t>(color >> 16); }
					static constexpr component_t Green(const color_t color) { return static_cast<component_t>(color >> 8); }
					static constexpr component_t Blue(const color_t color) { return static_cast<component_t>(color); }
					static constexpr color_t FromRgb888(const color_t color) { return color; }
				};

				struct Rgb10
				{
					using color_t = IntegerSignal::Rgb10::color_t;
					using component_t = IntegerSignal::Rgb10::component_t;
					static constexpr component_t COMPONENT_MAX = IntegerSignal::Rgb10::COMPONENT_MAX;

					static constexpr component_t Red(const color_t color) { return IntegerSignal::Rgb10::Red(color); }
					static constexpr component_t Green(const color_t color) { return IntegerSignal::Rgb10::Green(color); }
					static constexpr component_t Blue(const color_t color) { return IntegerSignal::Rgb10::Blue(color); }
					static constexpr color_t FromRgb888(const uint32_t color)
					{
						return IntegerSignal::Rgb10::Color8(
							static_cast<uint8_t>(color >> 16),
							static_cast<uint8_t>(color >> 8),
							static_cast<uint8_t>(color));
					}
				};

				struct RgbFraction16
				{
					using color_t = IntegerSignal::RgbFraction16::color_t;
					using component_t = IntegerSignal::RgbFraction16::component_t;
					static constexpr component_t COMPONENT_MAX = IntegerSignal::RgbFraction16::COMPONENT_MAX;

					static constexpr component_t Red(const color_t color) { return IntegerSignal::RgbFraction16::Red(color); }
					static constexpr component_t Green(const color_t color) { return IntegerSignal::RgbFraction16::Green(color); }
					static constexpr component_t Blue(const color_t color) { return IntegerSignal::RgbFraction16::Blue(color); }
					static constexpr color_t FromRgb888(const uint32_t color) { return IntegerSignal::RgbFraction16::Color8(color); }
				};
			}

			static constexpr light_intensity_t LIGHT_INTENSITY_MAX = UINT16_MAX;

			struct ILightIntensityDriver : Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~ILightIntensityDriver() = default;

				virtual void SetLightIntensities(const Inertia::Components::Light::light_intensity_t* intensities, const uint16_t count) = 0;
			};

			template<typename TStripColorModel>
			struct ILightColorDriver : Inertia::Components::Lifecycle::ILifecycleDriver
			{
				using color_t = typename TStripColorModel::color_t;

				~ILightColorDriver() = default;

				virtual bool Push(const uint8_t* buffer, const size_t size) = 0;
				virtual bool IsReady() const { return true; }
			};
		}
	}
}
#endif