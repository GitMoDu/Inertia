#ifndef _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h
#define _INERTIA_COMPONENTS_BOOT_COUNTER_LITTLE_FS_BOOT_COUNTER_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <LittleFS.h>
#include "../Model.h"

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
					const char* Path;
					uint32_t BootCount = 0;

				public:
					explicit LittleFsBootCounterRepository(const char* path = "/boot-count.bin")
						: Inertia::Model::IBootCounterRepository()
						, Path(path)
					{}

					bool Setup()
					{
						if (!LittleFS.begin())
						{
							return false;
						}

						uint32_t storedBootCount = 0;

						if (LittleFS.exists(Path))
						{
							File readFile = LittleFS.open(Path, "r");
							if (!readFile)
							{
								if (!ResetStorageFile())
								{
									LittleFS.end();
									return false;
								}
							}
							else
							{
								const size_t bytesRead = readFile.read(
									reinterpret_cast<uint8_t*>(&storedBootCount),
									sizeof(storedBootCount));
								readFile.close();

								if (bytesRead != sizeof(storedBootCount))
								{
									if (!ResetStorageFile())
									{
										LittleFS.end();
										return false;
									}
								}
							}
						}

						BootCount = storedBootCount + 1;

						File writeFile = LittleFS.open(Path, "w");
						if (!writeFile)
						{
							LittleFS.end();
							return false;
						}

						const size_t bytesWritten = writeFile.write(
							reinterpret_cast<const uint8_t*>(&BootCount),
							sizeof(BootCount));
						writeFile.flush();
						writeFile.close();
						LittleFS.end();

						return bytesWritten == sizeof(BootCount);
					}

					bool ResetStorageFile()
					{
						BootCount = 0;
						if (LittleFS.exists(Path) && !LittleFS.remove(Path))
						{
							return false;
						}

						return true;
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