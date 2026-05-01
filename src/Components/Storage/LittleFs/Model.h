#ifndef _INERTIA_COMPONENTS_STORAGE_LITTLEFS_MODEL_h
#define _INERTIA_COMPONENTS_STORAGE_LITTLEFS_MODEL_h

#include "../../../Components/Core/Primitives.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace LittleFs
			{
				static constexpr uint32_t LOG_TAG = 540971179; // Random unique tag for LittleFS logs.

				enum class LogCodeEnum : uint8_t
				{
					BeginFailed,
					FormatFailed,
					Format,
					BeginAfterFormatFailed,
					Mounted,
					Unmounted,

					EnumCount
				};
			}
		}
	}
}
#endif