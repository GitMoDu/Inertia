#ifndef _INERTIA_COMPONENTS_SURFACE_VALUE_PACK_h
#define _INERTIA_COMPONENTS_SURFACE_VALUE_PACK_h

#include "Model.h"
#include "../../../../IntegerSignal/src/FixedPoint/FactorScale.h"

namespace Inertia
{
	namespace Components
	{
		namespace Surface
		{
			namespace Abstract
			{
				/// <summary>
				/// Packs 4 12-bit values into 6 bytes.
				/// Variants for unsigned and signed values are provided.
				/// Unsigned values have a range of 0 to 4095.
				/// Signed values are stored in two's complement form, with a range of -2048 to 2047.
				/// </summary>
				class BlockPack4x12_6
				{
				public:
					static constexpr uint16_t RawMax = 0x0FFFu;

				public:
					uint8_t Data[6]{};

				public:
					BlockPack4x12_6() = default;

				protected:
					static constexpr int16_t SignedMin = -2048;
					static constexpr int16_t SignedMax = 2047;

					template<uint8_t ValueIndex>
					uint16_t GetRaw() const
					{
						static_assert(ValueIndex < 4, "ValueIndex must be between 0 and 3.");

						constexpr uint8_t byteIndex = (ValueIndex * 3) / 2;

						if constexpr ((ValueIndex & 1) == 0)
						{
							return (static_cast<uint16_t>(Data[byteIndex]) << 4)
								| static_cast<uint16_t>(Data[byteIndex + 1] >> 4);
						}
						else
						{
							return (static_cast<uint16_t>(Data[byteIndex] & 0x0F) << 8)
								| static_cast<uint16_t>(Data[byteIndex + 1]);
						}
					}

					template<uint8_t ValueIndex>
					void SetRaw(const uint16_t value)
					{
						static_assert(ValueIndex < 4, "ValueIndex must be between 0 and 3.");

						constexpr uint8_t byteIndex = (ValueIndex * 3) / 2;
						const uint16_t packed = IntegerSignal::LimitValue<uint16_t, 0, RawMax>(value);

						if constexpr ((ValueIndex & 1) == 0)
						{
							Data[byteIndex] = static_cast<uint8_t>(packed >> 4);
							Data[byteIndex + 1] = static_cast<uint8_t>(
								(Data[byteIndex + 1] & 0x0F)
								| ((packed & 0x000F) << 4));
						}
						else
						{
							Data[byteIndex] = static_cast<uint8_t>(
								(Data[byteIndex] & 0xF0)
								| ((packed >> 8) & 0x0F));
							Data[byteIndex + 1] = static_cast<uint8_t>(packed & 0x00FF);
						}
					}

					template<uint8_t ValueIndex>
					uint16_t GetUnsigned() const
					{
						return this->template GetRaw<ValueIndex>();
					}

					template<uint8_t ValueIndex>
					int16_t GetSigned() const
					{
						return SignExtend12(this->template GetRaw<ValueIndex>());
					}

					template<uint8_t ValueIndex>
					void SetUnsigned(const uint16_t value)
					{
						this->template SetRaw<ValueIndex>(value);
					}

					template<uint8_t ValueIndex>
					void SetSigned(const int16_t value)
					{
						SetUnsigned<ValueIndex>(PackSigned12(value));
					}

				private:
					static int16_t SignExtend12(const uint16_t value)
					{
						return (value & 0x0800u) != 0
							? static_cast<int16_t>(value | 0xF000u)
							: static_cast<int16_t>(value);
					}

					static uint16_t PackSigned12(const int16_t value)
					{
						return static_cast<uint16_t>(IntegerSignal::LimitValue<int16_t, SignedMin, SignedMax>(value)) & RawMax;
					}
				};

