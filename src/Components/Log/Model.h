#ifndef _INERTIA_COMPONENTS_LOG_MODEL_h
#define _INERTIA_COMPONENTS_LOG_MODEL_h

#include "../../Framework/Interface.h"

namespace Inertia
{
	namespace Components
	{
		namespace Log
		{
			static constexpr uint32_t LOG_TAG = 985820946; // Random unique tag for Log meta logs.

			static constexpr uint32_t ENTRY_CRC_SEED = LOG_TAG; // Use the same unique tag as a seed for log entry CRC calculations.

			enum class LogCodeEnum : uint8_t
			{
				ErrorRepositoryInvalidEntry, // Attempted to read an invalid log entry from a repository.
				ErrorCorruptedEntry // A log entry was found to be corrupted during retrieval from the repository.
			};
		}
	}
}
#endif