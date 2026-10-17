#ifndef _INERTIA_COMPONENTS_LIGHT_STRIP_MODEL_h
#define _INERTIA_COMPONENTS_LIGHT_STRIP_MODEL_h

#include "../Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Light
		{
			namespace Strip
			{
				namespace Model
				{
					namespace Color
					{
						enum class Channel : uint8_t
						{
							Red,
							Green,
							Blue,
							White,
							WarmWhite,
							CoolWhite
						};

						enum class WhiteMode : uint8_t
						{
							None,
							Extract,
							Split
						};

						template<Channel TTarget, Channel... TChannels>
						struct ChannelCounter;

						template<Channel TTarget>
						struct ChannelCounter<TTarget>
						{
							enum { Value = 0 };
						};

						template<Channel TTarget, Channel TFirst, Channel... TRest>
						struct ChannelCounter<TTarget, TFirst, TRest...>
						{
							enum { Value = (TTarget == TFirst ? 1 : 0) + ChannelCounter<TTarget, TRest...>::Value };
						};

						template<WhiteMode TWhiteMode, uint8_t WarmWhiteRatio, Channel... TChannels>
						struct Format
						{
							static constexpr uint8_t ChannelCount = sizeof...(TChannels);
							struct color_t
							{
								uint8_t Channels[ChannelCount];
							};

							static constexpr uint8_t RedCount = ChannelCounter<Channel::Red, TChannels...>::Value;
							static constexpr uint8_t GreenCount = ChannelCounter<Channel::Green, TChannels...>::Value;
							static constexpr uint8_t BlueCount = ChannelCounter<Channel::Blue, TChannels...>::Value;
							static constexpr uint8_t WhiteCount = ChannelCounter<Channel::White, TChannels...>::Value;
							static constexpr uint8_t WarmWhiteCount = ChannelCounter<Channel::WarmWhite, TChannels...>::Value;
							static constexpr uint8_t CoolWhiteCount = ChannelCounter<Channel::CoolWhite, TChannels...>::Value;

							static_assert(TWhiteMode == WhiteMode::None
								? ChannelCount == 3 && RedCount == 1 && GreenCount == 1 && BlueCount == 1
								&& WhiteCount == 0 && WarmWhiteCount == 0 && CoolWhiteCount == 0
								: TWhiteMode == WhiteMode::Extract
								? ChannelCount == 4 && RedCount == 1 && GreenCount == 1 && BlueCount == 1
								&& WhiteCount == 1 && WarmWhiteCount == 0 && CoolWhiteCount == 0
								: ChannelCount == 5 && RedCount == 1 && GreenCount == 1 && BlueCount == 1
								&& WhiteCount == 0 && WarmWhiteCount == 1 && CoolWhiteCount == 1,
								"Strip color format must specify exactly one of each channel required by its white mode.");

							template<typename ColorModelType>
							static void Encode(const typename ColorModelType::color_t color, uint8_t* destination)
							{
								using component_t = typename ColorModelType::component_t;
								const component_t red = ColorModelType::Red(color);
								const component_t green = ColorModelType::Green(color);
								const component_t blue = ColorModelType::Blue(color);
								const component_t white = TWhiteMode == WhiteMode::None
									? component_t(0)
									: (red < green ? (red < blue ? red : blue) : (green < blue ? green : blue));
								const component_t outputRed = TWhiteMode == WhiteMode::None ? red : red - white;
								const component_t outputGreen = TWhiteMode == WhiteMode::None ? green : green - white;
								const component_t outputBlue = TWhiteMode == WhiteMode::None ? blue : blue - white;
								const component_t warmWhite = static_cast<component_t>((static_cast<uint64_t>(white) * WarmWhiteRatio) / UINT8_MAX);
								const component_t coolWhite = white - warmWhite;
								WriteChannels<ColorModelType, TChannels...>(destination, 0,
									outputRed, outputGreen, outputBlue, white, warmWhite, coolWhite);
							}

							template<typename ColorModelType>
							static color_t Convert(const typename ColorModelType::color_t color)
							{
								color_t result = {};
								Encode<ColorModelType>(color, result.Channels);
								return result;
							}

							template<typename ColorModelType>
							static void Convert(
								const typename ColorModelType::color_t* source,
								color_t* destination,
								const uint16_t count)
							{
								for (uint16_t index = 0; index < count; ++index)
								{
									destination[index] = Convert<ColorModelType>(source[index]);
								}
							}

						private:
							template<typename ColorModelType>
							static void WriteChannels(uint8_t*, const uint8_t,
								const typename ColorModelType::component_t,
								const typename ColorModelType::component_t,
								const typename ColorModelType::component_t,
								const typename ColorModelType::component_t,
								const typename ColorModelType::component_t,
								const typename ColorModelType::component_t)
							{}

							template<typename ColorModelType, Channel TChannel, Channel... TRest>
							static void WriteChannels(uint8_t* destination, const uint8_t index,
								const typename ColorModelType::component_t red,
								const typename ColorModelType::component_t green,
								const typename ColorModelType::component_t blue,
								const typename ColorModelType::component_t white,
								const typename ColorModelType::component_t warmWhite,
								const typename ColorModelType::component_t coolWhite)
							{
								destination[index] = GetChannel<ColorModelType, TChannel>(red, green, blue, white, warmWhite, coolWhite);
								WriteChannels<ColorModelType, TRest...>(destination, index + 1,
									red, green, blue, white, warmWhite, coolWhite);
							}

							template<typename ColorModelType, Channel TChannel>
							static uint8_t GetChannel(
								const typename ColorModelType::component_t red,
								const typename ColorModelType::component_t green,
								const typename ColorModelType::component_t blue,
								const typename ColorModelType::component_t white,
								const typename ColorModelType::component_t warmWhite,
								const typename ColorModelType::component_t coolWhite)
							{
								switch (TChannel)
								{
								case Channel::Red: return ToByte<ColorModelType>(red);
								case Channel::Green: return ToByte<ColorModelType>(green);
								case Channel::Blue: return ToByte<ColorModelType>(blue);
								case Channel::White: return ToByte<ColorModelType>(white);
								case Channel::WarmWhite: return ToByte<ColorModelType>(warmWhite);
								default: return ToByte<ColorModelType>(coolWhite);
								}
							}

							template<typename ColorModelType>
							static uint8_t ToByte(const typename ColorModelType::component_t value)
							{
								return static_cast<uint8_t>(
									(static_cast<uint64_t>(value) * UINT8_MAX + ColorModelType::COMPONENT_MAX / 2)
									/ ColorModelType::COMPONENT_MAX);
							}
						};

						template<Channel... TChannels>
						using Rgb = Format<WhiteMode::None, 0, TChannels...>;

						template<Channel... TChannels>
						using Rgbw = Format<WhiteMode::Extract, 0, TChannels...>;

						template<uint8_t WarmWhiteRatio, Channel... TChannels>
						using Rgbww = Format<WhiteMode::Split, WarmWhiteRatio, TChannels...>;

						using Grb = Rgb<Channel::Green, Channel::Red, Channel::Blue>;
						using Rbg = Rgb<Channel::Red, Channel::Blue, Channel::Green>;
						using Gbr = Rgb<Channel::Green, Channel::Blue, Channel::Red>;
						using Brg = Rgb<Channel::Blue, Channel::Red, Channel::Green>;
						using Bgr = Rgb<Channel::Blue, Channel::Green, Channel::Red>;
						using Grbw = Rgbw<Channel::Green, Channel::Red, Channel::Blue, Channel::White>;
						using Wrgb = Rgbw<Channel::White, Channel::Red, Channel::Green, Channel::Blue>;
						using Wgrb = Rgbw<Channel::White, Channel::Green, Channel::Red, Channel::Blue>;

						template<uint8_t WarmWhiteRatio>
						using Grbww = Rgbww<WarmWhiteRatio,
							Channel::Green, Channel::Red, Channel::Blue, Channel::WarmWhite, Channel::CoolWhite>;
					}
				}
			}
		}
	}
}

#endif