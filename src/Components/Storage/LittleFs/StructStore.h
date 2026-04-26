#ifndef _INERTIA_COMPONENTS_STORAGE_LITTLEFS_STRUCT_STORE_h
#define _INERTIA_COMPONENTS_STORAGE_LITTLEFS_STRUCT_STORE_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "Model.h"
#include <LittleFS.h>
#include <Fletcher16.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace LittleFs
			{
				template<typename TData, uint16_t TVersion = 1>
				class StructStore
				{
				private:
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t DataOffset = CrcOffset + sizeof(uint16_t);
					static constexpr size_t FileSize = DataOffset + sizeof(TData);

					static constexpr uint32_t ChecksumSeed =
						(static_cast<uint32_t>(TVersion) << 16) ^ static_cast<uint32_t>(sizeof(TData)) ^ 1342608689;

				private:
					const char* Path;
					Fletcher16  Hasher{};

				public:
					explicit StructStore(const char* path)
						: Path(path)
					{}

					bool Write(const TData& data)
					{
						if (LittleFS.exists(Path) && !LittleFS.remove(Path)) { return false; }

						File file = LittleFS.open(Path, "w");
						if (!file) { return false; }

						const bool ok = WritePayload(file, data);
						file.flush();
						file.close();

						if (!ok) { LittleFS.remove(Path); }
						return ok;
					}

					bool Read(TData& data)
					{
						File file = LittleFS.open(Path, "r");
						if (!file) { return false; }

						const bool ok = TryReadPayload(file, data);
						file.close();
						return ok;
					}

					bool Exists() const
					{
						return LittleFS.exists(Path);
					}

					bool Delete()
					{
						if (!LittleFS.exists(Path)) { return true; }
						return LittleFS.remove(Path);
					}

				private:
					bool WritePayload(File& file, const TData& data)
					{
						if (!file.seek(0)) { return false; }

						uint8_t buf[FileSize]{};
						WriteUint16(buf + VersionOffset, TVersion);
						WriteUint16(buf + CrcOffset, ComputeCrc(data));
						memcpy(buf + DataOffset, &data, sizeof(TData));

						return file.write(buf, FileSize) == FileSize;
					}

					bool TryReadPayload(File& file, TData& data)
					{
						if (file.size() != FileSize || !file.seek(0)) { return false; }

						uint8_t buf[FileSize]{};
						if (file.read(buf, FileSize) != FileSize) { return false; }

						if (ReadUint16(buf + VersionOffset) != TVersion) { return false; }

						TData candidate{};
						memcpy(&candidate, buf + DataOffset, sizeof(TData));

						if (ReadUint16(buf + CrcOffset) != ComputeCrc(candidate)) { return false; }

						data = candidate;
						return true;
					}

					uint16_t ComputeCrc(const TData& data)
					{
						uint8_t seed[sizeof(uint32_t)]{};
						WriteUint32(seed, ChecksumSeed);

						Hasher.begin();
						Hasher.add(seed, sizeof(seed));
						Hasher.add(reinterpret_cast<const uint8_t*>(&data), sizeof(TData));
						return Hasher.getFletcher();
					}

					static void WriteUint16(uint8_t* dst, const uint16_t v)
					{
						dst[0] = static_cast<uint8_t>(v & 0xFFu);
						dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
					}

					static void WriteUint32(uint8_t* dst, const uint32_t v)
					{
						dst[0] = static_cast<uint8_t>(v & 0xFFu);
						dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
						dst[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
						dst[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
					}

					static uint16_t ReadUint16(const uint8_t* src)
					{
						return static_cast<uint16_t>(src[0])
							| static_cast<uint16_t>(static_cast<uint16_t>(src[1]) << 8);
					}
				};
			}
		}
	}
}

#endif
#endif