#ifndef _INERTIA_DRIVERS_I2C_MODEL_h
#define _INERTIA_DRIVERS_I2C_MODEL_h

#include "../../Framework/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace I2cInterface
		{
			static constexpr uint32_t LOG_TAG = 1033721212; // Random unique tag for I2C hardware logs.

			enum class LogCodeEnum : uint8_t
			{
				ErrorTimeout,
				RecoveryAttempt,
			};
		}
	}
}
#endif