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

			enum class LogCodeEnum : uint8_t
			{
				ErrorRepositoryInvalidEntry, // Attempted to read an invalid log entry from a repository.
				ErrorCorruptedEntry // A log entry was found to be corrupted during retrieval from the repository.
			};
		}
	}
}
#endif