				/// <summary>
				/// Packs 3 21-bit values into 8 bytes, leaving the top bit unused.
				/// Variants for unsigned and signed values are provided.
				/// Unsigned values have a range of 0 to 2097151.
				/// Signed values are stored in two's complement form, with a range of -1048576 to 1048575.
				/// </summary>
				class BlockPack3x21_8
				{
				public:
					static constexpr uint32_t RawMax = 0x001FFFFFu;

				public:
					uint8_t Data[8]{};

				public:
					BlockPack3x21_8() = default;

				protected:
					static constexpr int32_t SignedMin = -1048576;
					static constexpr int32_t SignedMax = 1048575;

					template<uint8_t ValueIndex>
					uint32_t GetRaw() const
					{
						static_assert(ValueIndex < 3, "ValueIndex must be between 0 and 2.");

						constexpr uint8_t BitOffset = ValueIndex * 21;
						return static_cast<uint32_t>((GetPacked() >> BitOffset) & 0x1FFFFFull);
					}

					template<uint8_t ValueIndex>
					void SetRaw(const uint32_t value)
					{
						static_assert(ValueIndex < 3, "ValueIndex must be between 0 and 2.");

						constexpr uint8_t BitOffset = ValueIndex * 21;
						uint64_t packed = GetPacked();

						packed &= ~(0x1FFFFFull << BitOffset);
						packed |= (static_cast<uint64_t>(IntegerSignal::LimitValue<uint32_t, 0, RawMax>(value)) << BitOffset);

						SetPacked(packed);
					}

					template<uint8_t ValueIndex>
					uint32_t GetUnsigned() const
					{
						return this->template GetRaw<ValueIndex>();
					}

					template<uint8_t ValueIndex>
					int32_t GetSigned() const
					{
						return SignExtend21(this->template GetRaw<ValueIndex>());
					}

					template<uint8_t ValueIndex>
					void SetUnsigned(const uint32_t value)
					{
						this->template SetRaw<ValueIndex>(value);
					}

					template<uint8_t ValueIndex>
					void SetSigned(const int32_t value)
					{
						SetUnsigned<ValueIndex>(PackSigned21(value));
					}

				private:
					uint64_t GetPacked() const
					{
						uint64_t packed = 0;

						for (uint8_t i = 0; i < sizeof(Data); i++)
						{
							packed |= static_cast<uint64_t>(Data[i]) << (i * 8);
						}

						return packed;
					}

					void SetPacked(const uint64_t packed)
					{
						for (uint8_t i = 0; i < sizeof(Data); i++)
						{
							Data[i] = static_cast<uint8_t>((packed >> (i * 8)) & 0xFFu);
						}

						Data[7] &= 0x7F;
					}

					static int32_t SignExtend21(const uint32_t value)
					{
						return (value & 0x00100000u) != 0
							? static_cast<int32_t>(value | 0xFFE00000u)
							: static_cast<int32_t>(value);
					}

					static uint32_t PackSigned21(const int32_t value)
					{
						return static_cast<uint32_t>(IntegerSignal::LimitValue<int32_t, SignedMin, SignedMax>(value)) & RawMax;
					}
				};
			}

			namespace Template
			{
				template<typename raw_t, raw_t RawMaxValue, typename value_t, value_t MinValue, value_t MaxValue>
				struct LimitedValueCodec
				{
					using packed_t = raw_t;
					using unpacked_t = value_t;

					static_assert(MaxValue >= MinValue, "MaxValue must be greater than or equal to MinValue.");

					static unpacked_t Decode(const packed_t value)
					{
						const packed_t limited = IntegerSignal::LimitValue<packed_t, 0, RawMaxValue>(value);

						if constexpr (MinValue < static_cast<unpacked_t>(0))
						{
							return SignExtend(limited);
						}
						else
						{
							return static_cast<unpacked_t>(limited);
						}
					}

