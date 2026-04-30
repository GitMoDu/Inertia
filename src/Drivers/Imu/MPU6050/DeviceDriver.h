#ifndef _INERTIA_DRIVERS_IMU_MPU6050_DEVICE_DRIVER_h
#define _INERTIA_DRIVERS_IMU_MPU6050_DEVICE_DRIVER_h

#include "Model.h"
#include "../../HardwareInterface/I2c/I2cInterface.h"
#include <Wire.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Imu
		{
			namespace Mpu6050
			{
				namespace Device
				{
					using namespace IntegerSignal;
					using namespace IntegerSignal::FixedPoint::FactorScale;

					static constexpr uint8_t DEVICE_ADDRESS_LOW = 0x68;
					static constexpr uint8_t DEVICE_ADDRESS_HIGH = 0x69;


					/// <summary>
					/// Raw hardware-access driver for the MPU6050 IMU.
					/// </summary>
					template<typename WireType = TwoWire>
					class Driver
					{
					private:
						static constexpr uint8_t REGISTER_CONFIG = 0x1A;
						static constexpr uint8_t REGISTER_GYRO_CONFIG = 0x1B;
						static constexpr uint8_t REGISTER_ACCEL_CONFIG = 0x1C;
						static constexpr uint8_t REGISTER_ACCEL_XOUT_H = 0x3B;
						static constexpr uint8_t REGISTER_TEMP_OUT_H = 0x41;
						static constexpr uint8_t REGISTER_PWR_MGMT_1 = 0x6B;
						static constexpr uint8_t REGISTER_PWR_MGMT_2 = 0x6C;
						static constexpr uint8_t REGISTER_WHO_AM_I = 0x75;

						static constexpr uint8_t PWR_MGMT_1_CLOCK_PLL_XGYRO = 0x01;
						static constexpr uint8_t CONFIG_DLPF_MASK = 0x07;
						static constexpr uint8_t RANGE_MASK = 0x18;
						static constexpr uint8_t RANGE_SHIFT = 3;
						static constexpr uint8_t WHO_AM_I_MASK = 0x7E;
						static constexpr uint8_t WHO_AM_I_EXPECTED = 0x68;
						static constexpr uint32_t CLOCK_SPEED_I2C = 400000;


					private:
						WireType& WireInstance;
						uint8_t Address;

					public:
						Driver(WireType& wire, const uint8_t address = DEVICE_ADDRESS_LOW)
							: WireInstance(wire)
							, Address(address)
						{}

						bool initialize()
						{
							return WriteRegister(REGISTER_PWR_MGMT_1, PWR_MGMT_1_CLOCK_PLL_XGYRO)
								&& WriteRegister(REGISTER_PWR_MGMT_2, 0x00);
						}

						bool testConnection()
						{
							uint8_t whoAmI = 0;
							return ReadBytes(REGISTER_WHO_AM_I, &whoAmI, 1)
								&& (whoAmI & WHO_AM_I_MASK) == WHO_AM_I_EXPECTED;
						}

						bool setFullScaleAccelRange(const uint8_t range)
						{
							return UpdateRegisterBits(REGISTER_ACCEL_CONFIG, RANGE_MASK, (range & 0x03) << RANGE_SHIFT);
						}

						bool setFullScaleGyroRange(const uint8_t range)
						{
							return UpdateRegisterBits(REGISTER_GYRO_CONFIG, RANGE_MASK, (range & 0x03) << RANGE_SHIFT);
						}

						bool setDLPFMode(const uint8_t mode)
						{
							return UpdateRegisterBits(REGISTER_CONFIG, CONFIG_DLPF_MASK, mode & CONFIG_DLPF_MASK);
						}

						bool getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz)
						{
							uint8_t buffer[14]{};
							if (!ReadBytes(REGISTER_ACCEL_XOUT_H, buffer, sizeof(buffer)))
							{
								return false;
							}

							*ax = CombineBytes(buffer[0], buffer[1]);
							*ay = CombineBytes(buffer[2], buffer[3]);
							*az = CombineBytes(buffer[4], buffer[5]);
							*gx = CombineBytes(buffer[8], buffer[9]);
							*gy = CombineBytes(buffer[10], buffer[11]);
							*gz = CombineBytes(buffer[12], buffer[13]);

							return true;
						}

						bool getTemperature(int16_t& temperature)
						{
							uint8_t buffer[2]{};
							if (!ReadBytes(REGISTER_TEMP_OUT_H, buffer, sizeof(buffer)))
							{
								return false;
							}

							temperature = CombineBytes(buffer[0], buffer[1]);
							return true;
						}

					private:
						static int16_t CombineBytes(const uint8_t high, const uint8_t low)
						{
							return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
						}

						bool UpdateRegisterBits(const uint8_t registerAddress, const uint8_t mask, const uint8_t value)
						{
							uint8_t currentValue = 0;
							if (!ReadBytes(registerAddress, &currentValue, 1))
							{
								return false;
							}

							currentValue = static_cast<uint8_t>((currentValue & ~mask) | (value & mask));
							return WriteRegister(registerAddress, currentValue);
						}

						bool WriteRegister(const uint8_t registerAddress, const uint8_t value)
						{
							Inertia::Drivers::HardwareInterface::I2c::Drivers::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
							WireInstance.beginTransmission(Address);
							WireInstance.write(registerAddress);
							WireInstance.write(value);
							return WireInstance.endTransmission() == 0;
						}

						bool ReadBytes(const uint8_t registerAddress, uint8_t* buffer, const uint8_t length)
						{
							Inertia::Drivers::HardwareInterface::I2c::Drivers::SetClockIfSupported(WireInstance, CLOCK_SPEED_I2C);
							WireInstance.beginTransmission(Address);
							WireInstance.write(registerAddress);
							if (WireInstance.endTransmission(false) != 0)
							{
								return false;
							}

							const uint8_t bytesRead = WireInstance.requestFrom(Address, length);
							if (bytesRead != length)
							{
								while (WireInstance.available())
								{
									WireInstance.read();
								}

								return false;
							}

							for (uint8_t i = 0; i < length; i++)
							{
								if (!WireInstance.available())
								{
									return false;
								}

								buffer[i] = static_cast<uint8_t>(WireInstance.read());
							}

							return true;
						}
					};
				}
			}
		}
	}
}

#endif