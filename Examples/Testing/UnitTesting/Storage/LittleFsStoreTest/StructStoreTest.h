#ifndef _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_LITTLEFS_STRUCT_STORE_TEST_h
#define _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_LITTLEFS_STRUCT_STORE_TEST_h

#include <Arduino.h>
#include <LittleFS.h>
#include <InertiaModel.h>

namespace Inertia
{
	namespace Test
	{
		namespace Storage
		{
			namespace LittleFs
			{
				struct StructStoreRecord
				{
					uint32_t Sequence = 0;
					int16_t Value = 0;
					uint8_t Flags = 0;
				};

				class StructStoreTest
				{
				private:
					using Store = Inertia::Components::Storage::LittleFs::StructStore<StructStoreRecord, 3>;
					using StoreWrongVersion = Inertia::Components::Storage::LittleFs::StructStore<StructStoreRecord, 4>;
					static constexpr const char* TestPath = "/struct-store.bin";
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t DataOffset = CrcOffset + sizeof(uint16_t);
					static constexpr uint32_t FastOperationThresholdUs = 1500;
					static constexpr uint32_t WriteOperationThresholdUs = 65000;
					static constexpr uint32_t DeleteOperationThresholdUs = 60000;

				private:
					static bool PrintCheckResult(const __FlashStringHelper* testName, const bool pass)
					{
						Serial.print(testName);
						Serial.print(F(": "));
						Serial.println(pass ? F("PASSED") : F("FAILED"));
						return pass;
					}

					static bool Fail(const __FlashStringHelper* message)
					{
						Serial.print(F("    FAIL: "));
						Serial.println(message);
						return false;
					}

					static bool FailValue(const __FlashStringHelper* message, const uint32_t expected, const uint32_t actual)
					{
						Serial.print(F("    FAIL: "));
						Serial.print(message);
						Serial.print(F(" expected="));
						Serial.print(expected);
						Serial.print(F(" actual="));
						Serial.println(actual);
						return false;
					}

					static bool FailRecord(const __FlashStringHelper* message, const StructStoreRecord& expected, const StructStoreRecord& actual)
					{
						Serial.print(F("    FAIL: "));
						Serial.print(message);
						Serial.print(F(" expected={"));
						Serial.print(expected.Sequence);
						Serial.print(F(", "));
						Serial.print(expected.Value);
						Serial.print(F(", "));
						Serial.print(expected.Flags);
						Serial.print(F("} actual={"));
						Serial.print(actual.Sequence);
						Serial.print(F(", "));
						Serial.print(actual.Value);
						Serial.print(F(", "));
						Serial.print(actual.Flags);
						Serial.println(F("}"));
						return false;
					}

					static bool ExpectTrue(const bool condition, const __FlashStringHelper* message)
					{
						return condition ? true : Fail(message);
					}

					static bool ExpectEqual(const uint32_t expected, const uint32_t actual, const __FlashStringHelper* message)
					{
						return expected == actual ? true : FailValue(message, expected, actual);
					}

					static bool FailDuration(const __FlashStringHelper* message, const uint32_t thresholdUs, const uint32_t actualUs)
					{
						Serial.print(F("    FAIL: "));
						Serial.print(message);
						Serial.print(F(" thresholdUs="));
						Serial.print(thresholdUs);
						Serial.print(F(" actualUs="));
						Serial.println(actualUs);
						return false;
					}

					static void PrintDuration(const __FlashStringHelper* message, const uint32_t thresholdUs, const uint32_t actualUs)
					{
						Serial.print(F("    duration: "));
						Serial.print(message);
						Serial.print(F(" thresholdUs="));
						Serial.print(thresholdUs);
						Serial.print(F(" actualUs="));
						Serial.println(actualUs);
					}

					static bool ExpectDurationAtMost(const uint32_t actualUs, const uint32_t thresholdUs, const __FlashStringHelper* message)
					{
						PrintDuration(message, thresholdUs, actualUs);
						return actualUs <= thresholdUs ? true : FailDuration(message, thresholdUs, actualUs);
					}

