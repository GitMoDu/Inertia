#ifndef _INERTIA_DRIVERS_UART_VG6328A_TEMPLATE_DRIVER_h
#define _INERTIA_DRIVERS_UART_VG6328A_TEMPLATE_DRIVER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "Model.h"
#include <Stream.h>
#include "DeviceDriver.h"


#include "../../../Components/Core/DataSource/Model.h"
#include "../../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Uart
		{
			/// <summary>
			/// Stream-based interface for communicating with a VG6328A Bluetooth module over a serial connection. 
			/// </summary>
			namespace VG6328A
			{
				/// <summary>
				/// Stream-based interface for communicating with a VG6328A Bluetooth module over a serial connection.
				/// Keeps baudrate at 115200 (max) and provides helper functions for initialization, device ID retrieval, and clean shutdown.
				/// </summary>
				/// <typeparam name="SerialType">The underlying serial communication type (e.g., HardwareSerial, SoftwareSerial) used to communicate with the VG6328A device.</typeparam>
				template<typename SerialType>
				class StreamDriver : public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Stream
				{
				private:
					using Driver = Device::Driver;
					char DeviceId[Driver::DEVICE_UID_SIZE]{};
					bool DeviceReady = false;

				private:
					const char* DeviceName = "XLBLE";

				private:
					SerialType& SerialInstance;

				public:
					StreamDriver(SerialType& serial)
						: Inertia::Components::Lifecycle::ILifecycleDriver()
						, Stream()
						, SerialInstance(serial)
					{}


					bool Setup(const char* name)
					{
						DeviceName = name;

						//TODO: Return true if device name is valid (null terminated)
						return true;
					}

					bool Start() override
					{
						// Ensure clean state.
						if (DeviceReady)
						{
							DeviceReady = false;
							Driver::disconnectSPP(SerialInstance);
							Driver::disconnectBLE(SerialInstance);
						}
						SerialInstance.end();
						SerialInstance.begin(Driver::BLE_BAUD_RATE);
						delay(10); // Allow time for serial port to initialize.

						// Factory reset to ensure a known state. This will clear any existing connections and settings.
						Driver::factoryResetNoResponse(SerialInstance);

						// Re-enter command mode after reset.
						if (!Driver::resetAndReenterCommandMode(SerialInstance)) { return false; }

						// Retrieve the device UID.
						if (!Driver::getFlashUidLine(SerialInstance, DeviceId, sizeof(DeviceId))) { return false; }

						// Set name for both SPP and BLE to ensure consistent identification across connection types.
						if (!Driver::setBLEName(SerialInstance, DeviceName)) { return false; }

						// Stop unused Bluetooth Classic (SPP) mode.
						if (!Driver::disconnectSPP(SerialInstance)) { return false; }

						// Restart to apply name change and return to command mode.
						Driver::resetNoResponse(SerialInstance);
						//if (!Driver::enterCommandMode(SerialInstance)) { return false; }
						//if (!Driver::resetAndReenterCommandMode(SerialInstance)) { return false; }

						// Device configured, enter data mode to start transparent serial communication over BLE.
						//if (!Driver::enterDataMode(SerialInstance)) { return false; }

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

					void Stop() override
					{
						if (DeviceReady)
						{
							DeviceReady = false;
							Driver::disconnectSPP(SerialInstance);
							Driver::disconnectBLE(SerialInstance);
						}
						SerialInstance.end();
					}

					// Hardware serial interface. begin and end are no-ops since initialization and cleanup are handled in Start and Stop.
				public:
					void begin(unsigned long baudRate = 0)
					{}

					void end()
					{}

					operator bool() const
					{
						return DeviceReady;
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

					using Stream::write;
				};
			}

		}
	}
}
#endif