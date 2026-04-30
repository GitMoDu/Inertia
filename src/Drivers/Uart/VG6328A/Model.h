#ifndef _INERTIA_DRIVERS_UART_VG6328A_MODEL_h
#define _INERTIA_DRIVERS_UART_VG6328A_MODEL_h

#include "../../../Framework/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Uart
		{
			/// <summary>
			/// Watchdog support for a VG6328A Bluetooth module connection indicator.
			/// </summary>
			namespace VG6328A
			{
				static constexpr uint32_t LOG_TAG = 505949888; // Random unique tag for VG6328A logs.

				enum class LogCodeEnum : uint8_t
				{
					InvalidDeviceName, // The device name provided in setup is invalid.

					// Watchdog error codes.
					WatchDogDetectedStuckState
				};
			}
		}
	}
}
#endif