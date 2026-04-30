#ifndef _INERTIA_DRIVERS_BOOT_COUNTER_NO_BOOT_COUNTER_REPOSITORY_h
#define _INERTIA_DRIVERS_BOOT_COUNTER_NO_BOOT_COUNTER_REPOSITORY_h

#include "../../../Components/BootCounter/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace BootCounter
		{
			namespace Repository
			{
				using namespace Inertia::Components::BootCounter;

				class NoBootCounterRepository
					: public Inertia::Components::BootCounter::IBootCounterRepository
				{
				public:
					NoBootCounterRepository()
						: Inertia::Components::BootCounter::IBootCounterRepository()
					{}

					bool Start() override
					{
						return true;
					}

					void Stop() override
					{}

					virtual uint32_t GetCounter() override
					{
						return 0;
					}
				};
			}
		}
	}
}
#endif