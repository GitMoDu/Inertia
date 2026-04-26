#ifndef _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_LITTLEFS_CIRCULAR_FILE_STORE_TEST_h
#define _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_LITTLEFS_CIRCULAR_FILE_STORE_TEST_h

#include <InertiaModel.h>

namespace Inertia
{
	namespace Test
	{
		namespace Storage
		{
			namespace LittleFs
			{
				struct CircularFileStoreBaseStruct
				{
					uint32_t Id = 0;
					uint8_t Count = 0;
				};

				struct CircularFileStoreInnerStruct
				{
					uint64_t Sequence = 0;
					bool Flag = false;
				};

				struct CircularFileStoreRecord : CircularFileStoreBaseStruct
				{
					CircularFileStoreInnerStruct Header{};
					uint32_t Sequence = 0;
					int16_t Value = 0;
				};

				class CircularFileStoreTest
				{
				private:
                 using Store = Inertia::Components::Storage::LittleFs::CircularStore<CircularFileStoreRecord, 4, 3>;
					static constexpr const char* TestPath = "/circular-file-store.bin";
					static constexpr uint32_t FastOperationThresholdUs = 2000;
					static constexpr uint32_t StartOperationThresholdUs = 200000;
					static constexpr uint32_t WriteOperationThresholdUs = 65000;
					static constexpr uint32_t TrimOperationThresholdUs = 10000;

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

					static bool FailRecord(const __FlashStringHelper* message, const CircularFileStoreRecord& expected, const CircularFileStoreRecord& actual)
					{
						Serial.print(F("    FAIL: "));
						Serial.print(message);
						Serial.print(F(" expected={"));
						Serial.print(expected.Id);
						Serial.print(F(", "));
						Serial.print(expected.Count);
						Serial.print(F(", "));
						Serial.print(static_cast<uint32_t>(expected.Header.Sequence));
						Serial.print(F(", "));
						Serial.print(expected.Header.Flag ? F("true") : F("false"));
						Serial.print(F(", "));
						Serial.print(expected.Sequence);
						Serial.print(F(", "));
						Serial.print(expected.Value);
						Serial.print(F("} actual={"));
						Serial.print(actual.Id);
						Serial.print(F(", "));
						Serial.print(actual.Count);
						Serial.print(F(", "));
						Serial.print(static_cast<uint32_t>(actual.Header.Sequence));
						Serial.print(F(", "));
						Serial.print(actual.Header.Flag ? F("true") : F("false"));
						Serial.print(F(", "));
						Serial.print(actual.Sequence);
						Serial.print(F(", "));
						Serial.print(actual.Value);
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

					static bool ExpectRecord(const CircularFileStoreRecord& expected, const CircularFileStoreRecord& actual, const __FlashStringHelper* message)
					{
						return expected.Id == actual.Id
							&& expected.Count == actual.Count
							&& expected.Header.Sequence == actual.Header.Sequence
							&& expected.Header.Flag == actual.Header.Flag
							&& expected.Sequence == actual.Sequence
							&& expected.Value == actual.Value
							? true
							: FailRecord(message, expected, actual);
					}

					static CircularFileStoreRecord MakeRecord(const uint32_t id,
						const uint8_t count,
						const uint64_t headerSequence,
						const bool headerFlag,
						const uint32_t sequence,
						const int16_t value)
					{
						CircularFileStoreRecord record{};
						record.Id = id;
						record.Count = count;
						record.Header.Sequence = headerSequence;
						record.Header.Flag = headerFlag;
						record.Sequence = sequence;
						record.Value = value;
						return record;
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

					template<typename TOperation>
					static uint32_t MeasureOperationMicros(TOperation operation)
					{
						const uint32_t start = micros();
						operation();
						return micros() - start;
					}

					static bool ReadRecord(Store& store, const uint32_t index, CircularFileStoreRecord& record)
					{
						if (!store.Read(index, record))
						{
							Serial.print(F("    FAIL: unable to read logical index "));
							Serial.println(index);
							return false;
						}

						return true;
					}

					static bool TestStartCreatesStorage()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should create the backing file."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));
						pass &= ExpectTrue(store.IsStarted(), F("Store should be started after Start()."));
						bool exists = false;
						const uint32_t existsDurationUs = MeasureOperationMicros([&]() { exists = LittleFS.exists(TestPath); });
						pass &= ExpectTrue(exists, F("Backing file should exist after Start()."));
						pass &= ExpectDurationAtMost(existsDurationUs, FastOperationThresholdUs, F("Exists should complete quickly."));
						pass &= ExpectEqual(0, store.GetCount(), F("New store should be empty."));
						pass &= ExpectEqual(4, store.GetCapacity(), F("Capacity should match the template argument."));
						pass &= ExpectTrue(!store.IsFull(), F("New store should not report full."));

						CircularFileStoreRecord record{};
						bool readOk = true;
						const uint32_t emptyReadDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(0, record); });
						pass &= ExpectTrue(!readOk, F("Empty store reads should fail."));
						pass &= ExpectDurationAtMost(emptyReadDurationUs, FastOperationThresholdUs, F("Empty read should complete quickly."));
						store.Stop();
						return pass;
					}

