#ifndef _INERTIA_COMPONENTS_SURFACE_MODEL_h
#define _INERTIA_COMPONENTS_SURFACE_MODEL_h

#include <IntegerSignal.h>

namespace Inertia
{
	namespace Components
	{
		namespace Surface
		{
			using namespace IntegerSignal::BitSize;
			using namespace IntegerSignal::BitType;
			using namespace IntegerSignal::ByteSize;
			using namespace IntegerSignal::ByteType;

			template<uint8_t enumSize>
			static constexpr uint64_t Mask()
			{
				static_assert(enumSize <= 64, "Enum size can only be up to 64.");

				return (enumSize == 0) ? 0 : ((uint64_t(1) << enumSize) - 1);
			}

			template<typename enum_t,
				uint8_t enumSize>
			static constexpr enum_t Mask()
			{
				static_assert(enumSize <= sizeof(enum_t) * 8, "Enum size can only be up to the size of the enum type.");

				return (enumSize == 0) ? 0 : ((enum_t(1) << enumSize) - 1);
			}


			template<uint8_t enumSize>
			static constexpr uint8_t MaskSize()
			{
				static_assert(enumSize <= 64, "Enum size can only be up to 64.");

				return (enumSize == 0) ? 0 : (bit_count<enumSize - 1>::value);
			}

			template<typename enum_t, uint8_t enumSize>
			static constexpr enum_t MaskSize()
			{
				static_assert(enumSize <= sizeof(enum_t) * 8, "Enum size can only be up to the size of the enum type.");

				return (enumSize == 0) ? 0 : (bit_count<enumSize - 1>::value);
			}

			template<typename ComboType,
				uint8_t ComboItem,
				uint8_t EnumSize
			>
			static constexpr uint8_t GetComboEnum(const ComboType comboBits)
			{
				return (comboBits >> ComboItem) & Mask<ComboType, EnumSize>();
			}

			template<typename ComboType,
				uint8_t ComboItem,
				uint8_t EnumSize
			>
			static void SetComboEnum(ComboType& comboBits, const uint8_t value)
			{
				comboBits &= ~((ComboType)Mask<ComboType, EnumSize>() << ComboItem);
				comboBits |= ((ComboType)value) << ComboItem;
			}
		}
	}
}

#endif