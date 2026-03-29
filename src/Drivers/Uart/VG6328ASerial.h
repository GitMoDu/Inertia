#ifndef _INERTIA_DRIVERS_VG6328A_SERIAL_h
#define _INERTIA_DRIVERS_VG6328A_SERIAL_h

#include "../../Framework/Model.h"
#include <Stream.h>
#include "VG6328A.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Uart
		{
			/// <summary>
			/// Stream-based interface for communicating with a VG6328A Bluetooth module over a serial connection.
			/// Keeps baudrate at 115200 (max) and provides helper functions for initialization, device ID retrieval, and clean shutdown.
			/// </summary>
			/// <typeparam name="SerialType">The underlying serial communication type (e.g., HardwareSerial, SoftwareSerial) used to communicate with the VG6328A device.</typeparam>
			template<typename SerialType>
			class VG6328ASerial : public Stream
			{
			private:
				using VG6328ADriver = Device::VG6328A;
				char DeviceId[VG6328ADriver::DEVICE_UID_SIZE]{};
				bool DeviceReady = false;

			private:
				SerialType& SerialInstance;

			public:
				VG6328ASerial(SerialType& serial)
					: Stream()
					, SerialInstance(serial)
				{}

				bool Start()
				{
					return Start("XLBLE");
				}

				bool Start(const char* name)
				{
					// Ensure clean state.
					if (DeviceReady)
					{
						DeviceReady = false;
						VG6328ADriver::disconnectSPP(SerialInstance);
						VG6328ADriver::disconnectBLE(SerialInstance);
					}
					SerialInstance.end();
					SerialInstance.begin(VG6328ADriver::BLE_BAUD_RATE);
					delay(10); // Allow time for serial port to initialize.

					// Factory reset to ensure a known state. This will clear any existing connections and settings.
					VG6328ADriver::factoryResetNoResponse(SerialInstance);

					// Re-enter command mode after reset.
					if (!VG6328ADriver::resetAndReenterCommandMode(SerialInstance)) { return false; }

					// Retrieve the device UID.
					if (!VG6328ADriver::getFlashUidLine(SerialInstance, DeviceId, sizeof(DeviceId))) { return false; }

					// Set name for both SPP and BLE to ensure consistent identification across connection types.
					if (!VG6328ADriver::setBLEName(SerialInstance, name)) { return false; }

					// Stop unused Bluetooth Classic (SPP) mode.
					if (!VG6328ADriver::disconnectSPP(SerialInstance)) { return false; }

					// Restart to apply name change and return to command mode.
					VG6328ADriver::resetNoResponse(SerialInstance);
					//if (!VG6328ADriver::enterCommandMode(SerialInstance)) { return false; }
					//if (!VG6328ADriver::resetAndReenterCommandMode(SerialInstance)) { return false; }

					// Device configured, enter data mode to start transparent serial communication over BLE.
					//if (!VG6328ADriver::enterDataMode(SerialInstance)) { return false; }

					DeviceReady = true;

					return true;
				}

				bool GetId(char* out, const size_t outSize) const
				{
					if (!DeviceReady || outSize > sizeof(DeviceId))
					{
						if (outSize > 0)
						{
							out[0] = '\0';
						}
						return false;
					}
					strncpy(out, DeviceId, outSize);
					out[outSize - 1] = '\0'; // Ensure null termination.
					return true;
				}

				template <typename PrintSerialType>
				void PrintId(PrintSerialType& serial)
				{
					if (DeviceReady)
					{
						for (uint_fast8_t i = 0; i < sizeof(DeviceId); ++i)
						{
							if (DeviceId[i] < 0x10)
								serial.print('0'); // Leading zero for single hex digits.
							serial.print(DeviceId[i], HEX);
							if (i < sizeof(DeviceId) - 1)
								serial.print(':');
						}
					}
				}

				void Stop()
				{
					if (DeviceReady)
					{
						DeviceReady = false;
						VG6328ADriver::disconnectSPP(SerialInstance);
						VG6328ADriver::disconnectBLE(SerialInstance);
					}
					SerialInstance.end();
				}


				// Stream interface implementation. All operations check DeviceReady before proceeding.
			public:
				int available() override
				{
					return DeviceReady && SerialInstance.available();
				}

				int availableForWrite() override
				{
					return DeviceReady && SerialInstance.availableForWrite();
				}

				int read() override
				{
					if (!DeviceReady)
					{
						return 0;
					}

					return SerialInstance.read();
				}

				int peek() override
				{
					if (!DeviceReady)
					{
						return 0;
					}

					return SerialInstance.peek();
				}

				void flush() override
				{
					if (!DeviceReady)
					{
						return;
					}

					SerialInstance.flush();
				}

				size_t write(uint8_t b) override
				{
					if (!DeviceReady)
					{
						return 0;
					}

					return SerialInstance.write(b);
				}

				size_t write(const uint8_t* buf, size_t n) override
				{
					if (!DeviceReady)
					{
						return 0;
					}

					return SerialInstance.write(buf, n);
				}

				using Print::write;
			};
		}
	}
}
#endif