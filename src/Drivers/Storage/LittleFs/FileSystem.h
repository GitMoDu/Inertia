#ifndef _INERTIA_DRIVERS_STORAGE_LITTLEFS_FILESYSTEM_h
#define _INERTIA_DRIVERS_STORAGE_LITTLEFS_FILESYSTEM_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "../../../Components/Storage/LittleFs/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace LittleFs
			{
				using namespace Inertia::Components::Storage::LittleFs;

				static bool PrepareFilesystem(Inertia::Model::ILogListener* logListener = nullptr, const uint8_t instanceId = 0)
				{
					if (!LittleFS.begin())
					{
						if (logListener != nullptr)
							logListener->OnLog(Inertia::Model::LogEntryStruct{
								.Tag = LOG_TAG,
								.Instance = instanceId,
								.Type = Inertia::Model::LogTypeEnum::Warning,
								.Code = static_cast<uint8_t>(LogCodeEnum::BeginFailed),
								.Value = 0 });

						if (!LittleFS.format())
						{
							if (logListener != nullptr)
								logListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = instanceId,
									.Type = Inertia::Model::LogTypeEnum::Error,
									.Code = static_cast<uint8_t>(LogCodeEnum::FormatFailed),
									.Value = 0 });
							return false;
						}

						if (!LittleFS.begin())
						{
							if (logListener != nullptr)
								logListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = instanceId,
									.Type = Inertia::Model::LogTypeEnum::Error,
									.Code = static_cast<uint8_t>(LogCodeEnum::BeginAfterFormatFailed),
									.Value = 0 });

							return false;
						}

						if (logListener != nullptr)
							logListener->OnLog(Inertia::Model::LogEntryStruct{
								.Tag = LOG_TAG,
								.Instance = instanceId,
								.Type = Inertia::Model::LogTypeEnum::Info,
								.Code = static_cast<uint8_t>(LogCodeEnum::Mounted),
								.Value = 0 });
					}

					return true;
				}

				static void StopFilesystem(Inertia::Model::ILogListener* logListener = nullptr, const uint8_t instanceId = 0)
				{
					LittleFS.end();

					if (logListener != nullptr)
						logListener->OnLog(Inertia::Model::LogEntryStruct{
							.Tag = LOG_TAG,
							.Instance = instanceId,
							.Type = Inertia::Model::LogTypeEnum::Info,
							.Code = static_cast<uint8_t>(LogCodeEnum::Unmounted),
							.Value = 0 });
				}

				class FileSystem : public Inertia::Model::ILifecycleDriver
				{
				public:
					static FileSystem& GetStaticInstance()
					{
						static FileSystem instance{};
						return instance;
					}

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0;

				private:
					bool Mounted = false;

				public:
					FileSystem() : Inertia::Model::ILifecycleDriver() {}
					~FileSystem() = default;

					virtual bool Start() override
					{
						if (Mounted)
							return true;

						Mounted = PrepareFilesystem(LogListener, InstanceId);

						return Mounted;
					}

					virtual void Stop() override
					{
						Mounted = false;
						StopFilesystem(LogListener, InstanceId);
					}
				};
			}
		}
	}
}
#endif
#endif