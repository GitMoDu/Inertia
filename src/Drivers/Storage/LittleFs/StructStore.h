#ifndef _INERTIA_DRIVERS_STORAGE_LITTLEFS_STRUCT_STORE_h
#define _INERTIA_DRIVERS_STORAGE_LITTLEFS_STRUCT_STORE_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "../../../Components/Storage/LittleFs/Model.h"
#include <LittleFS.h>
#include <Fletcher16.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace LittleFs
			{
				using namespace Inertia::Components::Storage::LittleFs;

				template<typename DataType, uint16_t Version = 1>
				class StructStore
				{
				private:
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t DataOffset = CrcOffset + sizeof(uint16_t);
					static constexpr size_t FileSize = DataOffset + sizeof(DataType);

					static constexpr uint32_t ChecksumSeed =
						(static_cast<uint32_t>(Version) << 16) ^ static_cast<uint32_t>(sizeof(DataType)) ^ 1342608689;

				private:
					const char* Path;
					Fletcher16  Hasher{};

				public:
					explicit StructStore(const char* path)
						: Path(path)
					{}

					bool Write(const DataType& data)
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

					bool Read(DataType& data)
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
					bool WritePayload(File& file, const DataType& data)
					{
						if (!file.seek(0)) { return false; }

						uint8_t buf[FileSize]{};
						WriteUint16(buf + VersionOffset, Version);
						WriteUint16(buf + CrcOffset, ComputeCrc(data));
						memcpy(buf + DataOffset, &data, sizeof(DataType));

						return file.write(buf, FileSize) == FileSize;
					}

					bool TryReadPayload(File& file, DataType& data)
					{
						if (file.size() != FileSize || !file.seek(0)) { return false; }

						uint8_t buf[FileSize]{};
						if (file.read(buf, FileSize) != FileSize) { return false; }

						if (ReadUint16(buf + VersionOffset) != Version) { return false; }

						DataType candidate{};
						memcpy(&candidate, buf + DataOffset, sizeof(DataType));

						if (ReadUint16(buf + CrcOffset) != ComputeCrc(candidate)) { return false; }

						data = candidate;
						return true;
					}

					uint16_t ComputeCrc(const DataType& data)
					{
						uint8_t seed[sizeof(uint32_t)]{};
						WriteUint32(seed, ChecksumSeed);

						Hasher.begin();
						Hasher.add(seed, sizeof(seed));
						Hasher.add(reinterpret_cast<const uint8_t*>(&data), sizeof(DataType));
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