#ifndef LIGHT_STRIP_COLOR_TESTS_H
#define LIGHT_STRIP_COLOR_TESTS_H

#include <Arduino.h>
#include <InertiaModel.h>

namespace LightStripColorTests
{
	namespace Light = Inertia::Components::Light;
	using Channel = Light::Strip::Model::Color::Channel;
	using Rgb888 = Light::ColorModel::Rgb888;
	using Rgb10 = Light::ColorModel::Rgb10;
	using RgbFraction16 = Light::ColorModel::RgbFraction16;
	using Grb = Light::Strip::Model::Color::Grb;
	using RgbwGrbw = Light::Strip::Model::Color::Rgbw<Channel::Green, Channel::Red, Channel::Blue, Channel::White>;
	using RgbwwGrbww = Light::Strip::Model::Color::Rgbww<128,
		Channel::Green, Channel::Red, Channel::Blue, Channel::WarmWhite, Channel::CoolWhite>;

	struct TestStripDriver : Light::ILightColorDriver<Grb>
	{
		Grb::color_t Colors[2]{};
		bool Pushed = false;

		bool Start() override { return true; }
		void Stop() override {}

		bool SetLightColor(const uint16_t index, const Grb::color_t color) override
		{
			if (index >= 2) return false;
			Colors[index] = color;
			return true;
		}

		bool Push() override
		{
			Pushed = true;
			return true;
		}
	};

	static_assert(Grb::ChannelCount == 3, "GRB must have three channels.");
	static_assert(RgbwGrbw::ChannelCount == 4, "GRBW must have four channels.");
	static_assert(RgbwwGrbww::ChannelCount == 5, "GRBWW must have five channels.");
	static_assert(sizeof(Grb::color_t) == Grb::ChannelCount, "GRB buffer must contain only its wire channels.");
	static_assert(sizeof(RgbwGrbw::color_t) == RgbwGrbw::ChannelCount, "GRBW buffer must contain only its wire channels.");
	static_assert(sizeof(RgbwwGrbww::color_t) == RgbwwGrbww::ChannelCount, "GRBWW buffer must contain only its wire channels.");

	inline bool Expect(const bool condition, const __FlashStringHelper* name)
	{
		Serial.print(name);
		Serial.println(condition ? F(": PASSED") : F(": FAILED"));
		return condition;
	}

	template<typename StripColorModelType>
	inline bool ExerciseStartedDriver(Light::ILightColorDriver<StripColorModelType>& driver,
		const typename StripColorModelType::color_t color, const uint16_t invalidIndex,
		const __FlashStringHelper* name)
	{
		Serial.println(name);
		bool pass = true;
		pass &= Expect(!driver.SetLightColor(0, color), F("Set color before Start"));
		pass &= Expect(!driver.Push(), F("Push before Start"));

		const bool started = driver.Start();
		pass &= Expect(started, F("Start"));
		if (started)
		{
			pass &= Expect(driver.Start(), F("Repeated Start"));
			pass &= Expect(driver.SetLightColor(0, color), F("Set valid color index"));
			pass &= Expect(!driver.SetLightColor(invalidIndex, color), F("Reject out-of-range color index"));
			pass &= Expect(driver.Push(), F("Push frame"));
		}

		driver.Stop();
		pass &= Expect(!driver.SetLightColor(0, color), F("Reject color after Stop"));
		pass &= Expect(!driver.Push(), F("Reject Push after Stop"));
		return pass;
	}

	template<typename RgbBitBangDriver, typename RgbwBitBangDriver,
		typename RgbSpi2MhzDriver, typename RgbwSpi2MhzDriver>
	inline bool RunAvrSk6812DriverTests(RgbBitBangDriver& bitBangRgb,
		RgbwBitBangDriver& bitBangRgbw, RgbSpi2MhzDriver& spi2MhzRgb,
		RgbwSpi2MhzDriver& spi2MhzRgbw)
	{
		bool pass = true;
		pass &= ExerciseStartedDriver<Grb>(bitBangRgb, Grb::color_t{ 0x34, 0x12, 0x56 }, 5,
			F("SK6812 AVR bitbang RGB"));
		pass &= ExerciseStartedDriver<RgbwGrbw>(bitBangRgbw, RgbwGrbw::color_t{ 0x34, 0x12, 0x56, 0x78 }, 1,
			F("SK6812 AVR bitbang RGBW"));
		pass &= ExerciseStartedDriver<Grb>(spi2MhzRgb, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("SK6812 AVR SPI 2 MHz RGB"));
		pass &= ExerciseStartedDriver<RgbwGrbw>(spi2MhzRgbw, RgbwGrbw::color_t{ 0x34, 0x12, 0x56, 0x78 }, 1,
			F("SK6812 AVR SPI 2 MHz RGBW"));
		return pass;
	}

#if F_CPU == 16000000UL
	template<typename RgbSpi8MhzDriver, typename RgbwSpi8MhzDriver>
	inline bool RunAvrSk6812Spi8MhzTests(RgbSpi8MhzDriver& spi8MhzRgb,
		RgbwSpi8MhzDriver& spi8MhzRgbw)
	{
		bool pass = true;
		pass &= ExerciseStartedDriver<Grb>(spi8MhzRgb, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("SK6812 AVR SPI 8 MHz RGB"));
		pass &= ExerciseStartedDriver<RgbwGrbw>(spi8MhzRgbw, RgbwGrbw::color_t{ 0x34, 0x12, 0x56, 0x78 }, 1,
			F("SK6812 AVR SPI 8 MHz RGBW"));
		return pass;
	}
#endif

