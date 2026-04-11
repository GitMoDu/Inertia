#ifndef _INERTIA_COMPONENTS_BOOT_COUNTER_NO_BOOT_COUNTER_REPOSITORY_h
#define _INERTIA_COMPONENTS_BOOT_COUNTER_NO_BOOT_COUNTER_REPOSITORY_h

#include "../Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace BootCounter
		{
			namespace Repository
			{
				class NoBootCounterRepository
					: public Inertia::Model::IBootCounterRepository
				{
				public:
					NoBootCounterRepository()
						: Inertia::Model::IBootCounterRepository()
					{}

					bool Setup()
					{
						return true;
					}

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