					static bool ExpectRecord(const StructStoreRecord& expected, const StructStoreRecord& actual, const __FlashStringHelper* message)
					{
						return expected.Sequence == actual.Sequence
							&& expected.Value == actual.Value
							&& expected.Flags == actual.Flags
							? true
							: FailRecord(message, expected, actual);
					}

					static bool StartLittleFs()
					{
						return Inertia::Components::Storage::LittleFs::PrepareFilesystem();
					}

					static void CloseLittleFs()
					{
						Inertia::Components::Storage::LittleFs::StopFilesystem();
					}

					static bool PrepareFilesystem()
					{
						if (LittleFS.exists(TestPath) && !LittleFS.remove(TestPath))
						{
							Serial.println(F("Unable to remove previous test file."));
							return false;
						}

						return true;
					}

					static bool MutateByte(const size_t offset, const uint8_t value)
					{
						File file = LittleFS.open(TestPath, "r+");
						if (!file)
						{
							return Fail(F("Failed to open test file for mutation."));
						}

						if (!file.seek(offset))
						{
							file.close();
							return Fail(F("Failed to seek within test file."));
						}

						if (file.write(&value, 1) != 1)
						{
							file.close();
							return Fail(F("Failed to write mutated byte."));
						}

						file.flush();
						file.close();
						return true;
					}

					template<typename TOperation>
					static uint32_t MeasureOperationMicros(TOperation operation)
					{
						const uint32_t start = micros();
						operation();
						return micros() - start;
					}

					static bool TestReadMissingFileFails()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						StructStoreRecord record{};
						bool pass = true;
						bool exists = true;
						const uint32_t existsDurationUs = MeasureOperationMicros([&]() { exists = store.Exists(); });
						pass &= ExpectTrue(!exists, F("Store should not exist before writing."));
						pass &= ExpectDurationAtMost(existsDurationUs, FastOperationThresholdUs, F("Missing-file exists should complete quickly."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(record); });
						pass &= ExpectTrue(!readOk, F("Read should fail when the file is missing."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Missing-file read should complete quickly."));
						bool deleteOk = false;
						const uint32_t deleteDurationUs = MeasureOperationMicros([&]() { deleteOk = store.Delete(); });
						pass &= ExpectTrue(deleteOk, F("Delete should succeed when the file is missing."));
						pass &= ExpectDurationAtMost(deleteDurationUs, FastOperationThresholdUs, F("Missing-file delete should complete quickly."));
						return pass;
					}

					static bool TestWriteReadPersistsData()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord expected{ 42, -123, 7 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write should complete within budget."));
						bool exists = false;
						const uint32_t existsDurationUs = MeasureOperationMicros([&]() { exists = store.Exists(); });
						pass &= ExpectTrue(exists, F("Store should exist after writing."));
						pass &= ExpectDurationAtMost(existsDurationUs, FastOperationThresholdUs, F("Exists should complete quickly."));
						bool readOk = false;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(readOk, F("Read should succeed after writing."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Read should complete quickly."));
						pass &= ExpectRecord(expected, actual, F("Read should return the written record."));

						Store reopenedStore(TestPath);
						StructStoreRecord reopened{};
						const uint32_t reopenedReadDurationUs = MeasureOperationMicros([&]() { readOk = reopenedStore.Read(reopened); });
						pass &= ExpectTrue(readOk, F("Reopened store should read persisted data."));
						pass &= ExpectDurationAtMost(reopenedReadDurationUs, FastOperationThresholdUs, F("Reopened read should complete quickly."));
						pass &= ExpectRecord(expected, reopened, F("Reopened read should match persisted data."));
						return pass;
					}

					static bool TestSecondWriteReplacesData()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord first{ 1, 10, 1 };
						const StructStoreRecord second{ 2, 20, 2 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(first); });
						pass &= ExpectTrue(writeOk, F("First write should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("First write should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(second); });
						pass &= ExpectTrue(writeOk, F("Second write should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Second write should complete within budget."));
						bool readOk = false;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(readOk, F("Read should succeed after overwrite."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Overwrite read should complete quickly."));
						pass &= ExpectRecord(second, actual, F("Second write should replace the previous payload."));
						return pass;
					}

					static bool TestDeleteRemovesFile()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord expected{ 9, 90, 3 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before delete."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Pre-delete write should complete within budget."));
						bool deleteOk = false;
						uint32_t deleteDurationUs = MeasureOperationMicros([&]() { deleteOk = store.Delete(); });
						pass &= ExpectTrue(deleteOk, F("Delete should remove the backing file."));
						pass &= ExpectDurationAtMost(deleteDurationUs, DeleteOperationThresholdUs, F("Delete should complete quickly."));
						bool exists = true;
						const uint32_t existsDurationUs = MeasureOperationMicros([&]() { exists = store.Exists(); });
						pass &= ExpectTrue(!exists, F("Store should not exist after delete."));
						pass &= ExpectDurationAtMost(existsDurationUs, FastOperationThresholdUs, F("Post-delete exists should complete quickly."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail after delete."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Post-delete read should complete quickly."));
						deleteDurationUs = MeasureOperationMicros([&]() { deleteOk = store.Delete(); });
						pass &= ExpectTrue(deleteOk, F("Delete should be idempotent."));
						pass &= ExpectDurationAtMost(deleteDurationUs, FastOperationThresholdUs, F("Idempotent delete should complete quickly."));
						return pass;
					}

					static bool TestCorruptCrcFailsRead()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord expected{ 100, -200, 5 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before CRC corruption."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("CRC precondition write should complete within budget."));
						pass &= ExpectTrue(MutateByte(CrcOffset, 0x5A), F("CRC byte mutation should succeed."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when CRC is corrupted."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Corrupt CRC read should complete quickly."));
						return pass;
					}

					static bool TestVersionMismatchFailsRead()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord expected{ 200, 300, 9 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before version mismatch check."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Version mismatch precondition write should complete within budget."));

