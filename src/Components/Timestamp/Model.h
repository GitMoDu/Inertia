#ifndef _INERTIA_COMPONENTS_TIMESTAMP_MODEL_h
#define _INERTIA_COMPONENTS_TIMESTAMP_MODEL_h

#include "../../Components/Core/Primitives.h"

namespace Inertia
{
	namespace Components
	{
		namespace Timestamp
		{
			struct millis_timestamp_t : Inertia::Model::overflow_timestamp16_t
			{
				millis_timestamp_t(const uint32_t timestamp, const uint16_t overflows)
					: Inertia::Model::overflow_timestamp16_t(timestamp, overflows)
				{}

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

			struct IMillisTimestampSource
			{
				~IMillisTimestampSource() = default;

				virtual millis_timestamp_t GetMillisTimestamp() = 0;
			};
		}
	}

	// Exposes millis timestamp type.
	namespace Model
	{
		//using millis_timestamp_t = Inertia::Components::Timestamp::millis_timestamp_t;
	}
}
#endif