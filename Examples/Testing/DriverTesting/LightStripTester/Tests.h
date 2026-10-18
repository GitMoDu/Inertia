#ifndef INERTIA_EXAMPLES_DRIVER_TESTING_LIGHT_STRIP_TESTER_TESTS_H
#define INERTIA_EXAMPLES_DRIVER_TESTING_LIGHT_STRIP_TESTER_TESTS_H

#include <Arduino.h>

namespace LightStripTester
{
	template<typename TSourceColorModel, typename TStripOutput, typename TStripDriver, uint16_t LedCount>
	class Tests
	{
	public:
		static void PushTestFrame(TStripDriver& strip, const uint8_t phase)
		{
			using source_color_t = typename TSourceColorModel::color_t;
			using strip_color_t = typename TStripOutput::color_t;
			static constexpr uint8_t DemoBrightness = 16;
			const source_color_t red = static_cast<source_color_t>(DemoBrightness) << 16;
			const source_color_t green = static_cast<source_color_t>(DemoBrightness) << 8;
			const source_color_t blue = DemoBrightness;

			const source_color_t TestColors[] =
			{
				0,
				red,
				green,
				red | green,
				blue,
				red | blue,
				green | blue,
				red | green | blue
			};

			for (uint16_t index = 0; index < LedCount; ++index)
			{
				const uint16_t colorIndex = (index + phase) % (sizeof(TestColors) / sizeof(TestColors[0]));
				const strip_color_t color = TStripOutput::template Convert<TSourceColorModel>(TestColors[colorIndex]);
				strip.SetLightColor(index, color);
			}

			strip.Push();
		}
	};
}

#endif