					static packed_t Encode(const unpacked_t value)
					{
						const unpacked_t limited = IntegerSignal::LimitValue<unpacked_t, MinValue, MaxValue>(value);

						if constexpr (MinValue < static_cast<unpacked_t>(0))
						{
							return static_cast<packed_t>(static_cast<packed_t>(limited) & RawMaxValue);
						}
						else
						{
							return IntegerSignal::LimitValue<packed_t, 0, RawMaxValue>(static_cast<packed_t>(limited));
						}
					}

				private:
					static unpacked_t SignExtend(const packed_t value)
					{
						constexpr packed_t SignBit = static_cast<packed_t>(RawMaxValue ^ (RawMaxValue >> 1));
						constexpr packed_t ExtendMask = static_cast<packed_t>(~RawMaxValue);

						return (value & SignBit) != 0
							? static_cast<unpacked_t>(value | ExtendMask)
							: static_cast<unpacked_t>(value);
					}
				};

				template<typename raw_t, raw_t RawMaxValue, typename value_t, value_t MinValue, value_t MaxValue>
				struct MaskedValueCodec
				{
					using packed_t = raw_t;
					using unpacked_t = value_t;

					static_assert(MaxValue >= MinValue, "MaxValue must be greater than or equal to MinValue.");

					static unpacked_t Decode(const packed_t value)
					{
						const packed_t masked = static_cast<packed_t>(value & RawMaxValue);

						if constexpr (MinValue < static_cast<unpacked_t>(0))
						{
							return SignExtend(masked);
						}
						else
						{
							return static_cast<unpacked_t>(masked);
						}
					}

					static packed_t Encode(const unpacked_t value)
					{
						return static_cast<packed_t>(static_cast<packed_t>(value) & RawMaxValue);
					}

				private:
					static unpacked_t SignExtend(const packed_t value)
					{
						constexpr packed_t SignBit = static_cast<packed_t>(RawMaxValue ^ (RawMaxValue >> 1));
						constexpr packed_t ExtendMask = static_cast<packed_t>(~RawMaxValue);

						return (value & SignBit) != 0
							? static_cast<unpacked_t>(value | ExtendMask)
							: static_cast<unpacked_t>(value);
					}
				};

				template<typename raw_t, raw_t RawMaxValue, typename value_t, value_t MinValue, value_t MaxValue>
				struct ScaledValueCodec
				{
					using packed_t = raw_t;
					using unpacked_t = value_t;
					using scale_t = IntegerSignal::FixedPoint::FactorScale::scale32_t;
					using range_t = uint32_t;

					static_assert(MaxValue > MinValue, "Scaled value packs require MaxValue > MinValue.");

					static constexpr range_t ValueRange = static_cast<range_t>(MaxValue - MinValue);
					static constexpr scale_t CompressScale = IntegerSignal::FixedPoint::FactorScale::GetFactor32<range_t>(
						static_cast<range_t>(RawMaxValue),
						ValueRange);
					static constexpr scale_t DecompressScale = IntegerSignal::FixedPoint::FactorScale::GetFactor32<range_t>(
						ValueRange,
						static_cast<range_t>(RawMaxValue));

					static unpacked_t Decode(const packed_t value)
					{
						const packed_t limited = IntegerSignal::LimitValue<packed_t, 0, RawMaxValue>(value);
						return static_cast<unpacked_t>(
							MinValue
							+ static_cast<unpacked_t>(IntegerSignal::FixedPoint::FactorScale::Scale(
								DecompressScale,
								static_cast<range_t>(limited))));
					}

					static packed_t Encode(const unpacked_t value)
					{
						const unpacked_t limited = IntegerSignal::LimitValue<unpacked_t, MinValue, MaxValue>(value);
						const range_t normalized = static_cast<range_t>(limited - MinValue);

						return IntegerSignal::LimitValue<packed_t, 0, RawMaxValue>(
							static_cast<packed_t>(IntegerSignal::FixedPoint::FactorScale::Scale(
								CompressScale,
								normalized)));
					}
				};
			}

