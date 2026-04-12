#ifndef _INERTIA_MODEL_h
#define _INERTIA_MODEL_h

#include <stdint.h>

#include <IntegerSignal.h>
#include <IntegerTrigonometry.h>

namespace Inertia
{
	using namespace IntegerSignal::Trigonometry;
	using IntegerSignal::Trigonometry::angle_t;

	namespace Model
	{
		// Abstract 16 bit 3D vector.
		struct vector16_t
		{
			int16_t x;
			int16_t y;
			int16_t z;
		};

		// Abstract 32 bit 3D vector.
		struct vector32_t
		{
			int32_t x;
			int32_t y;
			int32_t z;
		};

		// Abstract timestamped 32 bit 3D vector.
		struct timestamped_vector32_t : vector32_t
		{
			uint32_t timestamp; // in microseconds.
		};

		// Abstract timestamped 16 bit 3D vector.
		struct timestamped_vector16_t : vector16_t
		{
			uint32_t timestamp; // in microseconds.
		};

		static constexpr float G_PER_ACCELERATION_UNIT = 0.001f;

		static constexpr float DEG_PER_ANGLE_UNIT = 360.0f / static_cast<float>(ANGLE_RANGE);

		// Angle units.
		// 0.05 degrees per precision, up to 32767 rotations per second.
		struct timestamped_angle_t : vector32_t
		{
			uint32_t timestamp; // in microseconds.
		};

		// Quaternion representation.
		struct quaternion_t
		{
			angle_t w;
			angle_t x;
			angle_t y;
			angle_t z;
		};

		// Timestamped quaternion.
		// In angle_t units. 0.05 degrees per precision, full 360 degrees range.
		struct timestamped_quaternion_t : quaternion_t
		{
			uint32_t timestamp; // in microseconds.
		};

		// Euler angle representation.
		struct euler_angle_t
		{
			angle_t yaw;
			angle_t pitch;
			angle_t roll;
		};

		// Timestamped Euler angles.
		// In angle_t units. 0.05 degrees per precision, full 360 degrees range.
		struct timestamped_euler_angle_t : euler_angle_t
		{
			uint32_t timestamp; // in microseconds.
		};

		/// <summary>
		/// Timestamped acceleration vector.
		/// In milli-G's (1 G = 9.80665 m/s²). 1000 milli-G's per precision, up to +-32 G's.
		/// </summary>
		using timestamped_acceleration_t = timestamped_vector16_t;

		/// <summary>
		/// Timestamped angular velocity vector.
		/// In angle_t units per second. 0.05 degrees per precision, up to 32767 rotations per second.
		/// </summary>
		using timestamped_angular_velocity_t = timestamped_vector32_t;

		// Timestamped magnetometer vector.
		// In arbitrary units. Example: 1 microteslas precision, up to 32767 microteslas.
		using timestamped_magnet_t = timestamped_vector16_t;

		/// <summary>
		/// Temperature type, from 0 to 65535 centi-Kelvin (0.01 K precision, up to 655.35 K).
		/// </summary>
		using temperature_t = uint16_t;

		/// <summary>
		/// Timestamped temperature.
		/// In centi-Kelvin.
		/// </summary>
		struct timestamped_temperature_t
		{
			temperature_t temperature;
			uint32_t timestamp; // in microseconds.
		};

		// Range 32 bit distance, in millimeters.
		using range32_t = uint32_t;

		// Range 16 bit distance, in millimeters.
		using range16_t = uint16_t;

		// Timestamped 16 bit range.
		struct timestamped_range16_t
		{
			range16_t distance;
			uint32_t timestamp; // in microseconds.
		};

		// Timestamped 32 bit range.
		struct timestamped_range32_t
		{
			range32_t distance;
			uint32_t timestamp; // in microseconds.
		};

		struct timestamped_quality_range16_t : timestamped_range16_t
		{
			uint8_t quality; // 0-255.
		};

		struct timestamped_quality_range32_t : timestamped_range32_t
		{
			uint8_t quality; // 0-255.
		};

		/// <summary>
		/// Optical flow translation.
		/// In millimeters.
		/// </summary>
		struct flow_translation_t
		{
			int32_t x; // In arbitrary units.
			int32_t y; // In arbitrary units.
		};

		// Timestamped optical flow translation.
		struct timestamped_flow_translation_t : flow_translation_t
		{
			uint32_t timestamp; // in microseconds.
		};

		struct timestamped_quality_flow_translation_t : timestamped_flow_translation_t
		{
			uint8_t quality; // 0-255.
		};

		struct overflow_timestamp16_t
		{
			uint32_t timestamp;
			uint16_t overflows;
		};