						StoreWrongVersion wrongVersionStore(TestPath);
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = wrongVersionStore.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when template version differs."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Version mismatch read should complete quickly."));
						return pass;
					}

					static bool TestCorruptDataFailsRead()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store(TestPath);
						const StructStoreRecord expected{ 555, -555, 12 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before data corruption."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Data corruption precondition write should complete within budget."));
						pass &= ExpectTrue(MutateByte(DataOffset + 1, 0x33), F("Data byte mutation should succeed."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when payload bytes are corrupted."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Corrupt payload read should complete quickly."));
						return pass;
					}

				public:
					static bool RunTests()
					{
						Serial.println(F("Starting StructStore tests..."));
						if (!StartLittleFs())
						{
							Serial.println(F("StructStore tests FAILED."));
							return false;
						}

						bool pass = true;

						pass &= PrintCheckResult(F("TestReadMissingFileFails"), TestReadMissingFileFails());
						pass &= PrintCheckResult(F("TestWriteReadPersistsData"), TestWriteReadPersistsData());
						pass &= PrintCheckResult(F("TestSecondWriteReplacesData"), TestSecondWriteReplacesData());
						pass &= PrintCheckResult(F("TestDeleteRemovesFile"), TestDeleteRemovesFile());
						pass &= PrintCheckResult(F("TestCorruptCrcFailsRead"), TestCorruptCrcFailsRead());
						pass &= PrintCheckResult(F("TestVersionMismatchFailsRead"), TestVersionMismatchFailsRead());
						pass &= PrintCheckResult(F("TestCorruptDataFailsRead"), TestCorruptDataFailsRead());

						if (LittleFS.exists(TestPath))
						{
							LittleFS.remove(TestPath);
						}

						CloseLittleFs();

						Serial.println(pass
							? F("StructStore tests PASSED.")
							: F("StructStore tests FAILED."));

						return pass;
					}
				};
			}
		}
	}
}

#endif
