#ifndef _INERTIA_COMPONENTS_UART_INTERFACE_MODEL_h
#define _INERTIA_COMPONENTS_UART_INTERFACE_MODEL_h

#include <stdint.h>

namespace Inertia
{
	namespace Components
	{
		namespace UartInterface
		{
			static constexpr uint32_t LOG_TAG = 3851162775; // Random unique tag for UART interface.

			enum class LogCodeEnum : uint8_t
			{
				// Connection state change codes.
				Connected,
				Disconnected,
				ConnectingTimeout,

				// Interface handling error codes.
				ErrorRxUnrecognizedHeader,
				ErrorRxUnexpectedMessageInState,
				ErrorRxUnexpectedSize,

				// Uart RX error codes.
				ErrorRxStartTimeout,
				ErrorRxCrc,
				ErrorRxTooShort,
				ErrorRxTooLong,
				ErrorRxEndTimeout,
				ErrorRxUnknown,

				// Uart TX error codes.
				ErrorTxStartTimeout,
				ErrorTxDataTimeout,
				ErrorTxEndTimeout,
				ErrorTxUnknown				
			};
		}
	}
}
#endif