		struct millis_timestamp_t : overflow_timestamp16_t
		{
			uint32_t GetSeconds() const
			{
				// 2^32 / 1000 = 4294967.296 seconds. 
				// We use 64-bit math to avoid losing the .296ms precision per overflow
				return static_cast<uint32_t>(GetFullTimestamp() / 1000);
			}

			uint32_t GetMilliseconds() const
			{
				return timestamp;
			}

			uint64_t GetFullTimestamp() const
			{
				return (static_cast<uint64_t>(overflows) << 32) + timestamp;
			}
		};

		struct overflow_timestamp32_t
		{
			uint32_t timestamp;
			uint32_t overflows;
		};

		namespace Connection
		{
			enum class ConnectionEnum : uint8_t
			{
				Disconnected = 0,
				Connected = ((uint8_t)Disconnected) + 1
			};

			enum class ConnectingEnum : uint8_t
			{
				ScanningNew,
				PairingNew,
				ScanningExisting,
			};

			struct ConnectionCountersStruct
			{
				uint32_t RxBytes = 0;
				uint32_t TxBytes = 0;
				uint32_t RxCount = 0;
				uint32_t TxCount = 0;
				uint16_t RxError = 0;
				uint16_t TxError = 0;

				void Clear()
				{
					memset(this, 0, sizeof(ConnectionCountersStruct));
				}
			};

			struct ConnectionSessionIdStruct
			{
				uint32_t SessionId;
			};
		}

		using namespace Connection;


		namespace Log
		{
			/// <summary>
			/// Defines the types/levels of log messages.
			/// </summary>
			enum class LogTypeEnum : uint8_t
			{
				Debug,
				Info,
				Warning,
				Error
			};

			struct LogEntryStruct
			{
				// Source tag. Uniquely identifies the source or category of the log entry, such as a specific module, component, or subsystem. 
				// This allows for categorization and filtering of log entries based on their origin.
				uint32_t Tag;

				// Source instance Id. This allows for distinguishing between multiple instances of the same source type, 
				// such as multiple sensors of the same mode.
				uint8_t Instance;

				// uint8_t type. Defines the type or severity level of the log entry: debug, info, warning, or error.
				LogTypeEnum Type;

				// Code. User-defined code that indicates specific events, error codes, or state changes within the source.
				uint8_t Code;

				// Value. Can provide additional information relevant to the log entry.
				uint8_t Value;
			};

			static constexpr size_t LogEntrySize = sizeof(LogEntryStruct);
			static_assert(sizeof(LogTypeEnum) == sizeof(uint8_t), "LogTypeEnum must remain one byte.");
			static_assert(LogEntrySize == 8, "LogEntryStruct wire size changed.");

			struct LogRecordStruct : LogEntryStruct
			{
				// Rolling record ID to uniquely identify each log entry and 
				// allow for tracking the order of log entries, even across sessions and reboots.
				uint32_t RecordId;

				// Boot ID to correlate log entries within the same session, 
				// allowing for grouping and analysis of logs based on sessions.
				uint32_t BootId;

				// Boot timestamp in milliseconds, with overflow count to allow for extended uptime tracking.
				// The timestamp represents the time since the system booted, and is used to correlate log entries with system uptime and events.
				uint32_t TimestampMillis;
				uint16_t TimestampOverflows;

				// Internal CRC for the log record, calculated over the entire record except for the CRC field itself. 
				// This allows for integrity verification of log records when stored or transmitted.
				uint16_t Crc;
			};

			static constexpr size_t LogRecordSize = sizeof(LogRecordStruct);
			static_assert(LogRecordSize == 24, "LogRecordStruct wire size changed.");
		}

		using namespace Log;

		/// <summary>
		/// Converts an angle in degrees to the integer angle representation used by the system, normalizing and rounding as needed.
		/// </summary>
		/// <param name="angle">Angle in degrees. Can be negative or >= 360; the function normalizes the value into the valid integer angle range before conversion.</param>
		/// <returns>An angle_t integer representing the input angle mapped into the system's angle range (wrapped modulo ANGLE_RANGE). The conversion scales degrees by (ANGLE_RANGE / 360) and rounds to the nearest integer.</returns>
		static angle_t GetIntegerAngle(const float angle)
		{
			if (angle < 0)
			{
				return ANGLE_RANGE - GetIntegerAngle(-angle);
			}
			else if (angle >= 360)
			{
				return GetIntegerAngle(fmodf(angle, 360.0f));
			}
			else
			{
				static constexpr float FLOAT_RANGE = float(ANGLE_RANGE);

				return static_cast<angle_t>(((angle * FLOAT_RANGE) / 360.0f) + 0.5f);
			}
		}
	}
}

#endif
