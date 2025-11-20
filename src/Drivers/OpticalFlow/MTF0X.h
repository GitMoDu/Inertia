#ifndef _INERTIA_DRIVERS_MTF_0X_h
#define _INERTIA_DRIVERS_MTF_0X_h

#include "../../Framework/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace MTF0X
		{
			static constexpr uint32_t MTF01_BAUDRATE = 115200;

			template<typename SerialType,
				uint8_t MaxReadBytes = 32>
			class TemplateDriver : public Model::IPeriodicDriver
				, public Model::IDataSource<Model::timestamped_flow_translation_t>
				, public Model::IDataSource<Model::timestamped_range16_t>
			{
			public:
				using DataType1 = Model::timestamped_flow_translation_t;
				using DataType2 = Model::timestamped_range16_t;

			private:
				// Combined data type for both range and flow.
				struct DataCacheType : Model::timestamped_flow_translation_t
				{
					Model::range16_t range; // in millimeters.
				};

			private:
				static constexpr uint8_t MICOLINK_MSG_HEAD = 0xEF;  // Start byte of the Micolink packet
				static constexpr uint8_t MICOLINK_MSG_ID_RANGE_SENSOR = 0x51; // Message ID for combined Range/Flow data

				// Define the structure for the payload data (Message ID 0x51)
				// The fields are inferred based on the sensor's functionality:
				// Range (uint16_t), Flow X/Y (int16_t), and Quality (uint8_t).
				struct MicoAirData
				{
					uint16_t range_mm;          // Range in millimeters (0-8000mm)
					int16_t flow_x_cmas;        // Optical Flow X velocity (cm/s @ 1m height)
					int16_t flow_y_cmas;        // Optical Flow Y velocity (cm/s @ 1m height)
					uint8_t quality;            // Quality (0-255, higher is better)
					uint8_t reserved;           // Reserved/Status byte
				};

				// Define the full packet structure for easy parsing
				struct MicolinkPacket
				{
					uint8_t header;         // 0xEF
					uint8_t msg_id;         // 0x51
					uint8_t length;         // Length of the payload (sizeof(MicoAirData))
					MicoAirData data;       // The actual sensor data payload
					uint8_t checksum;       // 8-bit XOR checksum of bytes 1 through (Length + 2)
				};

			private:
				// State machine variables
				enum class StateEnum : uint8_t
				{
					STATE_WAIT_FOR_HEADER,
					STATE_WAIT_FOR_MSG_ID,
					STATE_WAIT_FOR_LENGTH,
					STATE_READ_PAYLOAD,
					STATE_WAIT_FOR_CHECKSUM
				};

			private:
				SerialType& SerialInstance;

				StateEnum State = StateEnum::STATE_WAIT_FOR_HEADER;

				MicolinkPacket CurrentPacket{};
				uint8_t CurrentPacketSize{};
				uint8_t CurrentChecksum{};

			private:
				DataCacheType LatestData{};
				bool DataAvailable = false;

			public:
				TemplateDriver(SerialType& serial_port)
					: Model::IPeriodicDriver()
					, Model::IDataSource<Model::timestamped_flow_translation_t>()
					, Model::IDataSource<Model::timestamped_range16_t>()
					, SerialInstance(serial_port)
				{
				}

				bool GetData(Model::timestamped_flow_translation_t& out_data) final
				{
					if (DataAvailable)
					{
						memcpy(&out_data, &LatestData, sizeof(Model::timestamped_flow_translation_t));
						return true;
					}

					return false;
				}

				bool GetData(Model::timestamped_range16_t& out_data) final
				{
					if (DataAvailable)
					{
						out_data.distance = LatestData.range;
						out_data.timestamp = LatestData.timestamp;
						return true;
					}

					return false;
				}

				bool Start() final
				{
					DataAvailable = false;
					State = StateEnum::STATE_WAIT_FOR_HEADER;

					if (SerialInstance)
					{
						SerialInstance.begin(MTF01_BAUDRATE);

						//TODO: Initializate MTF to MTS protocol and fixed output rate.
						//Serial.println("MTF-01 Driver Initialized. Waiting for data...");

						return true;
					}

					return false;
				}

				void Stop() final
				{
					DataAvailable = false;
					SerialInstance.end();
				}

				void Step()
				{
					uint8_t readCount = 0;
					while (readCount < MaxReadBytes
						&& SerialInstance.available())
					{
						readCount++;
						const uint8_t incoming_byte = SerialInstance.read();

						switch (State)
						{
						case StateEnum::STATE_WAIT_FOR_HEADER:
							if (incoming_byte == MICOLINK_MSG_HEAD)
							{
								CurrentPacket.header = incoming_byte;
								CurrentChecksum = 0; // Checksum starts *after* the header
								State = StateEnum::STATE_WAIT_FOR_MSG_ID;
							}
							break;

						case StateEnum::STATE_WAIT_FOR_MSG_ID:
							CurrentPacket.msg_id = incoming_byte;
							CurrentChecksum ^= incoming_byte;
							if (incoming_byte == MICOLINK_MSG_ID_RANGE_SENSOR)
							{
								State = StateEnum::STATE_WAIT_FOR_LENGTH;
							}
							else
							{
								// Unknown message ID, reset parser
								State = StateEnum::STATE_WAIT_FOR_HEADER;
							}
							break;

						case StateEnum::STATE_WAIT_FOR_LENGTH:
							CurrentPacket.length = incoming_byte;
							CurrentChecksum ^= incoming_byte;

							// Check if the reported length is what we expect
							if (CurrentPacket.length == sizeof(MicoAirData))
							{
								CurrentPacketSize = 0;
								State = StateEnum::STATE_READ_PAYLOAD;
							}
							else
							{
								// Length mismatch, reset parser
								//Serial.println("Error: Length mismatch. Resetting...");
								State = StateEnum::STATE_WAIT_FOR_HEADER;
							}
							break;

						case StateEnum::STATE_READ_PAYLOAD:
							// Read data directly into the payload struct
							((uint8_t*)&CurrentPacket.data)[CurrentPacketSize] = incoming_byte;
							CurrentChecksum ^= incoming_byte;
							CurrentPacketSize++;

							if (CurrentPacketSize >= sizeof(MicoAirData))
							{
								State = StateEnum::STATE_WAIT_FOR_CHECKSUM;
							}
							break;

						case StateEnum::STATE_WAIT_FOR_CHECKSUM:
							CurrentPacket.checksum = incoming_byte;

							// Checksum validation
							if (CurrentPacket.checksum == CurrentChecksum)
							{
								LatestData.timestamp = micros();
								LatestData.range = CurrentPacket.data.range_mm;
								LatestData.quality = CurrentPacket.data.quality;
								LatestData.x = static_cast<int32_t>(CurrentPacket.data.flow_x_cmas) * 10; // convert cm/s @1m to mm/s
								LatestData.y = static_cast<int32_t>(CurrentPacket.data.flow_y_cmas) * 10; // convert cm/s @1m to mm/s

								if (!DataAvailable)
									DataAvailable = true;
							}
							else
							{
								/*Serial.print("Error: Bad Checksum! Expected: 0x");
								Serial.print(CurrentChecksum, HEX);
								Serial.print(", Received: 0x");
								Serial.println(CurrentPacket.checksum, HEX);*/
							}

							// Packet complete, regardless of checksum result, restart search
							State = StateEnum::STATE_WAIT_FOR_HEADER;
							break;
						}
					}
				}
			};
		}
	}
}

#endif