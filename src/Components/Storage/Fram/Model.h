#ifndef _INERTIA_COMPONENTS_STORAGE_FRAM_h
#define _INERTIA_COMPONENTS_STORAGE_FRAM_h

#include "../../../Components/Core/Primitives.h"
#include "../../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace Fram
			{
				static constexpr uint32_t LOG_TAG = 2691980192; // Random unique tag for FRAM logs.

				enum class LogCodeEnum : uint8_t
				{
					EnumCount
				};

				struct IFramDriver : Inertia::Components::Lifecycle::ILifecycleDriver
				{
					~IFramDriver() = default;

					virtual bool IsStarted() const = 0;
					virtual bool Read(const uint16_t memoryAddress, uint8_t* buffer, size_t length) = 0;
					virtual bool Write(const uint16_t memoryAddress, const uint8_t* buffer, size_t length) = 0;
				};
			}
		}
	}
}
#endif