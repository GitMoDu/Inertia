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

		struct velocity_t
		{
			int32_t x; // In mm/s.
			int32_t y; // In mm/s.
		};

		struct position_t
		{
			int32_t x; // In arbitrary unit, such as millimeters.
			int32_t y; // In arbitrary unit, such as millimeters.
		};

		/// <summary>
		/// Represents a 2D velocity measurement with an associated timestamp.
		/// </summary>
		struct timestamped_velocity_t : velocity_t
		{
			uint32_t timestamp; // in microseconds.
		};

		struct timestamped_position_t : position_t
		{
			uint32_t timestamp; // in microseconds.
		};

		// 0-255, representing the effort level, where 0 indicates no effort and 255 indicates maximum effort.
		struct effort_t
		{
			//The specific interpretation of the effort value can be defined by the implementation.
			uint8_t value;
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

			overflow_timestamp16_t(const uint32_t timestamp, const uint16_t overflows)
				: timestamp(timestamp)
				, overflows(overflows)
			{}
		};

		struct overflow_timestamp32_t
		{
			uint32_t timestamp;
			uint32_t overflows;

			overflow_timestamp32_t(const uint32_t timestamp, const uint32_t overflows)
				: timestamp(timestamp)
				, overflows(overflows)
			{}
		};


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
				return GetIntegerAngle(static_cast<float>(fmodf(angle, 360.0f)));
			}
			else
			{
				static constexpr float FLOAT_RANGE = float(ANGLE_RANGE);

				return static_cast<angle_t>(((angle * FLOAT_RANGE) / 360.0f) + 0.5f);
			}
		}

		static angle_t GetIntegerAngle(const int32_t angle)
		{
			if (angle < 0)
			{
				return ANGLE_RANGE - GetIntegerAngle(-angle);
			}
			else if (angle >= 360)
			{
				return GetIntegerAngle(angle % 360);
			}
			else
			{
				static constexpr float FLOAT_RANGE = float(ANGLE_RANGE);
				return static_cast<angle_t>((angle * FLOAT_RANGE) / 360);
			}
		}

	}
}

#endif
