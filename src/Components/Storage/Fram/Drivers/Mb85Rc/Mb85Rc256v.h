#ifndef _INERTIA_COMPONENTS_STORAGE_FRAM_MB85_RC_256V_h
#define _INERTIA_COMPONENTS_STORAGE_FRAM_MB85_RC_256V_h

#include "Model.h"
#include "../../../../../Drivers/I2c/I2cInterface.h"

#include <Wire.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace Fram
			{
				namespace Drivers
				{
					namespace Mb85Rc
					{
						template<typename WireType = TwoWire>
						class Mb85Rc256v : public Inertia::Components::Storage::Fram::IFramDriver
						{
						public:
							static constexpr uint8_t DEVICE_ADDRESS_BASE = 0x50;
							static constexpr uint8_t DEVICE_ADDRESS_SELECT_MASK = 0x07;
							static constexpr uint16_t CAPACITY_BYTES = 32768;
							static constexpr uint32_t CLOCK_SPEED_I2C = 800000;

						private:
#if defined(I2C_BUFFER_LENGTH)
							static constexpr size_t WireBufferSize = I2C_BUFFER_LENGTH;
#elif defined(BUFFER_LENGTH)
							static constexpr size_t WireBufferSize = BUFFER_LENGTH;
#elif defined(WIRE_BUFFER_LENGTH)
							static constexpr size_t WireBufferSize = WIRE_BUFFER_LENGTH;
#else
							static constexpr size_t WireBufferSize = 32;
#endif
							static constexpr size_t AddressSizeBytes = 2;
							static constexpr size_t MaxTransferSize = WireBufferSize > AddressSizeBytes
								? WireBufferSize - AddressSizeBytes
								: 1;

						public:
							Inertia::Model::ILogListener* LogListener = nullptr;
							uint8_t InstanceId = 0;

						private:
							WireType& WireInstance;
							uint8_t DeviceAddress = DEVICE_ADDRESS_BASE;
							bool Started = false;

						public:
							explicit Mb85Rc256v(WireType& wire = Wire, const uint8_t deviceAddress = DEVICE_ADDRESS_BASE)
								: WireInstance(wire)
								, DeviceAddress(deviceAddress)
							{}

							static constexpr uint8_t GetDeviceAddress(const uint8_t addressSelect)
							{
								return static_cast<uint8_t>(DEVICE_ADDRESS_BASE | (addressSelect & DEVICE_ADDRESS_SELECT_MASK));
							}

							bool Start() override
							{
								Started = TestConnection();
								return Started;
							}

							void Stop() override
							{
								Started = false;
							}

							bool IsStarted() const
							{
								return Started;
							}

							bool TestConnection()
							{
								Inertia::Drivers::I2cInterface::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
								WireInstance.beginTransmission(DeviceAddress);
								const bool connected = WireInstance.endTransmission() == 0;
								if (!connected)
								{
									Inertia::Drivers::I2cInterface::RecoverInterface(WireInstance);
								}

								return connected;
							}

							bool ReadByte(const uint16_t memoryAddress, uint8_t& value)
							{
								return Read(memoryAddress, &value, 1);
							}

							bool Write(const uint16_t memoryAddress, const uint8_t value)
							{
								return Write(memoryAddress, &value, 1);
							}

							template<typename TData>
							bool Read(const uint16_t memoryAddress, TData& value)
							{
								return Read(memoryAddress, reinterpret_cast<uint8_t*>(&value), sizeof(TData));
							}

							template<typename TData>
							bool Write(const uint16_t memoryAddress, const TData& value)
							{
								return Write(memoryAddress, reinterpret_cast<const uint8_t*>(&value), sizeof(TData));
							}

							bool Read(const uint16_t memoryAddress, uint8_t* buffer, const size_t length)
							{
								if (!Started || !IsRangeValid(memoryAddress, length) || (buffer == nullptr && length > 0))
								{
									return false;
								}

								size_t offset = 0;
								uint16_t currentAddress = memoryAddress;

								while (offset < length)
								{
									const size_t remaining = length - offset;
									const size_t chunkLength = remaining < MaxTransferSize ? remaining : MaxTransferSize;

									if (!BeginMemoryTransfer(currentAddress, false))
									{
										return false;
									}

									const uint8_t requestedLength = static_cast<uint8_t>(chunkLength);
									Inertia::Drivers::I2cInterface::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
									const uint8_t bytesRead = WireInstance.requestFrom(DeviceAddress, requestedLength);
									if (bytesRead != requestedLength)
									{
										DrainReadBuffer();
										Inertia::Drivers::I2cInterface::RecoverInterface(WireInstance);
										return false;
									}

									for (size_t i = 0; i < chunkLength; i++)
									{
										if (!WireInstance.available())
										{
											Inertia::Drivers::I2cInterface::RecoverInterface(WireInstance);
											return false;
										}

										buffer[offset + i] = static_cast<uint8_t>(WireInstance.read());
									}

									offset += chunkLength;
									currentAddress = static_cast<uint16_t>(currentAddress + chunkLength);
								}

								return true;
							}

							bool Write(const uint16_t memoryAddress, const uint8_t* buffer, const size_t length)
							{
								if (!Started || !IsRangeValid(memoryAddress, length) || (buffer == nullptr && length > 0))
								{
									return false;
								}

								size_t offset = 0;
								uint16_t currentAddress = memoryAddress;

								while (offset < length)
								{
									const size_t remaining = length - offset;
									const size_t chunkLength = remaining < MaxTransferSize ? remaining : MaxTransferSize;

									Inertia::Drivers::I2cInterface::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
									WireInstance.beginTransmission(DeviceAddress);
									WireInstance.write(static_cast<uint8_t>((currentAddress >> 8) & 0xFFu));
									WireInstance.write(static_cast<uint8_t>(currentAddress & 0xFFu));
									const size_t bytesWritten = WireInstance.write(buffer + offset, chunkLength);
									if (bytesWritten != chunkLength || WireInstance.endTransmission() != 0)
									{
										Inertia::Drivers::I2cInterface::RecoverInterface(WireInstance);
										return false;
									}

									offset += chunkLength;
									currentAddress = static_cast<uint16_t>(currentAddress + chunkLength);
								}

								return true;
							}

						private:
							bool BeginMemoryTransfer(const uint16_t memoryAddress, const bool sendStop)
							{
								Inertia::Drivers::I2cInterface::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
								WireInstance.beginTransmission(DeviceAddress);
								WireInstance.write(static_cast<uint8_t>((memoryAddress >> 8) & 0xFFu));
								WireInstance.write(static_cast<uint8_t>(memoryAddress & 0xFFu));
								if (WireInstance.endTransmission(sendStop) != 0)
								{
									Inertia::Drivers::I2cInterface::RecoverInterface(WireInstance);
									return false;
								}

								return true;
							}

							static bool IsRangeValid(const uint16_t memoryAddress, const size_t length)
							{
								return static_cast<size_t>(memoryAddress) + length <= CAPACITY_BYTES;
							}

							void DrainReadBuffer()
							{
								while (WireInstance.available())
								{
									WireInstance.read();
								}
							}
						};
					}
				}
			}
		}
	}
}

#endif