	template<typename SpiDriver, typename BitBangDriver>
	inline bool RunAvrWs2812bDriverTests(SpiDriver& explicitSpiDriver,
		SpiDriver& automaticSpiDriver, BitBangDriver& bitBangDriver)
	{
		bool pass = true;
		pass &= ExerciseStartedDriver<Grb>(explicitSpiDriver, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("WS2812B AVR explicit SPI clock"));
		pass &= ExerciseStartedDriver<Grb>(automaticSpiDriver, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("WS2812B AVR automatic SPI clock"));
		pass &= ExerciseStartedDriver<Grb>(bitBangDriver, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("WS2812B AVR bitbang"));
		return pass;
	}

	template<typename PioDriver>
	inline bool RunRp2040Ws2812bDriverTests(PioDriver& pioDriver)
	{
		return ExerciseStartedDriver<Grb>(pioDriver, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("WS2812B RP2040 PIO"));
	}

	template<typename RgbBitBangDriver, typename RgbwBitBangDriver>
	inline bool RunStm32Sk6812BitBangDriverTests(RgbBitBangDriver& bitBangRgb,
		RgbwBitBangDriver& bitBangRgbw)
	{
		bool pass = true;
		pass &= ExerciseStartedDriver<Grb>(bitBangRgb, Grb::color_t{ 0x34, 0x12, 0x56 }, 1,
			F("SK6812 STM32 bitbang RGB"));
		pass &= ExerciseStartedDriver<RgbwGrbw>(bitBangRgbw, RgbwGrbw::color_t{ 0x34, 0x12, 0x56, 0x78 }, 1,
			F("SK6812 STM32 bitbang RGBW"));
		return pass;
	}

	template<typename Driver, typename Color>
	inline bool ExerciseStm32DmaDriver(Driver& driver, const Color color,
		const __FlashStringHelper* name)
	{
		Serial.println(name);
		bool pass = true;
		pass &= Expect(driver.IsReady(), F("DMA ready before Start"));
		pass &= Expect(driver.Start(), F("Start"));
		pass &= Expect(driver.SetLightColor(0, color), F("Set valid color index"));
		pass &= Expect(!driver.SetLightColor(1, color), F("Reject out-of-range color index"));
		pass &= Expect(driver.Push(), F("Start DMA frame"));
		pass &= Expect(!driver.IsReady(), F("DMA reports transfer active"));
		delayMicroseconds(100);
		driver.HandleTransmitComplete();
		pass &= Expect(driver.IsReady(), F("DMA completion restores ready state"));
		driver.Stop();
		return pass;
	}

	template<typename RgbDmaDriver, typename RgbwDmaDriver>
	inline bool RunStm32Sk6812DmaDriverTests(RgbDmaDriver& dmaRgb, RgbwDmaDriver& dmaRgbw)
	{
		bool pass = true;
		pass &= ExerciseStm32DmaDriver(dmaRgb, Grb::color_t{ 0x34, 0x12, 0x56 },
			F("SK6812 STM32 DMA RGB"));
		pass &= ExerciseStm32DmaDriver(dmaRgbw, RgbwGrbw::color_t{ 0x34, 0x12, 0x56, 0x78 },
			F("SK6812 STM32 DMA RGBW"));
		return pass;
	}

	inline bool RunColorPreparationTests()
	{
		const Grb::color_t rgb888Grb = Grb::Convert<Rgb888>(0x00123456);
		const Grb::color_t rgb10Grb = Grb::Convert<Rgb10>(IntegerSignal::Rgb10::Color(512, 256, 0));
		const Grb::color_t rgbFraction16Grb = Grb::Convert<RgbFraction16>(RgbFraction16::color_t{ 16384, 8192, 0 });
		const RgbwGrbw::color_t rgbwGray = RgbwGrbw::Convert<RgbFraction16>(RgbFraction16::color_t{ 16384, 16384, 16384 });
		const RgbwwGrbww::color_t rgbwwGray = RgbwwGrbww::Convert<Rgb10>(IntegerSignal::Rgb10::Color(512, 512, 512));
		const Rgb888::color_t sourceFrame[] = { 0x00123456, 0x006789AB };
		Grb::color_t preparedFrame[2] = {};
		Grb::Convert<Rgb888>(sourceFrame, preparedFrame, 2);
		TestStripDriver stripDriver;
		const bool staged = stripDriver.SetLightColor(0, preparedFrame[0]);
		const bool pushed = stripDriver.Push();

		return Expect(rgb888Grb.Channels[0] == 0x34 && rgb888Grb.Channels[1] == 0x12 && rgb888Grb.Channels[2] == 0x56, F("RGB888 GRB preparation"))
			&& Expect(preparedFrame[0].Channels[0] == 0x34 && preparedFrame[1].Channels[0] == 0x89, F("Frame conversion"))
			&& Expect(staged && pushed && stripDriver.Pushed && stripDriver.Colors[0].Channels[0] == 0x34, F("Stage native color and push"))
			&& Expect(rgb10Grb.Channels[0] == 64 && rgb10Grb.Channels[1] == 128 && rgb10Grb.Channels[2] == 0, F("RGB10 GRB preparation"))
			&& Expect(rgbFraction16Grb.Channels[0] == 64 && rgbFraction16Grb.Channels[1] == 128 && rgbFraction16Grb.Channels[2] == 0, F("RGBFraction16 GRB preparation"))
			&& Expect(rgbwGray.Channels[0] == 0 && rgbwGray.Channels[1] == 0 && rgbwGray.Channels[2] == 0 && rgbwGray.Channels[3] == 128, F("RGBW preparation"))
			&& Expect(rgbwwGray.Channels[0] == 0 && rgbwwGray.Channels[1] == 0 && rgbwwGray.Channels[2] == 0
				&& rgbwwGray.Channels[3] == 64 && rgbwwGray.Channels[4] == 64, F("RGBWW preparation"));
	}

}

#endif