					static bool TestWriteReadOrder()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should succeed."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));

						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(1, 1, 1001, true, 1, 10)); });
						pass &= ExpectTrue(writeOk, F("Write 1 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 1 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(2, 2, 1002, false, 2, 20)); });
						pass &= ExpectTrue(writeOk, F("Write 2 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 2 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(3, 3, 1003, true, 3, 30)); });
						pass &= ExpectTrue(writeOk, F("Write 3 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 3 should complete within budget."));
						pass &= ExpectEqual(3, store.GetCount(), F("Count should reflect written records."));

						CircularFileStoreRecord record{};
						bool readOk = false;
						uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 0, record); });
						pass &= ExpectTrue(readOk, F("Logical index 0 read should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Logical index 0 read should complete quickly."));
						pass &= ExpectRecord(MakeRecord(1, 1, 1001, true, 1, 10), record, F("Logical index 0 should be oldest."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 1, record); });
						pass &= ExpectTrue(readOk, F("Logical index 1 read should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Logical index 1 read should complete quickly."));
						pass &= ExpectRecord(MakeRecord(2, 2, 1002, false, 2, 20), record, F("Logical index 1 should be second."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 2, record); });
						pass &= ExpectTrue(readOk, F("Logical index 2 read should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Logical index 2 read should complete quickly."));
						pass &= ExpectRecord(MakeRecord(3, 3, 1003, true, 3, 30), record, F("Logical index 2 should be newest."));
						store.Stop();
						return pass;
					}

					static bool TestWrapAroundKeepsNewestRecords()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should succeed."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));

						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(1, 1, 2001, true, 1, 10)); });
						pass &= ExpectTrue(writeOk, F("Write 1 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 1 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(2, 2, 2002, false, 2, 20)); });
						pass &= ExpectTrue(writeOk, F("Write 2 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 2 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(3, 3, 2003, true, 3, 33)); });
						pass &= ExpectTrue(writeOk, F("Write 3 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 3 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(4, 4, 2004, false, 4, 44)); });
						pass &= ExpectTrue(writeOk, F("Write 4 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 4 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(5, 5, 2005, true, 5, 55)); });
						pass &= ExpectTrue(writeOk, F("Write 5 should wrap the buffer."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 5 should complete within budget."));
						pass &= ExpectEqual(4, store.GetCount(), F("Wrapped store should remain at capacity."));
						pass &= ExpectTrue(store.IsFull(), F("Store should report full after wrap-around."));

						CircularFileStoreRecord record{};
						bool readOk = false;
						uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 0, record); });
						pass &= ExpectTrue(readOk, F("Wrapped read 0 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Wrapped read 0 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(2, 2, 2002, false, 2, 20), record, F("Oldest visible record should advance after wrap."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 1, record); });
						pass &= ExpectTrue(readOk, F("Wrapped read 1 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Wrapped read 1 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(3, 3, 2003, true, 3, 33), record, F("Second record should match wrapped order."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 2, record); });
						pass &= ExpectTrue(readOk, F("Wrapped read 2 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Wrapped read 2 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(4, 4, 2004, false, 4, 44), record, F("Third record should match wrapped order."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(store, 3, record); });
						pass &= ExpectTrue(readOk, F("Wrapped read 3 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Wrapped read 3 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(5, 5, 2005, true, 5, 55), record, F("Newest record should be retained."));
						store.Stop();
						return pass;
					}

					static bool TestTrimFrontPersistsNewHead()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should succeed."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));

						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(10, 10, 3010, true, 10, 60)); });
						pass &= ExpectTrue(writeOk, F("Write 10 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 10 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(20, 20, 3020, false, 20, 70)); });
						pass &= ExpectTrue(writeOk, F("Write 20 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 20 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(30, 30, 3030, true, 30, 80)); });
						pass &= ExpectTrue(writeOk, F("Write 30 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 30 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(40, 40, 3040, false, 40, 90)); });
						pass &= ExpectTrue(writeOk, F("Write 40 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 40 should complete within budget."));

						bool trimOk = false;
						const uint32_t trimDurationUs = MeasureOperationMicros([&]() { trimOk = store.TrimFront(2); });
						pass &= ExpectTrue(trimOk, F("TrimFront should remove the two oldest records."));
						pass &= ExpectDurationAtMost(trimDurationUs, TrimOperationThresholdUs, F("TrimFront should complete within budget."));
						pass &= ExpectEqual(2, store.GetCount(), F("Count should shrink after TrimFront."));
						store.Stop();

						Store restartedStore;
						const uint32_t restartDurationUs = MeasureOperationMicros([&]() { startOk = restartedStore.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Restart should succeed after TrimFront."));
						pass &= ExpectDurationAtMost(restartDurationUs, StartOperationThresholdUs, F("Restart should complete quickly after TrimFront."));
						CircularFileStoreRecord record{};
						bool readOk = false;
						uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(restartedStore, 0, record); });
						pass &= ExpectTrue(readOk, F("Trimmed read 0 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Trimmed read 0 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(30, 30, 3030, true, 30, 80), record, F("Trimmed store should restart at the new head."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(restartedStore, 1, record); });
						pass &= ExpectTrue(readOk, F("Trimmed read 1 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Trimmed read 1 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(40, 40, 3040, false, 40, 90), record, F("Trimmed store should preserve remaining records."));
						const uint32_t oversizedTrimDurationUs = MeasureOperationMicros([&]() { trimOk = restartedStore.TrimFront(100); });
						pass &= ExpectTrue(trimOk, F("Oversized trim should empty the store."));
						pass &= ExpectDurationAtMost(oversizedTrimDurationUs, TrimOperationThresholdUs, F("Oversized trim should complete within budget."));
						pass &= ExpectEqual(0, restartedStore.GetCount(), F("Oversized trim should leave zero records."));
						restartedStore.Stop();
						return pass;
					}

					static bool TestRestartReloadsPersistedData()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should succeed."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));

						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(11, 11, 4011, true, 11, 110)); });
						pass &= ExpectTrue(writeOk, F("Write 11 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 11 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(22, 22, 4022, false, 22, 22)); });
						pass &= ExpectTrue(writeOk, F("Write 22 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 22 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(33, 33, 4033, true, 33, 33)); });
						pass &= ExpectTrue(writeOk, F("Write 33 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 33 should complete within budget."));
						store.Stop();

						Store restartedStore;
						const uint32_t restartDurationUs = MeasureOperationMicros([&]() { startOk = restartedStore.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Restart should succeed."));
						pass &= ExpectDurationAtMost(restartDurationUs, StartOperationThresholdUs, F("Restart should complete quickly."));
						pass &= ExpectEqual(3, restartedStore.GetCount(), F("Restart should restore count."));

						CircularFileStoreRecord record{};
						bool readOk = false;
						uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(restartedStore, 0, record); });
						pass &= ExpectTrue(readOk, F("Restart read 0 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Restart read 0 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(11, 11, 4011, true, 11, 110), record, F("Restart should restore oldest record."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(restartedStore, 1, record); });
						pass &= ExpectTrue(readOk, F("Restart read 1 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Restart read 1 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(22, 22, 4022, false, 22, 22), record, F("Restart should restore second record."));
						readDurationUs = MeasureOperationMicros([&]() { readOk = ReadRecord(restartedStore, 2, record); });
						pass &= ExpectTrue(readOk, F("Restart read 2 should succeed."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Restart read 2 should complete quickly."));
						pass &= ExpectRecord(MakeRecord(33, 33, 4033, true, 33, 33), record, F("Restart should restore newest record."));
						restartedStore.Stop();
						return pass;
					}

					static bool TestCorruptHeaderTriggersReset()
					{
						if (!PrepareFilesystem()) { return false; }

						Store store;
						bool pass = true;
						bool startOk = false;
						const uint32_t startDurationUs = MeasureOperationMicros([&]() { startOk = store.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Start should succeed."));
						pass &= ExpectDurationAtMost(startDurationUs, StartOperationThresholdUs, F("Start should complete quickly."));

						bool writeOk = false;
						uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(7, 7, 5007, true, 7, 70)); });
						pass &= ExpectTrue(writeOk, F("Write 7 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 7 should complete within budget."));
						writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(MakeRecord(8, 8, 5008, false, 8, 80)); });
						pass &= ExpectTrue(writeOk, F("Write 8 should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write 8 should complete within budget."));
						store.Stop();

						File file = LittleFS.open(TestPath, "r+");
						if (!file)
						{
							return Fail(F("Failed to open test file for corruption."));
						}

						if (!file.seek(2))
						{
							file.close();
							return Fail(F("Failed to seek to header CRC."));
						}

						const uint8_t corruptValue = 0xA5;
						if (file.write(&corruptValue, 1) != 1)
						{
							file.close();
							return Fail(F("Failed to corrupt the persisted header."));
						}

						file.flush();
						file.close();

						Store restartedStore;
						const uint32_t restartDurationUs = MeasureOperationMicros([&]() { startOk = restartedStore.Start(TestPath); });
						pass &= ExpectTrue(startOk, F("Restart should recover from a corrupt header."));
						pass &= ExpectDurationAtMost(restartDurationUs, StartOperationThresholdUs, F("Restart should complete quickly after corruption."));
						pass &= ExpectEqual(0, restartedStore.GetCount(), F("Corrupt header should reset the store to empty."));
						restartedStore.Stop();
						return pass;
					}

				public:
					static bool RunTests()
					{
						Serial.println(F("Starting CircularFileStore tests..."));
						if (!StartLittleFs())
						{
							Serial.println(F("CircularFileStore tests FAILED."));
							return false;
						}

						bool pass = true;

						pass &= PrintCheckResult(F("TestStartCreatesStorage"), TestStartCreatesStorage());
						pass &= PrintCheckResult(F("TestWriteReadOrder"), TestWriteReadOrder());
						pass &= PrintCheckResult(F("TestWrapAroundKeepsNewestRecords"), TestWrapAroundKeepsNewestRecords());
						pass &= PrintCheckResult(F("TestTrimFrontPersistsNewHead"), TestTrimFrontPersistsNewHead());
						pass &= PrintCheckResult(F("TestRestartReloadsPersistedData"), TestRestartReloadsPersistedData());
						pass &= PrintCheckResult(F("TestCorruptHeaderTriggersReset"), TestCorruptHeaderTriggersReset());

						if (LittleFS.exists(TestPath))
						{
							LittleFS.remove(TestPath);
						}

						CloseLittleFs();

						Serial.println(pass
							? F("CircularFileStore tests PASSED.")
							: F("CircularFileStore tests FAILED."));

						return pass;
					}
				};
			}
		}
	}
}

#endif