			struct ValuePack4x12_6 : Abstract::BlockPack4x12_6
			{
			protected:
				template<uint8_t ValueIndex>
				uint16_t GetUnsignedLimitedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, 0, Abstract::BlockPack4x12_6::RawMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetUnsignedLimitedValue(const uint16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, 0, Abstract::BlockPack4x12_6::RawMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				int16_t GetSignedLimitedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, Abstract::BlockPack4x12_6::SignedMin, Abstract::BlockPack4x12_6::SignedMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetSignedLimitedValue(const int16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, Abstract::BlockPack4x12_6::SignedMin, Abstract::BlockPack4x12_6::SignedMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				uint16_t GetUnsignedMaskedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, 0, Abstract::BlockPack4x12_6::RawMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetUnsignedMaskedValue(const uint16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, 0, Abstract::BlockPack4x12_6::RawMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				int16_t GetSignedMaskedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, Abstract::BlockPack4x12_6::SignedMin, Abstract::BlockPack4x12_6::SignedMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetSignedMaskedValue(const int16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, Abstract::BlockPack4x12_6::SignedMin, Abstract::BlockPack4x12_6::SignedMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex, uint16_t MinValue, uint16_t MaxValue>
				uint16_t GetUnsignedScaledValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, MinValue, MaxValue>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex, uint16_t MinValue, uint16_t MaxValue>
				void SetUnsignedScaledValue(const uint16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, uint16_t, MinValue, MaxValue>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex, int16_t MinValue, int16_t MaxValue>
				int16_t GetSignedScaledValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, MinValue, MaxValue>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex, int16_t MinValue, int16_t MaxValue>
				void SetSignedScaledValue(const int16_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, Abstract::BlockPack4x12_6::RawMax, int16_t, MinValue, MaxValue>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}
			};

			struct ValuePack3x21_8 : Abstract::BlockPack3x21_8
			{
			protected:
				template<uint8_t ValueIndex>
				uint32_t GetUnsignedLimitedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, 0, Abstract::BlockPack3x21_8::RawMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetUnsignedLimitedValue(const uint32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, 0, Abstract::BlockPack3x21_8::RawMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				int32_t GetSignedLimitedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, Abstract::BlockPack3x21_8::SignedMin, Abstract::BlockPack3x21_8::SignedMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetSignedLimitedValue(const int32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::LimitedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, Abstract::BlockPack3x21_8::SignedMin, Abstract::BlockPack3x21_8::SignedMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				uint32_t GetUnsignedMaskedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, 0, Abstract::BlockPack3x21_8::RawMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetUnsignedMaskedValue(const uint32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, 0, Abstract::BlockPack3x21_8::RawMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex>
				int32_t GetSignedMaskedValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, Abstract::BlockPack3x21_8::SignedMin, Abstract::BlockPack3x21_8::SignedMax>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex>
				void SetSignedMaskedValue(const int32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::MaskedValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, Abstract::BlockPack3x21_8::SignedMin, Abstract::BlockPack3x21_8::SignedMax>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex, uint32_t MinValue, uint32_t MaxValue>
				uint32_t GetUnsignedScaledValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, MinValue, MaxValue>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex, uint32_t MinValue, uint32_t MaxValue>
				void SetUnsignedScaledValue(const uint32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, uint32_t, MinValue, MaxValue>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}

				template<uint8_t ValueIndex, int32_t MinValue, int32_t MaxValue>
				int32_t GetSignedScaledValue() const
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, MinValue, MaxValue>;
					return codec_t::Decode(this->template GetRaw<ValueIndex>());
				}

				template<uint8_t ValueIndex, int32_t MinValue, int32_t MaxValue>
				void SetSignedScaledValue(const int32_t value)
				{
					using codec_t = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, Abstract::BlockPack3x21_8::RawMax, int32_t, MinValue, MaxValue>;
					this->template SetRaw<ValueIndex>(codec_t::Encode(value));
				}
			};

		}
	}
}

#endif