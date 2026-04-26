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

			static constexpr uint32_t GetLimitedClock(const uint32_t requestedClock)
			{
#if defined(ARDUINO_ARCH_AVR)
				// AVR-based Arduinos typically support up to 400 kHz I2C clock, but performance may degrade at higher speeds.
				return requestedClock > 400000 ? 400000 : requestedClock;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAM_DUE)
				// SAMD and SAM DUE support up to 1 MHz I2C clock.
				return requestedClock > 1000000 ? 1000000 : requestedClock;
#elif defined(ARDUINO_ARCH_ESP32S2) || defined(ARDUINO_ARCH_ESP32C3) || defined(ARDUINO_ARCH_ESP32S3) || defined(ARDUINO_ARCH_ESP32H2)
				// ESP32-S2 and ESP32-C3 support up to 1 MHz I2C clock.
				return requestedClock > 1000000 ? 1000000 : requestedClock;
#elif defined(ARDUINO_ARCH_NRF52)
				// NRF52-based Arduinos typically support up to 400 kHz I2C clock.
				return requestedClock > 400000 ? 400000 : requestedClock;
#elif defined(ARDUINO_ARCH_RP2040)
				// RP2040 supports up to 1 MHz I2C clock.
				return requestedClock > 1000000 ? 1000000 : requestedClock;
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
				// ESP32 and ESP8266 support up to 400 kHz I2C clock.
				return requestedClock > 400000 ? 400000 : requestedClock;
#else
				return 200000; // Minimum guaranteed by most Arduino hardware.
#endif

				return 0;
			}

			template<typename TWire>
			void SetClockIfSupported(TWire& wire, const uint32_t clockSpeed)
			{
#if defined(ARDUINO_ARCH_AVR) \
	|| defined(ARDUINO_ARCH_SAMD) \
	|| defined(ARDUINO_ARCH_SAM_DUE) \
	|| defined(ARDUINO_ARCH_ESP32) \
	|| defined(ARDUINO_ARCH_ESP32S2) \
	|| defined(ARDUINO_ARCH_ESP32C3) \
	|| defined(ARDUINO_ARCH_ESP32S3) \
	|| defined(ARDUINO_ARCH_ESP32H2) \
	|| defined(ARDUINO_ARCH_ESP8266) \
	|| defined(ARDUINO_ARCH_NRF52) \
	|| defined(ARDUINO_ARCH_RP2040)
				wire.setClock(GetLimitedClock(clockSpeed));
#else
				(void)wire;
				(void)clockSpeed;
#endif
			}
		}
	}
}
#endif