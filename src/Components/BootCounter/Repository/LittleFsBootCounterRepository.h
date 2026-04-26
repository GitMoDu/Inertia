#ifndef _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h
#define _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include "../Model.h"
#include "../../Storage/LittleFs/StructStore.h"

namespace Inertia
{
	namespace Components
	{
		namespace BootCounter
		{
			namespace Repository
			{
				class LittleFsBootCounterRepository
					: public Inertia::Model::IBootCounterRepository
				{
				private:
					struct stored_boot_count_t
					{
						uint32_t Count = 0;
					};

					static constexpr uint16_t StoreVersion = 1;
					using Store = Inertia::Components::Storage::LittleFs::StructStore<stored_boot_count_t, StoreVersion>;

					Store FileStore;
					uint32_t BootCount = 0;

				public:
					explicit LittleFsBootCounterRepository(const char* path = "/boot-count.bin")
						: Inertia::Model::IBootCounterRepository()
						, FileStore(path)
					{}

					void Stop() override
					{
						// No-op.
					}

					bool Start() override
					{
						stored_boot_count_t record{};

						if (FileStore.Exists())
						{
							if (!FileStore.Read(record))
							{
								if (!ResetStorageFile())
								{
									return false;
								}

								record = {};
							}
						}

						record.Count++;
						BootCount = record.Count;

						if (!FileStore.Write(record))
						{
							return false;
						}

						return true;
					}

					bool ResetStorageFile()
					{
						BootCount = 0;
						return FileStore.Delete();
					}

					virtual uint32_t GetCounter() override
					{
						return BootCount;
					}
				};
			}
		}
	}
}

#endif