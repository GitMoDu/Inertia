#ifndef _INERTIA_COMPONENTS_LOG_REPOSITORY_LITTLEFS_LOG_REPOSITORY_h
#define _INERTIA_COMPONENTS_LOG_REPOSITORY_LITTLEFS_LOG_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <LittleFS.h>

#include <Fletcher16.h>
#include "../Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Log
		{
			namespace Repository
			{
				class LittleFsLogRepository
					: public Inertia::Model::ILogRepository
				{
				private:
					struct stored_log_header_t
					{
						uint16_t Version = 0;
						uint16_t Crc = 0;
					};

					const char* Path;

				private:
					Fletcher16 Hasher{};
					size_t Count = 0;
					uint32_t NextRecordId = 1;
					bool Started = false;

					static constexpr uint16_t HeaderVersion = 1;
					static constexpr uint32_t HeaderChecksumSeed = 541873179;
					static constexpr size_t HeaderPayloadSize = sizeof(uint16_t);
					static constexpr size_t HeaderSize = HeaderPayloadSize + sizeof(uint16_t);
					static constexpr size_t RecordSize = sizeof(Inertia::Model::LogRecordStruct);
					static constexpr size_t TempPathBufferSize = 96;

				public:
					explicit LittleFsLogRepository(const char* path = "/logs.bin")
						: Inertia::Model::ILogRepository()
						, Path(path)
					{}

					~LittleFsLogRepository()
					{
						Stop();
					}

					bool Start() override
					{
						if (Started)
						{
							return true;
						}

						if (!LittleFS.begin())
						{
							return false;
						}

						Count = 0;
						NextRecordId = 1;

						if (!LittleFS.exists(Path))
						{
							return ResetStorage();
						}

						File file = LittleFS.open(Path, "r");
						if (!file)
						{
							return ResetStorage();
						}

						const size_t fileSize = file.size();
						const bool hasHeader = TryReadHeader(file);
						const bool rewriteRequired = !hasHeader;
						size_t storedCount = 0;
						size_t validCount = 0;
						bool salvageRequired = false;

						if (hasHeader)
						{
							const size_t dataSize = fileSize - HeaderSize;
							storedCount = dataSize / RecordSize;
							salvageRequired = (dataSize % RecordSize) != 0;
						}
						else
						{
							storedCount = fileSize / RecordSize;
							salvageRequired = (fileSize % RecordSize) != 0;

							if (!file.seek(0))
							{
								file.close();
								return ResetStorage();
							}
						}

						Inertia::Model::LogRecordStruct lastRecord{};
						Inertia::Model::LogRecordStruct currentRecord{};
						for (size_t i = 0; i < storedCount; ++i)
						{
							if (!ReadLogRecord(file, currentRecord)
								|| !IsValidLogRecord(currentRecord)
								|| (validCount > 0 && currentRecord.RecordId <= lastRecord.RecordId))
							{
								salvageRequired = true;
								break;
							}

							lastRecord = currentRecord;
							validCount++;
						}

						file.close();

						if ((rewriteRequired || salvageRequired) && !RewriteValidPrefix(validCount, hasHeader))
						{
							return ResetStorage();
						}

						Count = validCount;
						if (Count > 0)
						{
							const uint32_t nextRecordIdFromLog = lastRecord.RecordId + 1;
							if (nextRecordIdFromLog > NextRecordId)
							{
								NextRecordId = nextRecordIdFromLog;
							}
						}

						Started = true;
						return true;
					}

					void Stop() override
					{
						if (Started)
						{
							LittleFS.end();
						}

						Count = 0;
						NextRecordId = 1;
						Started = false;
					}

					bool AddEntry(const uint32_t bootId, const Inertia::Model::millis_timestamp_t& timestamp, const Inertia::Model::LogEntryStruct& logEntry) override
					{
						if (!EnsureInitialized() || IsFull())
						{
							return false;
						}

						if (!LittleFS.exists(Path) && !ResetStorage())
						{
							return false;
						}

						File file = LittleFS.open(Path, "a");
						if (!file)
						{
							return false;
						}

						Inertia::Model::LogRecordStruct record{};
						record.RecordId = NextRecordId;
						record.BootId = bootId;
						record.TimestampMillis = timestamp.timestamp;
						record.TimestampOverflows = timestamp.overflows;
						record.Tag = logEntry.Tag;
						record.Instance = logEntry.Instance;
						record.Type = logEntry.Type;
						record.Code = logEntry.Code;
						record.Value = logEntry.Value;

						// Calculate CRC over the record, excluding the Crc field itself, which is the last field in the struct.
						Hasher.begin();
						Hasher.add(reinterpret_cast<const uint8_t*>(&record), sizeof(record) - sizeof(record.Crc));
						record.Crc = Hasher.getFletcher();

						static_cast<Inertia::Model::LogEntryStruct&>(record) = logEntry;

						const size_t bytesWritten = file.write(
							reinterpret_cast<const uint8_t*>(&record),
							sizeof(record));
						file.flush();
						file.close();

						if (bytesWritten != sizeof(record))
						{
							return false;
						}

						++Count;
						++NextRecordId;
						return true;
					}

					bool GetRecordAt(const size_t index, Inertia::Model::LogRecordStruct& logRecord) override
					{
						if (!EnsureInitialized() || index >= Count)
						{
							return false;
						}

						File file = LittleFS.open(Path, "r");
						if (!file)
						{
							return false;
						}

						const bool success = ReadLogRecordAt(file, index, logRecord);
						file.close();
						return success;
					}

					bool GetLatestEntryId(uint32_t& recordId) override
					{
						if (!EnsureInitialized() || Count == 0 || NextRecordId == 0)
						{
							return false;
						}

						recordId = NextRecordId - 1;
						return true;
					}

					bool DeleteRecordsThrough(const uint32_t recordId) override
					{
						if (!EnsureInitialized())
						{
							return false;
						}

						if (Count == 0)
						{
							return true;
						}

						char tempPath[TempPathBufferSize]{};
						if (!TryGetTempPath(tempPath, sizeof(tempPath)))
						{
							return false;
						}

						File sourceFile = LittleFS.open(Path, "r");
						if (!sourceFile)
						{
							return false;
						}

						if (LittleFS.exists(tempPath))
						{
							LittleFS.remove(tempPath);
						}

						File tempFile = LittleFS.open(tempPath, "w");
						if (!tempFile)
						{
							sourceFile.close();
							return false;
						}

						if (!WriteHeader(tempFile)
							|| !sourceFile.seek(HeaderSize))
						{
							tempFile.close();
							sourceFile.close();
							LittleFS.remove(tempPath);
							return false;
						}

						size_t remainingCount = 0;
						Inertia::Model::LogRecordStruct record{};

						for (size_t i = 0; i < Count; ++i)
						{
							if (!ReadLogRecord(sourceFile, record))
							{
								tempFile.close();
								sourceFile.close();
								LittleFS.remove(tempPath);
								return false;
							}

							if (record.RecordId <= recordId)
							{
								continue;
							}

							if (!WriteLogRecord(tempFile, record))
							{
								tempFile.close();
								sourceFile.close();
								LittleFS.remove(tempPath);
								return false;
							}

							++remainingCount;
						}

						tempFile.flush();
						tempFile.close();
						sourceFile.close();

						if (LittleFS.exists(Path) && !LittleFS.remove(Path))
						{
							LittleFS.remove(tempPath);
							return false;
						}

						if (!LittleFS.rename(tempPath, Path))
						{
							LittleFS.remove(tempPath);
							return false;
						}

						Count = remainingCount;
						return true;
					}

					void ClearRecords() override
					{
						ResetStorage();
					}

					uint32_t GetCount() override
					{
						if (!EnsureInitialized())
						{
							return 0;
						}

						// Cap the count at UINT32_MAX, respecting interface contract.
						return Count <= UINT32_MAX ? static_cast<uint32_t>(Count) : UINT32_MAX;
					}

					uint32_t GetCapacity() override
					{
						if (!EnsureInitialized())
						{
							return 0;
						}

						FSInfo info{};
						if (!LittleFS.info(info))
						{
							return Count;
						}

						return info.totalBytes > HeaderSize
							? static_cast<uint32_t>((info.totalBytes - HeaderSize) / RecordSize)
							: 0;
					}

					bool IsFull() override
					{
						if (!EnsureInitialized())
						{
							return true;
						}

						FSInfo info{};
						if (!LittleFS.info(info))
						{
							return false;
						}

						return (info.totalBytes - info.usedBytes) < RecordSize;
					}

				private:
					bool EnsureInitialized()
					{
						return Started;
					}

					bool ResetStorage()
					{
						Count = 0;
						NextRecordId = 1;

						if (LittleFS.exists(Path) && !LittleFS.remove(Path))
						{
							LittleFS.end();
							return false;
						}

						File file = LittleFS.open(Path, "w");
						if (!file)
						{
							LittleFS.end();
							return false;
						}

						const bool success = WriteHeader(file);
						file.flush();
						file.close();

						if (!success)
						{
							LittleFS.remove(Path);
							LittleFS.end();
							return false;
						}

						Started = true;
						return true;
					}

					bool TryGetTempPath(char* buffer, const size_t bufferSize)
					{
						if (buffer == nullptr || bufferSize == 0)
						{
							return false;
						}

						const int written = snprintf(buffer, bufferSize, "%s.tmp", Path);
						return written > 0 && static_cast<size_t>(written) < bufferSize;
					}

					bool RewriteValidPrefix(const size_t validCount, const bool sourceHasHeader)
					{
						char tempPath[TempPathBufferSize]{};
						if (!TryGetTempPath(tempPath, sizeof(tempPath)))
						{
							return false;
						}

						if (LittleFS.exists(tempPath))
						{
							LittleFS.remove(tempPath);
						}

						File sourceFile = LittleFS.open(Path, "r");
						if (!sourceFile)
						{
							return false;
						}

						File tempFile = LittleFS.open(tempPath, "w");
						if (!tempFile)
						{
							sourceFile.close();
							return false;
						}

						if (!WriteHeader(tempFile)
							|| (sourceHasHeader && !sourceFile.seek(HeaderSize)))
						{
							tempFile.close();
							sourceFile.close();
							LittleFS.remove(tempPath);
							return false;
						}

						Inertia::Model::LogRecordStruct record{};
						for (size_t i = 0; i < validCount; ++i)
						{
							if (!ReadLogRecord(sourceFile, record)
								|| !WriteLogRecord(tempFile, record))
							{
								tempFile.close();
								sourceFile.close();
								LittleFS.remove(tempPath);
								return false;
							}
						}

						tempFile.flush();
						tempFile.close();
						sourceFile.close();

						if (LittleFS.exists(Path) && !LittleFS.remove(Path))
						{
							LittleFS.remove(tempPath);
							return false;
						}

						if (!LittleFS.rename(tempPath, Path))
						{
							LittleFS.remove(tempPath);
							return false;
						}

						return true;
					}

					bool IsValidLogRecord(const Inertia::Model::LogRecordStruct& logRecord)
					{
						Hasher.begin();
						Hasher.add(reinterpret_cast<const uint8_t*>(&logRecord), sizeof(logRecord) - sizeof(logRecord.Crc));
						return Hasher.getFletcher() == logRecord.Crc;
					}

					bool TryReadHeader(File& file)
					{
						if (file.size() < HeaderSize
							|| !file.seek(0))
						{
							return false;
						}

						uint8_t headerBuffer[HeaderSize]{};
						if (file.read(headerBuffer, sizeof(headerBuffer)) != sizeof(headerBuffer))
						{
							return false;
						}

						stored_log_header_t header{};
						header.Version = ReadUint16(headerBuffer);
						header.Crc = ReadUint16(headerBuffer + HeaderPayloadSize);

						return header.Version == HeaderVersion
							&& header.Crc == ComputeHeaderChecksum(header.Version);
					}

					bool WriteHeader(File& file)
					{
						uint8_t headerBuffer[HeaderSize]{};
						BuildHeader(headerBuffer);

						return file.write(headerBuffer, sizeof(headerBuffer)) == sizeof(headerBuffer);
					}

					void BuildHeader(uint8_t* const headerBuffer)
					{
						stored_log_header_t header{};
						header.Version = HeaderVersion;
						header.Crc = ComputeHeaderChecksum(header.Version);

						WriteUint16(headerBuffer, header.Version);
						WriteUint16(headerBuffer + HeaderPayloadSize, header.Crc);
					}

					uint16_t ComputeHeaderChecksum(const uint16_t version)
					{
						uint8_t payload[HeaderPayloadSize]{};
						uint8_t seedBuffer[sizeof(HeaderChecksumSeed)]{};

						WriteUint32(seedBuffer, HeaderChecksumSeed);
						WriteUint16(payload, version);

						Hasher.begin();
						Hasher.add(seedBuffer, sizeof(seedBuffer));
						Hasher.add(payload, sizeof(payload));
						return Hasher.getFletcher();
					}

					bool ReadLogRecord(File& file, Inertia::Model::LogRecordStruct& logRecord)
					{
						return file.read(
							reinterpret_cast<uint8_t*>(&logRecord),
							sizeof(logRecord)) == sizeof(logRecord);
					}

					bool ReadLogRecordAt(File& file, const size_t index, Inertia::Model::LogRecordStruct& logRecord)
					{
						if (!file.seek(HeaderSize + (index * RecordSize)))
						{
							return false;
						}

						return file.read(
							reinterpret_cast<uint8_t*>(&logRecord),
							sizeof(logRecord)) == sizeof(logRecord);
					}

					bool WriteLogRecord(File& file, const Inertia::Model::LogRecordStruct& logRecord)
					{
						return file.write(
							reinterpret_cast<const uint8_t*>(&logRecord),
							sizeof(logRecord)) == sizeof(logRecord);
					}

					static void WriteUint16(uint8_t* const destination, const uint16_t value)
					{
						destination[0] = static_cast<uint8_t>(value & 0xFFu);
						destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
					}

					static void WriteUint32(uint8_t* const destination, const uint32_t value)
					{
						destination[0] = static_cast<uint8_t>(value & 0xFFu);
						destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
						destination[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
						destination[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
					}

					static uint16_t ReadUint16(const uint8_t* const source)
					{
						return static_cast<uint16_t>(source[0])
							| static_cast<uint16_t>(static_cast<uint16_t>(source[1]) << 8);
					}

					static uint32_t ReadUint32(const uint8_t* const source)
					{
						return static_cast<uint32_t>(source[0])
							| (static_cast<uint32_t>(source[1]) << 8)
							| (static_cast<uint32_t>(source[2]) << 16)
							| (static_cast<uint32_t>(source[3]) << 24);
					}
				};
			}
		}
	}
}

#endif