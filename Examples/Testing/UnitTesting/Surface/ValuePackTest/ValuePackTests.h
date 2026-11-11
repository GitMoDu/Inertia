#ifndef VALUE_PACK_TESTS_H
#define VALUE_PACK_TESTS_H

#include <InertiaModel.h>
#include <stdint.h>

namespace ValuePackTests
{
	using Signed8FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 0xFFu,
		int8_t, INT8_MIN, INT8_MAX>;
	using Unsigned8FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, UINT8_MAX,
		uint8_t, 0, UINT8_MAX>;
	using Unsigned8Range = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 0x0Fu,
		uint8_t, 10, 200>;
	using Signed8Asymmetric = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 0x0Fu,
		int8_t, -5, 20>;
	using Signed8Negative = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 0x0Fu,
		int8_t, -20, -3>;
	using Signed8EncodeTie = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 3u,
		int8_t, 0, 2>;
	using Signed8DecodeTie = Inertia::Components::Surface::Template::ScaledValueCodec<uint8_t, 4u,
		int8_t, 0, 3>;
	using Signed16FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, UINT16_MAX,
		int16_t, INT16_MIN, INT16_MAX>;
	using Unsigned16FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, UINT16_MAX,
		uint16_t, 0, UINT16_MAX>;
	using Signed16Asymmetric = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, 0x0FFFu,
		int16_t, -30000, 12345>;
	using Unsigned16Range = Inertia::Components::Surface::Template::ScaledValueCodec<uint16_t, 0x0FFFu,
		uint16_t, 10, 4000>;
	using Signed32CompactRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, 0x001FFFFFu,
		int32_t, INT32_MIN, INT32_MAX>;
	using Signed32FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, UINT32_MAX,
		int32_t, INT32_MIN, INT32_MAX>;
	using Unsigned32FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<uint32_t, UINT32_MAX,
		uint32_t, 0, UINT32_MAX>;
	using SignedPacked8FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<int8_t, INT8_MAX,
		int8_t, INT8_MIN, INT8_MAX>;
	using SignedPacked16FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<int16_t, INT16_MAX,
		int16_t, INT16_MIN, INT16_MAX>;
	using SignedPacked32FullRange = Inertia::Components::Surface::Template::ScaledValueCodec<int32_t, INT32_MAX,
		int32_t, INT32_MIN, INT32_MAX>;
	using Signed4BitLimited = Inertia::Components::Surface::Template::LimitedValueCodec<uint8_t, 0x0Fu,
		int8_t, -8, 7>;
	using Signed4BitMasked = Inertia::Components::Surface::Template::MaskedValueCodec<uint8_t, 0x0Fu,
		int8_t, -8, 7>;

	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<Signed8FullRange::range_t, uint8_t>::value,
		"8-bit codecs should use 8-bit range storage");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<Signed8FullRange::scale_t, uint16_t>::value,
		"8-bit codecs should use 16-bit scale intermediates");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<Unsigned16Range::scale_t, uint32_t>::value,
		"16-bit codecs should use 32-bit scale intermediates");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<Signed32FullRange::scale_t, uint64_t>::value,
		"32-bit codecs should use 64-bit scale intermediates");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<SignedPacked32FullRange::range_t, uint32_t>::value,
		"signed packed ranges should use unsigned range intermediates");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<SignedPacked8FullRange::range_t, uint8_t>::value,
		"signed 8-bit packed ranges should use unsigned range intermediates");
	static_assert(IntegerSignal::TypeTraits::TypeDispatch::is_same<SignedPacked16FullRange::range_t, uint16_t>::value,
		"signed 16-bit packed ranges should use unsigned range intermediates");
	static_assert(Signed32FullRange::ValueRange == UINT32_MAX, "int32 full range should fit uint32_t");
	static_assert(Unsigned32FullRange::ValueRange == UINT32_MAX, "uint32 full range should fit uint32_t");
	static_assert(SignedPacked32FullRange::ValueRange == UINT32_MAX, "signed-packed int32 range should fit uint32_t");

	static_assert(Inertia::Components::Surface::Mask<64>() == UINT64_MAX, "64-bit mask should cover its full width");
	static_assert(Inertia::Components::Surface::Mask<uint8_t, 8>() == UINT8_MAX, "typed 8-bit mask should cover its full width");
	static_assert(Inertia::Components::Surface::Mask<uint16_t, 16>() == UINT16_MAX, "typed 16-bit mask should cover its full width");
	static_assert(Inertia::Components::Surface::Mask<uint32_t, 32>() == UINT32_MAX, "typed 32-bit mask should cover its full width");
	static_assert(Inertia::Components::Surface::Mask<uint64_t, 64>() == UINT64_MAX, "typed 64-bit mask should cover its full width");
	static_assert(Inertia::Components::Surface::Mask<int32_t, 32>() == -1, "typed signed mask should preserve all bits");

	inline bool Expect(const bool condition, const char*)
	{
		return condition;
	}

	template<typename codec_t, typename value_t, value_t MinValue, typename packed_t, packed_t RawMaxValue>
	inline bool CheckEncodeQuantization(const value_t value)
	{
		const uint64_t valueRange = codec_t::ValueRange;
		const uint64_t rawRange = static_cast<uint64_t>(static_cast<int64_t>(RawMaxValue));
		const int64_t normalizedSigned = static_cast<int64_t>(value) - static_cast<int64_t>(MinValue);
		const int64_t encodedSigned = static_cast<int64_t>(codec_t::Encode(value));
		if (normalizedSigned < 0 || static_cast<uint64_t>(normalizedSigned) > valueRange ||
			encodedSigned < 0 || static_cast<uint64_t>(encodedSigned) > rawRange)
			return false;

		const uint64_t normalized = static_cast<uint64_t>(normalizedSigned);
		const uint64_t mapped = static_cast<uint64_t>(encodedSigned) * valueRange;
		const uint64_t exact = normalized * rawRange;
		const uint64_t error = mapped > exact ? mapped - exact : exact - mapped;
		return error <= valueRange / 2u;
	}

	template<typename codec_t, typename value_t, value_t MinValue, typename packed_t, packed_t RawMaxValue>
	inline bool CheckDecodeQuantization(const packed_t value)
	{
		const uint64_t valueRange = codec_t::ValueRange;
		const uint64_t rawRange = static_cast<uint64_t>(static_cast<int64_t>(RawMaxValue));
		const int64_t rawSigned = static_cast<int64_t>(value);
		const int64_t decodedOffset = static_cast<int64_t>(codec_t::Decode(value)) - static_cast<int64_t>(MinValue);
		if (rawSigned < 0 || static_cast<uint64_t>(rawSigned) > rawRange || decodedOffset < 0 ||
			static_cast<uint64_t>(decodedOffset) > valueRange)
			return false;

		const uint64_t raw = static_cast<uint64_t>(rawSigned);
		const uint64_t mapped = static_cast<uint64_t>(decodedOffset) * rawRange;
		const uint64_t exact = raw * valueRange;
		const uint64_t error = mapped > exact ? mapped - exact : exact - mapped;
		return error <= rawRange / 2u;
	}

	template<typename codec_t, typename value_t, value_t MinValue,
		typename packed_t, packed_t RawMaxValue, uint32_t SampleCount>
	inline bool CheckQuantizationSamples()
	{
		const uint64_t valueRange = codec_t::ValueRange;
		const uint64_t rawRange = static_cast<uint64_t>(static_cast<int64_t>(RawMaxValue));
		bool pass = true;

		for (uint32_t sample = 0; sample <= SampleCount; ++sample)
		{
			const int64_t value = static_cast<int64_t>(MinValue) +
				static_cast<int64_t>((valueRange * sample) / SampleCount);
			const uint64_t raw = (rawRange * sample) / SampleCount;
			pass &= CheckEncodeQuantization<codec_t, value_t, MinValue, packed_t, RawMaxValue>(static_cast<value_t>(value));
			pass &= CheckDecodeQuantization<codec_t, value_t, MinValue, packed_t, RawMaxValue>(static_cast<packed_t>(raw));
		}

		return pass;
	}

	template<typename codec_t, typename value_t, value_t MinValue,
		typename packed_t, packed_t RawMaxValue, uint32_t SampleCount>
	inline bool CheckMonotonicSamples()
	{
		const uint64_t valueRange = codec_t::ValueRange;
		const uint64_t rawRange = static_cast<uint64_t>(static_cast<int64_t>(RawMaxValue));
		packed_t previousEncoded = codec_t::Encode(MinValue);
		value_t previousDecoded = codec_t::Decode(static_cast<packed_t>(0));

		for (uint32_t sample = 1; sample <= SampleCount; ++sample)
		{
			const value_t value = static_cast<value_t>(static_cast<int64_t>(MinValue) +
				static_cast<int64_t>((valueRange * sample) / SampleCount));
			const packed_t encoded = codec_t::Encode(value);
			const packed_t raw = static_cast<packed_t>((rawRange * sample) / SampleCount);
			const value_t decoded = codec_t::Decode(raw);
			if (encoded < previousEncoded || decoded < previousDecoded)
				return false;
			previousEncoded = encoded;
			previousDecoded = decoded;
		}

		return true;
	}

	inline bool RunSigned8FullRangeTests()
	{
		bool pass = true;
		pass &= Expect(Signed8FullRange::Encode(INT8_MIN) == 0, "int8 full-range minimum encodes to zero");
		pass &= Expect(Signed8FullRange::Encode(INT8_MAX) == 0xFFu, "int8 full-range maximum encodes to raw maximum");
		pass &= Expect(Signed8FullRange::Decode(0) == INT8_MIN, "int8 full-range zero decodes to minimum");
		pass &= Expect(Signed8FullRange::Decode(0xFFu) == INT8_MAX, "int8 full-range raw maximum decodes to maximum");

		bool exactMapping = true;
		for (int16_t value = INT8_MIN; value <= INT8_MAX; ++value)
		{
			const int8_t signedValue = static_cast<int8_t>(value);
			const uint8_t expectedRaw = static_cast<uint8_t>(value + 128);
			exactMapping &= Signed8FullRange::Encode(signedValue) == expectedRaw;
			exactMapping &= Signed8FullRange::Decode(expectedRaw) == signedValue;
		}
		pass &= Expect(exactMapping, "int8 full-range values map exactly across zero");
		pass &= Expect(CheckMonotonicSamples<Signed8FullRange, int8_t, INT8_MIN, uint8_t, UINT8_MAX, 256>(),
			"int8 full-range encode and decode are monotonic");
		pass &= Expect(CheckQuantizationSamples<Signed8FullRange, int8_t, INT8_MIN, uint8_t, UINT8_MAX, 256>(),
			"int8 full-range encode and decode stay within nearest-rounding error bounds");
		pass &= Expect(CheckQuantizationSamples<Unsigned8FullRange, uint8_t, 0, uint8_t, UINT8_MAX, 256>(),
			"uint8 full-range encode and decode stay within nearest-rounding error bounds");
		return pass;
	}

	inline bool RunAsymmetricSignedTests()
	{
		bool pass = true;
		pass &= Expect(Signed8Asymmetric::Encode(-5) == 0, "asymmetric signed minimum endpoint");
		pass &= Expect(Signed8Asymmetric::Encode(20) == 0x0Fu, "asymmetric signed maximum endpoint");
		pass &= Expect(Signed8Asymmetric::Decode(0) == -5, "asymmetric signed raw-zero decode");
		pass &= Expect(Signed8Asymmetric::Decode(0x0Fu) == 20, "asymmetric signed raw-maximum decode");
		pass &= Expect(Signed8Asymmetric::Encode(0) == 3 && Signed8Asymmetric::Decode(3) == 0,
			"asymmetric signed zero maps consistently");

		bool monotonic = true;
		uint8_t previousEncoded = 0;
		for (int16_t value = -5; value <= 20; ++value)
		{
			const uint8_t encoded = Signed8Asymmetric::Encode(static_cast<int8_t>(value));
			monotonic &= encoded >= previousEncoded;
			previousEncoded = encoded;
		}
		int8_t previousDecoded = Signed8Asymmetric::Decode(0);
		for (uint8_t raw = 1; raw <= 0x0Fu; ++raw)
		{
			const int8_t decoded = Signed8Asymmetric::Decode(raw);
			monotonic &= decoded >= previousDecoded;
			previousDecoded = decoded;
		}
		pass &= Expect(monotonic, "asymmetric signed encode and decode are monotonic");
		pass &= Expect(CheckMonotonicSamples<Signed8Asymmetric, int8_t, -5, uint8_t, 0x0Fu, 256>(),
			"asymmetric signed mappings stay monotonic across their full ranges");
		pass &= Expect(CheckQuantizationSamples<Signed8Asymmetric, int8_t, -5, uint8_t, 0x0Fu, 256>(),
			"asymmetric signed mapping meets encode and decode quantization bounds");
		return pass;
	}

	inline bool RunNegativeAndUnsignedTests()
	{
		bool pass = true;
		pass &= Expect(Signed8Negative::Encode(-20) == 0 && Signed8Negative::Decode(0) == -20,
			"all-negative minimum endpoint");
		pass &= Expect(Signed8Negative::Encode(-3) == 0x0Fu && Signed8Negative::Decode(0x0Fu) == -3,
			"all-negative maximum endpoint");
		pass &= Expect(Unsigned8FullRange::Encode(0) == 0 && Unsigned8FullRange::Encode(UINT8_MAX) == UINT8_MAX,
			"unsigned 8-bit full-range endpoints");
		pass &= Expect(Unsigned8Range::Encode(10) == 0 && Unsigned8Range::Decode(0) == 10,
			"unsigned 8-bit range minimum endpoint");
		pass &= Expect(Unsigned8Range::Encode(200) == 0x0Fu && Unsigned8Range::Decode(0x0Fu) == 200,
			"unsigned 8-bit range maximum endpoint");
		pass &= Expect(Signed16FullRange::Encode(INT16_MIN) == 0 && Signed16FullRange::Encode(INT16_MAX) == UINT16_MAX,
			"int16 full-range endpoints");
		pass &= Expect(Signed16FullRange::Decode(0) == INT16_MIN && Signed16FullRange::Decode(UINT16_MAX) == INT16_MAX,
			"int16 full-range decode endpoints");
		pass &= Expect(Unsigned16FullRange::Encode(0) == 0 && Unsigned16FullRange::Encode(UINT16_MAX) == UINT16_MAX,
			"uint16 full-range endpoints");
		pass &= Expect(Unsigned16FullRange::Decode(0) == 0 && Unsigned16FullRange::Decode(UINT16_MAX) == UINT16_MAX,
			"uint16 full-range decode endpoints");
		pass &= Expect(Unsigned16Range::Encode(10) == 0 && Unsigned16Range::Decode(0) == 10,
			"unsigned minimum endpoint");
		pass &= Expect(Unsigned16Range::Encode(4000) == 0x0FFFu && Unsigned16Range::Decode(0x0FFFu) == 4000,
			"unsigned maximum endpoint");
		pass &= Expect(CheckQuantizationSamples<Signed8Negative, int8_t, -20, uint8_t, 0x0Fu, 256>(),
			"all-negative signed mapping meets quantization bounds");
		pass &= Expect(CheckMonotonicSamples<Signed8Negative, int8_t, -20, uint8_t, 0x0Fu, 256>(),
			"all-negative signed mapping is monotonic");
		pass &= Expect(CheckQuantizationSamples<Unsigned8Range, uint8_t, 10, uint8_t, 0x0Fu, 256>(),
			"unsigned 8-bit mapping meets quantization bounds");
		pass &= Expect(CheckMonotonicSamples<Unsigned8Range, uint8_t, 10, uint8_t, 0x0Fu, 256>(),
			"unsigned 8-bit mapping is monotonic");
		pass &= Expect(CheckQuantizationSamples<Signed16FullRange, int16_t, INT16_MIN, uint16_t, UINT16_MAX, 128>(),
			"int16 full-range mapping meets quantization bounds");
		pass &= Expect(CheckQuantizationSamples<Unsigned16FullRange, uint16_t, 0, uint16_t, UINT16_MAX, 128>(),
			"uint16 full-range mapping meets quantization bounds");
		pass &= Expect(CheckQuantizationSamples<Signed16Asymmetric, int16_t, -30000, uint16_t, 0x0FFFu, 128>(),
			"asymmetric int16 mapping meets quantization bounds");
		pass &= Expect(CheckQuantizationSamples<Unsigned16Range, uint16_t, 10, uint16_t, 0x0FFFu, 128>(),
			"unsigned int16 mapping meets quantization bounds");
		pass &= Expect(CheckMonotonicSamples<Signed16FullRange, int16_t, INT16_MIN, uint16_t, UINT16_MAX, 128>(),
			"int16 full-range mapping is monotonic");
		pass &= Expect(CheckMonotonicSamples<Signed16Asymmetric, int16_t, -30000, uint16_t, 0x0FFFu, 128>(),
			"asymmetric int16 mapping is monotonic");
		pass &= Expect(CheckMonotonicSamples<Unsigned16Range, uint16_t, 10, uint16_t, 0x0FFFu, 128>(),
			"unsigned int16 mapping is monotonic");
		return pass;
	}

	inline bool RunSignedPackedRangeTests()
	{
		bool pass = true;
		pass &= Expect(SignedPacked8FullRange::Encode(INT8_MIN) == 0 && SignedPacked8FullRange::Encode(INT8_MAX) == INT8_MAX,
			"signed 8-bit packed range maps both endpoints");
		pass &= Expect(SignedPacked8FullRange::Decode(0) == INT8_MIN && SignedPacked8FullRange::Decode(INT8_MAX) == INT8_MAX,
			"signed 8-bit packed range decodes both endpoints");
		pass &= Expect(SignedPacked16FullRange::Encode(INT16_MIN) == 0 && SignedPacked16FullRange::Encode(INT16_MAX) == INT16_MAX,
			"signed 16-bit packed range maps both endpoints");
		pass &= Expect(SignedPacked16FullRange::Decode(0) == INT16_MIN && SignedPacked16FullRange::Decode(INT16_MAX) == INT16_MAX,
			"signed 16-bit packed range decodes both endpoints");
		pass &= Expect(SignedPacked32FullRange::Encode(INT32_MIN) == 0 && SignedPacked32FullRange::Encode(INT32_MAX) == INT32_MAX,
			"signed 32-bit packed range maps both endpoints");
		pass &= Expect(SignedPacked32FullRange::Decode(0) == INT32_MIN && SignedPacked32FullRange::Decode(INT32_MAX) == INT32_MAX,
			"signed 32-bit packed range decodes both endpoints");
		pass &= Expect(CheckQuantizationSamples<SignedPacked8FullRange, int8_t, INT8_MIN, int8_t, INT8_MAX, 256>(),
			"signed 8-bit packed range meets quantization bounds");
		pass &= Expect(CheckMonotonicSamples<SignedPacked8FullRange, int8_t, INT8_MIN, int8_t, INT8_MAX, 256>(),
			"signed 8-bit packed range is monotonic");
		pass &= Expect(CheckQuantizationSamples<SignedPacked16FullRange, int16_t, INT16_MIN, int16_t, INT16_MAX, 128>(),
			"signed 16-bit packed range meets quantization bounds");
		pass &= Expect(CheckMonotonicSamples<SignedPacked16FullRange, int16_t, INT16_MIN, int16_t, INT16_MAX, 128>(),
			"signed 16-bit packed range is monotonic");
		pass &= Expect(CheckMonotonicSamples<SignedPacked32FullRange, int32_t, INT32_MIN, int32_t, INT32_MAX, 32>(),
			"signed 32-bit packed range is monotonic");
		return pass;
	}

	inline bool RunRoundingAndClampTests()
	{
		bool pass = true;
		pass &= Expect(Signed8EncodeTie::Encode(1) == 2,
			"encode halfway values round up toward the packed maximum");
		pass &= Expect(Signed8DecodeTie::Decode(2) == 2,
			"decode halfway values round up toward the value maximum");
		pass &= Expect(Signed8Asymmetric::Encode(INT8_MIN) == 0 && Signed8Asymmetric::Encode(INT8_MAX) == 0x0Fu,
			"signed scaled inputs clamp to declared bounds");
		pass &= Expect(Signed8Negative::Encode(INT8_MIN) == 0 && Signed8Negative::Encode(INT8_MAX) == 0x0Fu,
			"all-negative scaled inputs clamp to declared bounds");
		pass &= Expect(Unsigned16Range::Encode(0) == 0 && Unsigned16Range::Encode(UINT16_MAX) == 0x0FFFu,
			"unsigned scaled inputs clamp to declared bounds");
		pass &= Expect(Signed8Asymmetric::Decode(0xFFu) == 20,
			"scaled decode clamps packed input to raw maximum");
		pass &= Expect(Signed16Asymmetric::Encode(INT16_MIN) == 0 && Signed16Asymmetric::Encode(INT16_MAX) == 0x0FFFu,
			"asymmetric int16 encode clamps to declared bounds");
		pass &= Expect(Signed16Asymmetric::Decode(UINT16_MAX) == 12345,
			"asymmetric int16 decode clamps packed input");
		pass &= Expect(Unsigned8Range::Encode(0) == 0 && Unsigned8Range::Encode(UINT8_MAX) == 0x0Fu,
			"unsigned 8-bit encode clamps to declared bounds");
		pass &= Expect(Unsigned8Range::Decode(UINT8_MAX) == 200,
			"unsigned 8-bit decode clamps packed input");
		pass &= Expect(SignedPacked32FullRange::Decode(-1) == INT32_MIN,
			"signed packed decode clamps negative raw values to zero");
		return pass;
	}

	inline bool RunWideSignedBoundaryTests()
	{
		bool pass = true;
		pass &= Expect(Signed32FullRange::ValueRange == 0xFFFFFFFFu,
			"int32 full-range difference is represented without signed overflow");
		pass &= Expect(Signed32FullRange::Encode(INT32_MIN) == 0,
			"int32 minimum encodes to zero");
		pass &= Expect(Signed32FullRange::Encode(INT32_MAX) == UINT32_MAX,
			"int32 maximum encodes to the full uint32 packed maximum");
		pass &= Expect(Signed32FullRange::Decode(0) == INT32_MIN,
			"int32 raw zero decodes to minimum");
		pass &= Expect(Signed32FullRange::Decode(UINT32_MAX) == INT32_MAX,
			"int32 raw maximum decodes to maximum");
		pass &= Expect(Signed32FullRange::Encode(0) == 0x80000000u && Signed32FullRange::Decode(0x80000000u) == 0,
			"int32 zero maps exactly at the full-range midpoint");
		pass &= Expect(Unsigned32FullRange::Encode(UINT32_MAX) == UINT32_MAX && Unsigned32FullRange::Decode(UINT32_MAX) == UINT32_MAX,
			"uint32 maximum maps exactly through the full packed range");
		pass &= Expect(Unsigned32FullRange::Encode(0x80000000u) == 0x80000000u &&
			Unsigned32FullRange::Decode(0x80000000u) == 0x80000000u,
			"uint32 midpoint maps exactly through the full packed range");
		pass &= Expect(Signed32CompactRange::Encode(INT32_MAX) == 0x001FFFFFu &&
			Signed32CompactRange::Decode(0x001FFFFFu) == INT32_MAX,
			"int32 range remains exact with the compact 21-bit packed maximum");

		const int32_t signedCases[] = { INT32_MIN, INT32_MIN + 1, -1, 0, 1, INT32_MAX - 1, INT32_MAX };
		const uint32_t unsignedCases[] = { 0u, 1u, 2u, 0x7FFFFFFFu, 0x80000000u, UINT32_MAX - 1u, UINT32_MAX };
		for (uint8_t index = 0; index < sizeof(signedCases) / sizeof(signedCases[0]); ++index)
		{
			pass &= CheckEncodeQuantization<Signed32FullRange, int32_t, INT32_MIN, uint32_t, UINT32_MAX>(signedCases[index]);
			pass &= CheckEncodeQuantization<SignedPacked32FullRange, int32_t, INT32_MIN, int32_t, INT32_MAX>(signedCases[index]);
		}
		for (uint8_t index = 0; index < sizeof(unsignedCases) / sizeof(unsignedCases[0]); ++index)
		{
			pass &= CheckEncodeQuantization<Unsigned32FullRange, uint32_t, 0, uint32_t, UINT32_MAX>(unsignedCases[index]);
		}
		pass &= Expect(CheckQuantizationSamples<Signed32FullRange, int32_t, INT32_MIN, uint32_t, UINT32_MAX, 128>(),
			"int32 full-range mapping stays within quantization bounds");
		pass &= Expect(CheckQuantizationSamples<Unsigned32FullRange, uint32_t, 0, uint32_t, UINT32_MAX, 128>(),
			"uint32 full-range mapping stays within quantization bounds");
		pass &= Expect(CheckQuantizationSamples<SignedPacked32FullRange, int32_t, INT32_MIN, int32_t, INT32_MAX, 128>(),
			"signed-packed int32 mapping stays within quantization bounds");
		pass &= Expect(CheckMonotonicSamples<Signed32FullRange, int32_t, INT32_MIN, uint32_t, UINT32_MAX, 32>(),
			"int32 full-range mapping stays monotonic");
		pass &= Expect(CheckMonotonicSamples<Unsigned32FullRange, uint32_t, 0, uint32_t, UINT32_MAX, 32>(),
			"uint32 full-range mapping stays monotonic");
		return pass;
	}

	inline bool RunTwoComplementCodecTests()
	{
		bool pass = true;
		pass &= Expect(Signed4BitLimited::Encode(-8) == 8 && Signed4BitLimited::Decode(8) == -8,
			"limited signed codec preserves four-bit two's-complement minimum");
		pass &= Expect(Signed4BitLimited::Encode(-9) == 8 && Signed4BitLimited::Encode(8) == 7,
			"limited signed codec clamps before packing");
		pass &= Expect(Signed4BitMasked::Encode(-8) == 8 && Signed4BitMasked::Decode(8) == -8,
			"masked signed codec preserves four-bit two's-complement sign");
		pass &= Expect(Signed4BitMasked::Encode(-9) == 7 && Signed4BitMasked::Decode(0xF7u) == 7,
			"masked signed codec retains mask behavior independently of scaling");
		return pass;
	}

	inline bool RunMaskBoundaryTests()
	{
		bool pass = true;
		pass &= Expect(Inertia::Components::Surface::Mask<64>() == ~uint64_t(0),
			"64-bit mask covers the full width without shifting by 64");
		pass &= Expect(Inertia::Components::Surface::Mask<uint8_t, 8>() == UINT8_MAX,
			"typed full-width mask covers all 8 bits");
		pass &= Expect(Inertia::Components::Surface::Mask<uint16_t, 16>() == UINT16_MAX,
			"typed full-width mask covers all 16 bits");
		pass &= Expect(Inertia::Components::Surface::Mask<uint32_t, 32>() == UINT32_MAX,
			"typed full-width mask covers all 32 bits");
		pass &= Expect(Inertia::Components::Surface::Mask<uint64_t, 64>() == UINT64_MAX,
			"typed full-width mask covers all 64 bits");
		return pass;
	}

	inline bool RunAllTests()
	{
		bool pass = true;
		pass &= RunSigned8FullRangeTests();
		pass &= RunAsymmetricSignedTests();
		pass &= RunNegativeAndUnsignedTests();
		pass &= RunRoundingAndClampTests();
		pass &= RunWideSignedBoundaryTests();
		pass &= RunSignedPackedRangeTests();
		pass &= RunTwoComplementCodecTests();
		pass &= RunMaskBoundaryTests();
		return pass;
	}
}

#endif
