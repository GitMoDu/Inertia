#ifndef _INERTIA_DRIVERS_UART_VG6328A_DEVICE_DRIVER_h
#define _INERTIA_DRIVERS_UART_VG6328A_DEVICE_DRIVER_h

#include <stdint.h>

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
				namespace Device
				{
					/// <summary>
					/// Static hardware driver for the VG6328A BLE Serial Port Profile module.
					/// Provides AT command helpers over a caller-owned serial port. Use VG6328ASerial for Arduino Stream integration.
					/// </summary>
					class Driver
					{
					public:
						enum class ConnStatus : uint8_t
						{
							Unknown = 0xFF,
							None = 0x04,
							BLE = 0x10,
							SPP = 0x0F,
							Dual = 0x1B
						};

						enum class Baud : uint8_t
						{
							B9600 = 0,
							B19200 = 1,
							B38400 = 2,
							B57600 = 3,
							B115200 = 4
						};

					private:
						enum class CodeASCII : char
						{
							Zero = '0',
							One = '1',
							Four = '4',
							CarriageReturn = '\r',
							LineFeed = '\n',
							NullTerminator = '\0'
						};

						enum class CodeHex : char
						{
							BLower = 'b',
							BUpper = 'B',
							FLower = 'f',
							FUpper = 'F',
							PrefixLower = 'x',
							PrefixUpper = 'X'
						};

					public:
						/// <summary>
						/// Default module UART baud rate.
						/// </summary>
						static constexpr uint32_t BLE_BAUD_RATE = 115200;

						/// <summary>
						/// The size of a device unique identifier (UID) in bytes.
						/// </summary>
						static constexpr uint8_t DEVICE_UID_SIZE = 16;

						/// <summary>
						/// The maximum supported device name length in characters.
						/// </summary>
						static constexpr uint8_t DEVICE_NAME_MAX_LENGTH = 15;

					private:
						/// <summary>
						/// Delay in milliseconds after reset commands.
						/// </summary>
						static constexpr uint16_t RESET_DELAY_MILLIS = 60;

						/// <summary>
						/// AT command response timeout in milliseconds.
						/// </summary>
						static constexpr uint16_t AT_TIMEOUT_MILLIS = 500;

						/// <summary>
						/// Delay in milliseconds between serial polls.
						/// </summary>
						static constexpr uint8_t SERIAL_POLL_DELAY_MILLIS = 1;

						/// <summary>
						/// Internal line buffer size for AT response parsing.
						/// </summary>
						static constexpr size_t LineBufSize = 16;

						static constexpr uint8_t BAUD_VALUE_INDEX = 7;
						static constexpr uint8_t HEX_PREFIX_LENGTH = 2;
						static constexpr uint8_t OK_RESPONSE_LENGTH = 2;

					private:
						Driver() = delete;

					public:
						// AT: mode
						template <typename SerialType>
						static bool enterCommandMode(SerialType& serial)
						{
							return sendExpectOK(serial, F("AT+ENAT"));
						}

						template <typename SerialType>
						static bool enterDataMode(SerialType& serial)
						{
							return sendExpectOK(serial, F("AT+EXAT"));
						}

						// AT: advertising / discoverability
						template <typename SerialType>
						static bool enableBLEBroadcast(SerialType& serial, const bool on)
						{
							return sendExpectOK(serial, on ? F("AT+LEON") : F("AT+LEOF"));
						}

						template <typename SerialType>
						static bool enableSPPBroadcast(SerialType& serial, const bool on)
						{
							return sendExpectOK(serial, on ? F("AT+SPON") : F("AT+SPOF"));
						}

						// AT: names
						template <typename SerialType>
						static bool getSPPName(SerialType& serial, char* out, const size_t outSize)
						{
							return queryLine(serial, F("AT+SPGN"), out, outSize);
						}

						template <typename SerialType>
						static bool getBLEName(SerialType& serial, char* out, const size_t outSize)
						{
							return queryLine(serial, F("AT+LEGN"), out, outSize);
						}

						template <typename SerialType>
						static bool setSPPName(SerialType& serial, const char* name)
						{
							return sendWithParamExpectOK(serial, F("AT+SPNA"), name);
						}

						template <typename SerialType>
						static bool setBLEName(SerialType& serial, const char* name)
						{
							return sendWithParamExpectOK(serial, F("AT+LENA"), name)
								&& setSPPName(serial, name); // Keep SPP name in sync with BLE name for simplicity.
						}

						// AT: addresses (12 hex chars)
						template <typename SerialType>
						static bool getSPPAddress(SerialType& serial, char* out, const size_t outSize)
						{
							return queryLine(serial, F("AT+SPGA"), out, outSize);
						}

						template <typename SerialType>
						static bool getBLEAddress(SerialType& serial, char* out, const size_t outSize)
						{
							return queryLine(serial, F("AT+LEGA"), out, outSize);
						}

						template <typename SerialType>
						static bool setSPPAddressHex12(SerialType& serial, const char* hex12)
						{
							return sendWithParamExpectOK(serial, F("AT+SPAD"), hex12);
						}

						template <typename SerialType>
						static bool setBLEAddressHex12(SerialType& serial, const char* hex12)
						{
							return sendWithParamExpectOK(serial, F("AT+LEAD"), hex12);
						}

						// AT: connection status / disconnect
						template <typename SerialType>
						static bool getConnStatus(SerialType& serial, ConnStatus& outStatus)
						{
							char line[LineBufSize]{};
							if (!queryLine(serial, F("AT+CONN"), line, sizeof(line)))
							{
								return false;
							}

							const char* code = line;
							if (hasHexPrefix(code))
							{
								code += HEX_PREFIX_LENGTH;
							}

							switch (code[0])
							{
							case static_cast<char>(CodeASCII::Four):
								if (code[1] == static_cast<char>(CodeASCII::NullTerminator))
								{
									outStatus = ConnStatus::None;
									return true;
								}
								break;

							case static_cast<char>(CodeASCII::Zero):
								switch (code[1])
								{
								case static_cast<char>(CodeASCII::Four):
									if (code[2] == static_cast<char>(CodeASCII::NullTerminator))
									{
										outStatus = ConnStatus::None;
										return true;
									}
									break;

								case static_cast<char>(CodeHex::FUpper):
								case static_cast<char>(CodeHex::FLower):
									if (code[2] == static_cast<char>(CodeASCII::NullTerminator))
									{
										outStatus = ConnStatus::SPP;
										return true;
									}
									break;
								}
								break;

							case static_cast<char>(CodeHex::FUpper):
							case static_cast<char>(CodeHex::FLower):
								if (code[1] == static_cast<char>(CodeASCII::NullTerminator))
								{
									outStatus = ConnStatus::SPP;
									return true;
								}
								break;

							case static_cast<char>(CodeASCII::One):
								switch (code[1])
								{
								case static_cast<char>(CodeASCII::Zero):
									if (code[2] == static_cast<char>(CodeASCII::NullTerminator))
									{
										outStatus = ConnStatus::BLE;
										return true;
									}
									break;

								case static_cast<char>(CodeHex::BUpper):
								case static_cast<char>(CodeHex::BLower):
									if (code[2] == static_cast<char>(CodeASCII::NullTerminator))
									{
										outStatus = ConnStatus::Dual;
										return true;
									}
									break;
								}
								break;
							}

							outStatus = ConnStatus::Unknown;
							return false;
						}

						template <typename SerialType>
						static bool disconnectSPP(SerialType& serial)
						{
							return sendExpectOK(serial, F("AT+SPNC"));
						}

						template <typename SerialType>
						static bool disconnectBLE(SerialType& serial)
						{
							return sendExpectOK(serial, F("AT+LENC"));
						}

						// AT: baud
						template <typename SerialType>
						static bool setBaud(SerialType& serial, const Baud b)
						{
							char cmd[] = "AT+BAUD0";
							cmd[BAUD_VALUE_INDEX] = static_cast<char>(static_cast<char>(CodeASCII::Zero) + static_cast<uint8_t>(b));
							return sendExpectOK(serial, cmd);
						}

						// AT: reset / defaults
						// Datasheet indicates no response for these.
						template <typename SerialType>
						static void resetNoResponse(SerialType& serial)
						{
							sendNoWait(serial, F("AT+REST"));
							delay(RESET_DELAY_MILLIS);
						}

						template <typename SerialType>
						static void factoryResetNoResponse(SerialType& serial)
						{
							sendNoWait(serial, F("AT+RDEF"));
							delay(RESET_DELAY_MILLIS);
						}

						// AT: UID / version
						template <typename SerialType>
						static bool getFlashUidLine(SerialType& serial, char* out, const size_t outSize = DEVICE_UID_SIZE)
						{
							if (outSize > DEVICE_UID_SIZE)
							{
								out[0] = static_cast<char>(CodeASCII::NullTerminator);
								return false;
							}
							else
							{
								return queryLine(serial, F("AT+FUID"), out, outSize);
							}
						}

						template <typename SerialType>
						static bool getVersionLine(SerialType& serial, char* out, const size_t outSize)
						{
							return queryLine(serial, F("AT+VERS"), out, outSize);
						}

						template <typename SerialType>
						static bool resetAndReenterCommandMode(SerialType& serial)
						{
							drainInput(serial);
							resetNoResponse(serial);
							return enterCommandMode(serial);
						}

					private:
						template <typename SerialType>
						static void drainInput(SerialType& serial)
						{
							while (serial.available())
							{
								(void)serial.read();
							}
						}

						template <typename SerialType>
						static void sendLine(SerialType& serial, const __FlashStringHelper* cmd)
						{
							serial.print(cmd);
							serial.println();
						}

						template <typename SerialType>
						static void sendLine(SerialType& serial, const char* cmd)
						{
							serial.print(cmd);
							serial.println();
						}

						template <typename SerialType>
						static void sendNoWait(SerialType& serial, const __FlashStringHelper* cmd)
						{
							drainInput(serial);
							sendLine(serial, cmd);
						}

						template <typename SerialType>
						static bool sendExpectOK(SerialType& serial, const __FlashStringHelper* cmd)
						{
							drainInput(serial);
							sendLine(serial, cmd);
							return readExactOK(serial);
						}

						template <typename SerialType>
						static bool sendExpectOK(SerialType& serial, const char* cmd)
						{
							drainInput(serial);
							sendLine(serial, cmd);
							return readExactOK(serial);
						}

						template <typename SerialType>
						static bool sendWithParamExpectOK(SerialType& serial, const __FlashStringHelper* cmdPrefix, const char* param)
						{
							drainInput(serial);
							serial.print(cmdPrefix);
							serial.print(param);
							serial.println();

							return readExactOK(serial);
						}

						template <typename SerialType>
						static bool readLine(SerialType& serial, char* out, const size_t outSize)
						{
							if (!out || outSize == 0)
							{
								return false;
							}

							out[0] = static_cast<char>(CodeASCII::NullTerminator);

							const uint32_t start = millis();
							size_t n = 0;

							while ((millis() - start) < AT_TIMEOUT_MILLIS)
							{
								if (!serial.available())
								{
									delay(SERIAL_POLL_DELAY_MILLIS);
									continue;
								}

								const char c = static_cast<char>(serial.read());

								if (c == static_cast<char>(CodeASCII::CarriageReturn))
								{
									continue;
								}

								if (c == static_cast<char>(CodeASCII::LineFeed))
								{
									break;
								}

								if ((n + 1) < outSize)
								{
									out[n++] = c;
								}
							}

							out[n] = static_cast<char>(CodeASCII::NullTerminator);

							return out[0] != static_cast<char>(CodeASCII::NullTerminator);
						}

						template <typename SerialType>
						static bool readExactOK(SerialType& serial)
						{
							char line[LineBufSize]{};

							if (!readLine(serial, line, sizeof(line)))
							{
								return false;
							}

							if (strlen(line) != OK_RESPONSE_LENGTH)
							{
								return false;
							}

							return line[0] == 'O'
								&& line[1] == 'K'
								&& line[2] == static_cast<char>(CodeASCII::NullTerminator);
						}

						template <typename SerialType>
						static bool queryLine(SerialType& serial, const __FlashStringHelper* cmd, char* out, const size_t outSize)
						{
							drainInput(serial);
							sendLine(serial, cmd);
							return readLine(serial, out, outSize);
						}

						static bool hasHexPrefix(const char* value)
						{
							return value
								&& value[0] == static_cast<char>(CodeASCII::Zero)
								&& (value[1] == static_cast<char>(CodeHex::PrefixLower)
									|| value[1] == static_cast<char>(CodeHex::PrefixUpper));
						}
					};
				}
			}
		}
	}
}
#endif