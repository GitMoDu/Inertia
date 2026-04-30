#ifndef _INERTIA_COMPONENTS_HARDWARE_INTERFACE_SERIAL_MODEL_h
#define _INERTIA_COMPONENTS_HARDWARE_INTERFACE_SERIAL_MODEL_h

#include "../../../Framework/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace HardwareInterface
		{
			namespace Serial
			{
				static constexpr uint32_t LOG_TAG = 2116068916; // Random unique tag for Serial hardware logs.

				enum class LogCodeEnum : uint8_t
				{
					EnumCount
				};
			}
		}
	}
}
#endif