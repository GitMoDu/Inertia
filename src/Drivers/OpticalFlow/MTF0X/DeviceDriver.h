#ifndef _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_DEVICE_DRIVER_h
#define _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_DEVICE_DRIVER_h

// Uncomment the following line to enable detailed diagnostics for the MTF-0X driver, including sync counts, packet counts, CRC errors, and last packet details. Use this for debugging purposes, but be aware that it may impact performance due to serial printing.
//#define MTF_0X_DEBUG

#include "Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace OpticalFlow
		{
			namespace MTF0X
			{
				namespace Device
				{
					static constexpr uint32_t MTF01_BAUDRATE = 115200;

					class Driver
					{
					private:
						static constexpr uint32_t ErrorLogIntervalMillis = 1000;

					private:
						enum class StateEnum : uint8_t
						{
							STATE_WAIT_FOR_HEADER_1,
							STATE_WAIT_FOR_HEADER_2,
							STATE_WAIT_FOR_DIRECTION,
							STATE_WAIT_FOR_FLAGS,
							STATE_WAIT_FOR_FUNCTION_LOW,
							STATE_WAIT_FOR_FUNCTION_HIGH,
							STATE_WAIT_FOR_LENGTH_LOW,
							STATE_WAIT_FOR_LENGTH_HIGH,
							STATE_READ_PAYLOAD,
							STATE_WAIT_FOR_CHECKSUM
						};

					private:
						static constexpr uint8_t MSP_V2_HEADER_1 = '$';
						static constexpr uint8_t MSP_V2_HEADER_2 = 'X';
						static constexpr uint8_t MSP_V2_DIRECTION_FROM_DEVICE = '<';
#if defined(MTF_0X_DEBUG)
						static constexpr uint32_t DIAGNOSTIC_INTERVAL_MILLIS = 1000;
#endif

						static constexpr uint16_t MSP_V2_MSG_ID_RANGE_SENSOR = 0x1F01;
						static constexpr uint16_t MSP_V2_MSG_ID_OPTICAL_FLOW = 0x1F02;

						// Observed MSP v2 payload sizes from the sensor stream.
						static constexpr uint16_t RANGE_PAYLOAD_SIZE = 5;
						static constexpr uint16_t FLOW_PAYLOAD_SIZE = 9;
						static constexpr uint16_t MAX_PAYLOAD_SIZE = FLOW_PAYLOAD_SIZE;

					private:
						StateEnum State = StateEnum::STATE_WAIT_FOR_HEADER_1;

						uint8_t CurrentFlags{};
						uint16_t CurrentFunction{};
						uint16_t CurrentPayloadSize{};
						uint16_t CurrentPayloadIndex{};
						uint8_t CurrentChecksum{};
						uint8_t CurrentPayload[MAX_PAYLOAD_SIZE]{};

					private:
						Model::timestamped_quality_flow_translation_t FlowData{};
						Model::timestamped_quality_range16_t RangeData{};
						bool FlowDataAvailable = false;
						bool RangeDataAvailable = false;

#if defined(MTF_0X_DEBUG)
						uint32_t LastDiagnosticMillis = 0;
#endif
						uint32_t SyncCount = 0;
						uint32_t FrameHeaderCount = 0;
						uint32_t RangePacketCount = 0;
						uint32_t FlowPacketCount = 0;
						uint32_t CrcErrorCount = 0;
						uint32_t OversizePayloadCount = 0;
						uint32_t LastOversizePayloadLogMillis = 0;
						uint32_t LastPayloadCrcLogMillis = 0;
						uint32_t LastUnexpectedPayloadSizeLogMillis = 0;
						uint16_t LastFunction = 0;
						uint16_t LastPayloadSize = 0;
						uint8_t LastChecksumExpected = 0;
						uint8_t LastChecksumReceived = 0;

					public:
						Inertia::Model::ILogListener* LogListener = nullptr;
						uint8_t InstanceId = 0;

					public:
						Driver() {}

						bool GetFlow(Model::timestamped_quality_flow_translation_t& out_data)
						{
							if (FlowDataAvailable)
							{
								memcpy(&out_data, &FlowData, sizeof(Model::timestamped_quality_flow_translation_t));
								return true;
							}

							return false;
						}

						bool GetRange(Model::timestamped_quality_range16_t& out_data)
						{
							if (RangeDataAvailable)
							{
								memcpy(&out_data, &RangeData, sizeof(Model::timestamped_quality_range16_t));
								return true;
							}

							return false;
						}

						bool Start()
						{
							FlowDataAvailable = false;
							RangeDataAvailable = false;
							RangeData.distance = 0;
							RangeData.timestamp = 0;
							FlowData.x = 0;
							FlowData.y = 0;
							FlowData.quality = 0;
							FlowData.timestamp = 0;

#if defined(MTF_0X_DEBUG)
							LastDiagnosticMillis = 0;
#endif
							SyncCount = 0;
							FrameHeaderCount = 0;
							RangePacketCount = 0;
							FlowPacketCount = 0;
							CrcErrorCount = 0;
							OversizePayloadCount = 0;
							LastOversizePayloadLogMillis = 0;
							LastPayloadCrcLogMillis = 0;
							LastUnexpectedPayloadSizeLogMillis = 0;
							LastFunction = 0;
							LastPayloadSize = 0;
							LastChecksumExpected = 0;
							LastChecksumReceived = 0;
							ResetParser();

#if defined(MTF_0X_DEBUG)

							Serial.println(F("MTF-01 MSP v2 driver initialized with sparse diagnostics."));
#endif

							return true;
						}

						void Stop()
						{
							FlowDataAvailable = false;
							RangeDataAvailable = false;
							ResetParser();
						}

						bool Parse(const uint8_t incoming_byte)
						{
							bool is_new_packet = false;
							switch (State)
							{
							case StateEnum::STATE_WAIT_FOR_HEADER_1:
								if (incoming_byte == MSP_V2_HEADER_1)
								{
									State = StateEnum::STATE_WAIT_FOR_HEADER_2;
								}
								break;

							case StateEnum::STATE_WAIT_FOR_HEADER_2:
								if (incoming_byte == MSP_V2_HEADER_2)
								{
									State = StateEnum::STATE_WAIT_FOR_DIRECTION;
								}
								else
								{
									ResetParserWithResync(incoming_byte);
								}
								break;

							case StateEnum::STATE_WAIT_FOR_DIRECTION:
								if (incoming_byte == MSP_V2_DIRECTION_FROM_DEVICE)
								{
									CurrentChecksum = 0;
									SyncCount++;
									State = StateEnum::STATE_WAIT_FOR_FLAGS;
								}
								else
								{
									ResetParserWithResync(incoming_byte);
								}
								break;

							case StateEnum::STATE_WAIT_FOR_FLAGS:
								CurrentFlags = incoming_byte;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								State = StateEnum::STATE_WAIT_FOR_FUNCTION_LOW;
								break;

							case StateEnum::STATE_WAIT_FOR_FUNCTION_LOW:
								CurrentFunction = incoming_byte;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								State = StateEnum::STATE_WAIT_FOR_FUNCTION_HIGH;
								break;

							case StateEnum::STATE_WAIT_FOR_FUNCTION_HIGH:
								CurrentFunction |= static_cast<uint16_t>(incoming_byte) << 8;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								State = StateEnum::STATE_WAIT_FOR_LENGTH_LOW;
								break;

							case StateEnum::STATE_WAIT_FOR_LENGTH_LOW:
								CurrentPayloadSize = incoming_byte;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								State = StateEnum::STATE_WAIT_FOR_LENGTH_HIGH;
								break;

							case StateEnum::STATE_WAIT_FOR_LENGTH_HIGH:
								CurrentPayloadSize |= static_cast<uint16_t>(incoming_byte) << 8;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								LastFunction = CurrentFunction;
								LastPayloadSize = CurrentPayloadSize;

								if (CurrentPayloadSize > MAX_PAYLOAD_SIZE)
								{
									OversizePayloadCount++;
									Log(Inertia::Model::LogTypeEnum::Warning,
										Model::LogCodeEnum::ErrorOversizePayload,
										static_cast<uint8_t>(CurrentPayloadSize));
									ResetParserWithResync(incoming_byte);
								}
								else if (CurrentPayloadSize == 0)
								{
									FrameHeaderCount++;
									State = StateEnum::STATE_WAIT_FOR_CHECKSUM;
								}
								else
								{
									FrameHeaderCount++;
									CurrentPayloadIndex = 0;
									State = StateEnum::STATE_READ_PAYLOAD;
								}
								break;

							case StateEnum::STATE_READ_PAYLOAD:
								CurrentPayload[CurrentPayloadIndex] = incoming_byte;
								CurrentChecksum = UpdateChecksum(CurrentChecksum, incoming_byte);
								CurrentPayloadIndex++;

								if (CurrentPayloadIndex >= CurrentPayloadSize)
								{
									State = StateEnum::STATE_WAIT_FOR_CHECKSUM;
								}
								break;

							case StateEnum::STATE_WAIT_FOR_CHECKSUM:
								if (incoming_byte == CurrentChecksum)
								{
									CommitPacket();
									is_new_packet = true;
								}
								else
								{
									CrcErrorCount++;
									LastChecksumReceived = incoming_byte;
									LastChecksumExpected = CurrentChecksum;
									Log(Inertia::Model::LogTypeEnum::Warning,
										Model::LogCodeEnum::ErrorPayloadCrc,
										incoming_byte);
								}

								ResetParser();
								break;
							}

							return is_new_packet;
						}

					private:
						static uint8_t UpdateChecksum(uint8_t checksum, const uint8_t data)
						{
							checksum ^= data;
							for (uint8_t i = 0; i < 8; i++)
							{
								if (checksum & 0x80)
								{
									checksum = static_cast<uint8_t>((checksum << 1) ^ 0xD5);
								}
								else
								{
									checksum <<= 1;
								}
							}

							return checksum;
						}

						static int32_t ReadInt32LE(const uint8_t* data)
						{
							return static_cast<int32_t>(
								((static_cast<uint32_t>(data[0]) << 0))
								| ((static_cast<uint32_t>(data[1]) << 8))
								| ((static_cast<uint32_t>(data[2]) << 16))
								| ((static_cast<uint32_t>(data[3]) << 24)));
						}

						void Log(const Inertia::Model::LogTypeEnum type,
							const Model::LogCodeEnum code,
							const uint8_t value)
						{
							if (LogListener != nullptr && ShouldLog(code))
							{
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = Model::LOG_TAG,
									.Instance = InstanceId,
									.Type = type,
									.Code = static_cast<uint8_t>(code),
									.Value = value
									});
							}
						}

						bool ShouldLog(const Model::LogCodeEnum code)
						{
							uint32_t* lastLogMillis = nullptr;
							switch (code)
							{
							case Model::LogCodeEnum::ErrorOversizePayload:
								lastLogMillis = &LastOversizePayloadLogMillis;
								break;
							case Model::LogCodeEnum::ErrorPayloadCrc:
								lastLogMillis = &LastPayloadCrcLogMillis;
								break;
							case Model::LogCodeEnum::ErrorUnexpectedPayloadSize:
								lastLogMillis = &LastUnexpectedPayloadSizeLogMillis;
								break;
							default:
								return true;
							}

							const uint32_t now = millis();
							if ((now - *lastLogMillis) < ErrorLogIntervalMillis)
							{
								return false;
							}

							*lastLogMillis = now;
							return true;
						}

						void ResetParser()
						{
							State = StateEnum::STATE_WAIT_FOR_HEADER_1;
							CurrentFlags = 0;
							CurrentFunction = 0;
							CurrentPayloadSize = 0;
							CurrentPayloadIndex = 0;
							CurrentChecksum = 0;
						}

						void ResetParserWithResync(const uint8_t incoming_byte)
						{
							ResetParser();
							if (incoming_byte == MSP_V2_HEADER_1)
							{
								State = StateEnum::STATE_WAIT_FOR_HEADER_2;
							}
						}

						void CommitRangePacket()
						{
							const uint8_t quality = CurrentPayload[0];
							const int32_t distance_mm = ReadInt32LE(&CurrentPayload[1]);
							const uint32_t clamped_distance_mm = distance_mm <= 0
								? 0u
								: (static_cast<uint32_t>(distance_mm) > 0xFFFFu
									? 0xFFFFu
									: static_cast<uint32_t>(distance_mm));

							RangeData.distance = static_cast<Model::range16_t>(clamped_distance_mm);
							RangeData.timestamp = micros();
							RangeData.quality = quality;
							RangeDataAvailable = true;
							RangePacketCount++;
						}

						void CommitFlowPacket()
						{
							FlowData.quality = CurrentPayload[0];
							FlowData.x = ReadInt32LE(&CurrentPayload[1]);
							FlowData.y = ReadInt32LE(&CurrentPayload[5]);
							FlowData.timestamp = micros();
							FlowDataAvailable = true;
							FlowPacketCount++;
						}

						void CommitPacket()
						{
							switch (CurrentFunction)
							{
							case MSP_V2_MSG_ID_RANGE_SENSOR:
								if (CurrentPayloadSize == RANGE_PAYLOAD_SIZE)
								{
									CommitRangePacket();
								}
								else
								{
									Log(Inertia::Model::LogTypeEnum::Warning,
										Model::LogCodeEnum::ErrorUnexpectedPayloadSize,
										static_cast<uint8_t>(CurrentPayloadSize));
								}
								break;

							case MSP_V2_MSG_ID_OPTICAL_FLOW:
								if (CurrentPayloadSize == FLOW_PAYLOAD_SIZE)
								{
									CommitFlowPacket();
								}
								else
								{
									Log(Inertia::Model::LogTypeEnum::Warning,
										Model::LogCodeEnum::ErrorUnexpectedPayloadSize,
										static_cast<uint8_t>(CurrentPayloadSize));
								}
								break;

							default:
								break;
							}
						}
					};
				}
			}
		}
	}
}

#endif