#ifndef _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_FRAM_STRUCT_STORE_TEST_h
#define _INERTIA_EXAMPLES_TESTING_UNIT_TESTING_STORAGE_FRAM_STRUCT_STORE_TEST_h

#include <Arduino.h>
#include <InertiaModel.h>

namespace Inertia
{
	namespace Test
	{
		namespace Storage
		{
			namespace Fram
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
					static constexpr uint16_t TestBaseAddress = 512;
					using Store = Inertia::Components::Storage::Fram::StructStore<StructStoreRecord, 3, TestBaseAddress>;
					using StoreWrongVersion = Inertia::Components::Storage::Fram::StructStore<StructStoreRecord, 4, TestBaseAddress>;
					static constexpr size_t VersionOffset = 0;
					static constexpr size_t CrcOffset = VersionOffset + sizeof(uint16_t);
					static constexpr size_t DataOffset = CrcOffset + sizeof(uint16_t);

#if defined(ARDUINO_ARCH_RP2040)
					static constexpr uint32_t FastOperationThresholdUs = 1500;
					static constexpr uint32_t WriteOperationThresholdUs = 1500;
#else
					static constexpr uint32_t FastOperationThresholdUs = 20000;
					static constexpr uint32_t WriteOperationThresholdUs = 40000;
#endif

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

					static bool FillStorage(Inertia::Components::Storage::Fram::IFramDriver& driver, const uint8_t value)
					{
						uint8_t buffer[16]{};
						memset(buffer, value, sizeof(buffer));
						size_t remaining = Store::GetStoreSize();
						uint16_t address = TestBaseAddress;

						while (remaining > 0)
						{
							const size_t chunkLength = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
							if (!driver.Write(address, buffer, chunkLength))
							{
								return Fail(F("Failed to fill FRAM struct storage."));
							}

							address = static_cast<uint16_t>(address + chunkLength);
							remaining -= chunkLength;
						}

						return true;
					}

					static bool PrepareStorage(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						return FillStorage(driver, 0xFFu);
					}

					static bool MutateByte(Inertia::Components::Storage::Fram::IFramDriver& driver, const size_t offset, const uint8_t value)
					{
						const uint8_t buffer[1]{ value };
						return driver.Write(static_cast<uint16_t>(TestBaseAddress + offset), buffer, sizeof(buffer))
							? true
							: Fail(F("Failed to write mutated FRAM byte."));
					}

					template<typename TOperation>
					static uint32_t MeasureOperationMicros(TOperation operation)
					{
						const uint32_t start = micros();
						operation();
						return micros() - start;
					}

					static bool TestReadBlankStorageFails(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
						StructStoreRecord record{};
						bool pass = true;
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(record); });
						pass &= ExpectTrue(!readOk, F("Read should fail when FRAM store is blank."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Blank-store read should complete quickly."));
						return pass;
					}

					static bool TestWriteReadPersistsData(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
						const StructStoreRecord expected{ 42, -123, 7 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Write should complete within budget."));
						bool readOk = false;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(readOk, F("Read should succeed after writing."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Read should complete quickly."));
						pass &= ExpectRecord(expected, actual, F("Read should return the written record."));

						Store reopenedStore(driver);
						StructStoreRecord reopened{};
						const uint32_t reopenedReadDurationUs = MeasureOperationMicros([&]() { readOk = reopenedStore.Read(reopened); });
						pass &= ExpectTrue(readOk, F("Reopened store should read persisted data."));
						pass &= ExpectDurationAtMost(reopenedReadDurationUs, FastOperationThresholdUs, F("Reopened read should complete quickly."));
						pass &= ExpectRecord(expected, reopened, F("Reopened read should match persisted data."));
						return pass;
					}

					static bool TestSecondWriteReplacesData(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
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

					static bool TestCorruptCrcFailsRead(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
						const StructStoreRecord expected{ 100, -200, 5 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before CRC corruption."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("CRC precondition write should complete within budget."));
						pass &= ExpectTrue(MutateByte(driver, CrcOffset, 0x5A), F("CRC byte mutation should succeed."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when CRC is corrupted."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Corrupt CRC read should complete quickly."));
						return pass;
					}

					static bool TestVersionMismatchFailsRead(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
						const StructStoreRecord expected{ 200, 300, 9 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before version mismatch check."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Version mismatch precondition write should complete within budget."));

						StoreWrongVersion wrongVersionStore(driver);
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = wrongVersionStore.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when template version differs."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Version mismatch read should complete quickly."));
						return pass;
					}

					static bool TestCorruptDataFailsRead(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						if (!PrepareStorage(driver)) { return false; }

						Store store(driver);
						const StructStoreRecord expected{ 555, -555, 12 };
						StructStoreRecord actual{};
						bool pass = true;
						bool writeOk = false;
						const uint32_t writeDurationUs = MeasureOperationMicros([&]() { writeOk = store.Write(expected); });
						pass &= ExpectTrue(writeOk, F("Write should succeed before data corruption."));
						pass &= ExpectDurationAtMost(writeDurationUs, WriteOperationThresholdUs, F("Data corruption precondition write should complete within budget."));
						pass &= ExpectTrue(MutateByte(driver, DataOffset + 1, 0x33), F("Data byte mutation should succeed."));
						bool readOk = true;
						const uint32_t readDurationUs = MeasureOperationMicros([&]() { readOk = store.Read(actual); });
						pass &= ExpectTrue(!readOk, F("Read should fail when payload bytes are corrupted."));
						pass &= ExpectDurationAtMost(readDurationUs, FastOperationThresholdUs, F("Corrupt payload read should complete quickly."));
						return pass;
					}

				public:
					static bool RunTests(Inertia::Components::Storage::Fram::IFramDriver& driver)
					{
						Serial.println(F("Starting FRAM StructStore tests..."));
						bool pass = true;

						pass &= PrintCheckResult(F("TestReadBlankStorageFails"), TestReadBlankStorageFails(driver));
						pass &= PrintCheckResult(F("TestWriteReadPersistsData"), TestWriteReadPersistsData(driver));
						pass &= PrintCheckResult(F("TestSecondWriteReplacesData"), TestSecondWriteReplacesData(driver));
						pass &= PrintCheckResult(F("TestCorruptCrcFailsRead"), TestCorruptCrcFailsRead(driver));
						pass &= PrintCheckResult(F("TestVersionMismatchFailsRead"), TestVersionMismatchFailsRead(driver));
						pass &= PrintCheckResult(F("TestCorruptDataFailsRead"), TestCorruptDataFailsRead(driver));

						Serial.println(pass
							? F("FRAM StructStore tests PASSED.")
							: F("FRAM StructStore tests FAILED."));

						return pass;
					}
				};
			}
		}
	}
}

#endif
