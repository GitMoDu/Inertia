#ifndef _INERTIA_DRIVERS_I2C_INTERFACE_h
#define _INERTIA_DRIVERS_I2C_INTERFACE_h

#include "Model.h"
#include <Wire.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace I2cInterface
		{
			void RecoverInterface(TwoWire& wire)
			{
				// Attempt to recover the I2C bus by sending a stop condition and clearing any error states.
				wire.endTransmission();

#if defined(ARDUINO_ARCH_RP2040)
				wire.clearTimeoutFlag();
				wire.abortAsync();
				wire.clearWriteError();
#endif
			}

		}
	}
}
#endif