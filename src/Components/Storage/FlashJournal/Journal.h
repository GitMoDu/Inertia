#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_h

#include <type_traits>

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				namespace Detail
				{
					/// <summary>
					/// On-flash record:
					/// +----------------+-----------------+----------------+----------------+
					/// | sequence (32b) | Entry (padded) | Fletcher-16    | commit marker  |
					/// +----------------+-----------------+----------------+----------------+
					/// The checksum covers the key, sequence, and unpadded Entry bytes. The commit marker is programmed last;
					/// recovery accepts only committed records with a valid checksum.
					///
					/// Page rotation:
					/// +---------------------------+--------------------------+
					/// | page containing latest   | circular successor       |
					/// +---------------------------+--------------------------+
					/// Append while the active page has room. When full, keep it intact, erase the successor, then append there.
					/// </summary>
					template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t PageCount, uint16_t PayloadSize>
					class ByteFlashJournal
					{
					public:
						using size_type = uint16_t;
						using SaveStepResult = ::Inertia::Components::Storage::FlashJournal::SaveStepResultEnum;
						using SaveOperation = ::Inertia::Components::Storage::FlashJournal::SaveOperation;

					private:
						static constexpr uint32_t FlashWordSizeBytes = sizeof(uint16_t);
						static constexpr uint32_t SequenceSizeBytes = sizeof(uint32_t);
						static constexpr uint32_t ChecksumSizeBytes = sizeof(uint16_t);
						static constexpr uint32_t CommitMarkerSizeBytes = sizeof(uint16_t);
						static constexpr uint32_t SequenceOffset = 0;
						static constexpr uint32_t PayloadOffset = SequenceOffset + SequenceSizeBytes;
						static constexpr uint32_t PaddedPayloadSize =
							((uint32_t(PayloadSize) + FlashWordSizeBytes - 1) / FlashWordSizeBytes) * FlashWordSizeBytes;
						static constexpr uint32_t ChecksumOffset = PayloadOffset + PaddedPayloadSize;
						static constexpr uint32_t CommitMarkerOffset = ChecksumOffset + ChecksumSizeBytes;
						static constexpr uint32_t RecordSize = uint32_t(CommitMarkerOffset + CommitMarkerSizeBytes);
						static constexpr uint32_t SlotsPerPage = Flash::PageSizeBytes / RecordSize;
						static constexpr uint32_t JournalSize = uint32_t(PageCount) * Flash::PageSizeBytes;

						struct RecordInfo
						{
							uint32_t Sequence;
							uint16_t PageIndex;
							uint32_t SlotIndex;
						};

						static_assert(PageCount >= 2, "The journal requires at least two flash pages.");
						static_assert(PayloadSize > 0, "The journal payload cannot be empty.");
						static_assert(SlotsPerPage > 0, "The payload and journal metadata must fit in one flash page.");
						static_assert(PaddedPayloadSize <= 0xFFFF, "The padded payload must fit the journal size type.");
						static_assert(TargetFlashSizeBytes >= JournalSize, "The target flash size is smaller than the journal region.");
						static_assert(TargetFlashSizeBytes% Flash::PageSizeBytes == 0, "The target flash size must be page aligned.");

					public:

						ByteFlashJournal(const uint8_t* key, const size_type keySize)
							: Key(key), KeySize(keySize)
						{}

						static uint32_t GetJournalStartAddress()
						{
							return Flash::GetFlashBaseAddress() + TargetFlashSizeBytes - JournalSize;
						}

						static uint32_t GetJournalEndAddress()
						{
							return GetJournalStartAddress() + JournalSize;
						}

						static uint32_t GetRecordSize()
						{
							return RecordSize;
						}

						static uint32_t GetSlotsPerPage()
						{
							return SlotsPerPage;
						}

						static constexpr uint16_t GetPageCount()
						{
							return PageCount;
						}

						static constexpr size_type GetPayloadSize()
						{
							return PayloadSize;
						}

						static constexpr size_type GetPaddedPayloadSize()
						{
							return PaddedPayloadSize;
						}

						bool Load(uint8_t* payload, uint32_t& sequence) const
						{
							if (!HasKey() || !Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
								return false;

							RecordInfo latest{};
							if (!FindLatest(latest))
							{
								return false;
							}

							ReadPayload(GetRecordAddress(latest.PageIndex, latest.SlotIndex), payload);
							sequence = latest.Sequence;
							return true;
						}

						bool Save(const uint8_t* payload)
						{
							SaveOperation operation{};
							if (!BeginSave(payload, operation))
								return false;
							while (SaveStep(payload, operation) == SaveStepResult::InProgress)
								;
							return operation.CurrentPhase == SavePhaseEnum::Completed;
						}

						bool BeginSave(const uint8_t* payload, SaveOperation& operation) const
						{
							operation = SaveOperation{};
							if (!HasKey() || !Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
							{
								operation.CurrentPhase = SavePhaseEnum::Failed;
								return false;
							}

							RecordInfo latest{};
							const bool hasLatest = FindLatest(latest);
							uint16_t pageIndex = 0;
							uint32_t slotIndex = 0;
							operation.Sequence = 1;
							bool needsErase = false;
							if (hasLatest)
							{
								operation.Sequence = latest.Sequence + 1;
								pageIndex = latest.PageIndex;
								if (!FindAppendSlot(pageIndex, slotIndex))
								{
									pageIndex = uint16_t((pageIndex + 1) % PageCount);
									slotIndex = 0;
									needsErase = true;
								}
							}
							else
							{
								needsErase = true;
							}

							operation.Address = GetRecordAddress(pageIndex, slotIndex);
							operation.PageAddress = GetPageAddress(pageIndex);
							operation.CurrentPhase = needsErase
								? SavePhaseEnum::Erase
								: SavePhaseEnum::ProgramSequence;
							return true;
						}

						SaveStepResult SaveStep(const uint8_t* payload, SaveOperation& operation)
						{
							switch (operation.CurrentPhase)
							{
							case SavePhaseEnum::Erase:
								if (!Flash::ErasePage(operation.PageAddress))
									operation.CurrentPhase = SavePhaseEnum::Failed;
								else
									operation.CurrentPhase = SavePhaseEnum::ProgramSequence;
								break;
							case SavePhaseEnum::ProgramSequence:
							{
								const uint32_t address = operation.Address + SequenceOffset + uint32_t(operation.Offset);
								const size_type shift = operation.Offset * BitsPerByte;
								if (!ProgramBytePair(address, uint8_t(operation.Sequence >> shift),
									uint8_t(operation.Sequence >> (shift + BitsPerByte))))
									operation.CurrentPhase = SavePhaseEnum::Failed;
								else if ((operation.Offset += FlashWordSizeBytes) >= SequenceSizeBytes)
								{
									operation.Offset = 0;
									operation.CurrentPhase = SavePhaseEnum::ProgramPayload;
								}
								break;
							}
							case SavePhaseEnum::ProgramPayload:
							{
								const uint8_t low = operation.Offset < PayloadSize ? payload[operation.Offset] : 0xFF;
								const uint8_t high = operation.Offset + 1 < PayloadSize ? payload[operation.Offset + 1] : 0xFF;
								if (!ProgramBytePair(operation.Address + PayloadOffset + uint32_t(operation.Offset), low, high))
									operation.CurrentPhase = SavePhaseEnum::Failed;
								else if ((operation.Offset += FlashWordSizeBytes) >= PaddedPayloadSize)
								{
									operation.Offset = 0;
									operation.CurrentPhase = SavePhaseEnum::ProgramChecksum;
								}
								break;
							}
							case SavePhaseEnum::ProgramChecksum:
							{
								const uint16_t checksum = ComputePayloadChecksum(operation.Sequence, payload);
								if (!ProgramBytePair(operation.Address + ChecksumOffset, uint8_t(checksum), uint8_t(checksum >> BitsPerByte)))
									operation.CurrentPhase = SavePhaseEnum::Failed;
								else
									operation.CurrentPhase = SavePhaseEnum::ProgramCommit;
								break;
							}
							case SavePhaseEnum::ProgramCommit:
								if (!Flash::ProgramHalfWord(operation.Address + CommitMarkerOffset, CommitMarker))
									operation.CurrentPhase = SavePhaseEnum::Failed;
								else
									operation.CurrentPhase = SavePhaseEnum::Verify;
								break;
							case SavePhaseEnum::Verify:
							{
								RecordInfo verified{};
								operation.CurrentPhase = ReadRecord(operation.Address, verified)
									? SavePhaseEnum::Completed
									: SavePhaseEnum::Failed;
								break;
							}
							case SavePhaseEnum::Completed:
								return SaveStepResult::Completed;
							case SavePhaseEnum::Failed:
							case SavePhaseEnum::NotStarted:
							default:
								operation.CurrentPhase = SavePhaseEnum::Failed;
								return SaveStepResult::Failed;
							}

							if (operation.CurrentPhase == SavePhaseEnum::Failed)
								return SaveStepResult::Failed;
							if (operation.CurrentPhase == SavePhaseEnum::Completed)
								return SaveStepResult::Completed;
							return SaveStepResult::InProgress;
						}

						bool EraseJournalPage(const uint16_t pageIndex)
						{
							if (pageIndex >= PageCount
								|| !Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
								return false;
							return Flash::ErasePage(GetPageAddress(pageIndex));
						}

					private:
						static uint32_t GetPageAddress(const uint16_t pageIndex)
						{
							return GetJournalStartAddress() + uint32_t(pageIndex) * Flash::PageSizeBytes;
						}

						static uint32_t GetRecordAddress(const uint16_t pageIndex, const uint32_t slotIndex)
						{
							return GetPageAddress(pageIndex) + slotIndex * RecordSize;
						}

						static uint16_t ReadHalfWord(const uint32_t address)
						{
							return Flash::ReadHalfWord(address);
						}

						static uint32_t Read32(const uint32_t address)
						{
							return uint32_t(ReadHalfWord(address)) | (uint32_t(ReadHalfWord(address + sizeof(uint16_t))) << 16);
						}

						static void ReadPayload(const uint32_t address, uint8_t* payload)
						{
							const uint32_t payloadAddress = address + PayloadOffset;
							for (size_type index = 0; index < PayloadSize; index += FlashWordSizeBytes)
							{
								const uint16_t value = ReadHalfWord(payloadAddress + uint32_t(index));
								payload[index] = uint8_t(value);
								if (index + 1 < PayloadSize)
									payload[index + 1] = uint8_t(value >> BitsPerByte);
							}
						}

						static bool IsErasedSlot(const uint32_t address)
						{
							for (uint32_t offset = 0; offset < RecordSize; offset += FlashWordSizeBytes)
							{
								if (ReadHalfWord(address + offset) != 0xFFFF)
									return false;
							}
							return true;
						}

						static bool FindAppendSlot(const uint16_t pageIndex, uint32_t& slotIndex)
						{
							uint32_t lastOccupiedSlot = 0;
							bool foundOccupiedSlot = false;

							for (uint32_t index = 0; index < SlotsPerPage; ++index)
							{
								if (!IsErasedSlot(GetRecordAddress(pageIndex, index)))
								{
									lastOccupiedSlot = index;
									foundOccupiedSlot = true;
								}
							}

							slotIndex = foundOccupiedSlot ? lastOccupiedSlot + 1 : 0;
							return slotIndex < SlotsPerPage;
						}

						bool FindLatest(RecordInfo& latest) const
						{
							bool found = false;
							for (uint16_t pageIndex = 0; pageIndex < PageCount; ++pageIndex)
							{
								for (uint32_t slotIndex = 0; slotIndex < SlotsPerPage; ++slotIndex)
								{
									RecordInfo candidate{};
									if (ReadRecord(GetRecordAddress(pageIndex, slotIndex), candidate)
										&& (!found || IsNewer(candidate.Sequence, latest.Sequence)))
									{
										latest = candidate;
										latest.PageIndex = pageIndex;
										latest.SlotIndex = slotIndex;
										found = true;
									}
								}
							}
							return found;
						}

						static bool IsNewer(const uint32_t candidate, const uint32_t current)
						{
							return candidate != current && uint32_t(candidate - current) < 0x80000000UL;
						}

						bool ReadRecord(const uint32_t address, RecordInfo& record) const
						{
							const uint32_t sequence = Read32(address + SequenceOffset);
							if (ReadHalfWord(address + CommitMarkerOffset) != CommitMarker)
								return false;
							if (ComputeRecordChecksum(address, sequence) != ReadHalfWord(address + ChecksumOffset))
								return false;

							record.Sequence = sequence;
							return true;
						}

						static constexpr uint16_t FletcherModulus = 255;
						static constexpr uint8_t BitsPerByte = 8;

						static void UpdateFletcher16(uint16_t& sum1, uint16_t& sum2, const uint8_t value)
						{
							sum1 = uint16_t((sum1 + value) % FletcherModulus);
							sum2 = uint16_t((sum2 + sum1) % FletcherModulus);
						}

						void InitializeFletcher16(uint16_t& sum1, uint16_t& sum2) const
						{
							sum1 = 0;
							sum2 = 0;
							for (size_type index = 0; index < KeySize; ++index)
								UpdateFletcher16(sum1, sum2, Key[index]);
						}

						static void UpdateSequence(uint16_t& sum1, uint16_t& sum2, const uint32_t sequence)
						{
							for (size_type index = 0; index < SequenceSizeBytes; ++index)
								UpdateFletcher16(sum1, sum2, uint8_t(sequence >> (index * BitsPerByte)));
						}

						static uint16_t FinishFletcher16(const uint16_t sum1, const uint16_t sum2)
						{
							return uint16_t((sum2 << BitsPerByte) | sum1);
						}

						uint16_t ComputeRecordChecksum(const uint32_t address, const uint32_t sequence) const
						{
							uint16_t sum1 = 0;
							uint16_t sum2 = 0;
							InitializeFletcher16(sum1, sum2);
							UpdateSequence(sum1, sum2, sequence);

							const uint32_t payloadAddress = address + PayloadOffset;
							for (size_type index = 0; index < PayloadSize; ++index)
							{
								const size_type wordIndex = (index / FlashWordSizeBytes) * FlashWordSizeBytes;
								const uint16_t value = ReadHalfWord(payloadAddress + uint32_t(wordIndex));
								const uint8_t shift = uint8_t((index % FlashWordSizeBytes) * BitsPerByte);
								UpdateFletcher16(sum1, sum2, uint8_t(value >> shift));
							}
							return FinishFletcher16(sum1, sum2);
						}

						uint16_t ComputePayloadChecksum(const uint32_t sequence, const uint8_t* payload) const
						{
							uint16_t sum1 = 0;
							uint16_t sum2 = 0;
							InitializeFletcher16(sum1, sum2);
							UpdateSequence(sum1, sum2, sequence);
							for (size_type index = 0; index < PayloadSize; ++index)
								UpdateFletcher16(sum1, sum2, payload[index]);
							return FinishFletcher16(sum1, sum2);
						}

						static bool ProgramBytePair(const uint32_t address, const uint8_t low, const uint8_t high)
						{
							return Flash::ProgramHalfWord(address, uint16_t(low | (uint16_t(high) << BitsPerByte)));
						}

						bool WriteRecord(const uint32_t address, const uint32_t sequence, const uint8_t* payload)
						{
							if (!IsErasedSlot(address))
								return false;

							for (size_type index = 0; index < SequenceSizeBytes; index += FlashWordSizeBytes)
							{
								const size_type shift = index * BitsPerByte;
								if (!ProgramBytePair(address + SequenceOffset + uint32_t(index),
									uint8_t(sequence >> shift), uint8_t(sequence >> (shift + BitsPerByte))))
									return false;
							}

							for (size_type index = 0; index < PaddedPayloadSize; index += FlashWordSizeBytes)
							{
								const uint8_t low = index < PayloadSize ? payload[index] : 0xFF;
								const uint8_t high = index + 1 < PayloadSize ? payload[index + 1] : 0xFF;
								if (!ProgramBytePair(address + PayloadOffset + uint32_t(index), low, high))
									return false;
							}

							const uint16_t checksum = ComputePayloadChecksum(sequence, payload);
							if (!ProgramBytePair(address + ChecksumOffset, uint8_t(checksum), uint8_t(checksum >> BitsPerByte)))
								return false;
							if (!Flash::ProgramHalfWord(address + CommitMarkerOffset, CommitMarker))
								return false;

							RecordInfo verified{};
							return ReadRecord(address, verified);
						}

						bool HasKey() const
						{
							return Key != nullptr && KeySize > 0;
						}

						const uint8_t* Key;
						size_type KeySize;

					};
				}

				/// <summary>
				/// Stores trivially copyable entries as snapshots in a circular flash journal. Entry's native object
				/// representation is persisted, so keep its layout stable across firmware versions.
				///
				/// FlashJournal Entry -> Detail ByteFlashJournal -> flash backend
				/// </summary>
				template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t PageCount, typename Entry>
				class FlashJournal
				{
				private:
					using ByteJournal = Detail::ByteFlashJournal<Flash, TargetFlashSizeBytes, PageCount, sizeof(Entry)>;
					using size_type = typename ByteJournal::size_type;
					ByteJournal Storage;

				public:
					using entry_type = Entry;
					using SaveOperation = typename ByteJournal::SaveOperation;
					using SaveStepResult = typename ByteJournal::SaveStepResult;

					static_assert(std::is_trivially_copyable<Entry>::value,
						"FlashJournal entries must be trivially copyable.");
					static_assert(sizeof(Entry) <= 0xFFFF,
						"FlashJournal entries must fit the journal payload size.");

					FlashJournal(const uint8_t* key, const size_type keySize)
						: Storage(key, keySize)
					{}

					static constexpr size_type GetPayloadSize()
					{
						return sizeof(Entry);
					}

					static constexpr size_type GetPaddedPayloadSize()
					{
						return ByteJournal::GetPaddedPayloadSize();
					}

					static uint32_t GetJournalStartAddress()
					{
						return ByteJournal::GetJournalStartAddress();
					}

					static uint32_t GetJournalEndAddress()
					{
						return ByteJournal::GetJournalEndAddress();
					}

					static uint32_t GetRecordSize()
					{
						return ByteJournal::GetRecordSize();
					}

					static uint32_t GetSlotsPerPage()
					{
						return ByteJournal::GetSlotsPerPage();
					}

					static constexpr uint16_t GetPageCount()
					{
						return ByteJournal::GetPageCount();
					}

					bool Load(Entry& entry, uint32_t& sequence) const
					{
						return Storage.Load(reinterpret_cast<uint8_t*>(&entry), sequence);
					}

					bool Save(const Entry& entry)
					{
						return Storage.Save(reinterpret_cast<const uint8_t*>(&entry));
					}

					bool BeginSave(const Entry& entry, SaveOperation& operation) const
					{
						return Storage.BeginSave(reinterpret_cast<const uint8_t*>(&entry), operation);
					}

					SaveStepResult SaveStep(const Entry& entry, SaveOperation& operation)
					{
						return Storage.SaveStep(reinterpret_cast<const uint8_t*>(&entry), operation);
					}

					bool EraseJournalPage(const uint16_t pageIndex)
					{
						return Storage.EraseJournalPage(pageIndex);
					}
				};
			}
		}
	}
}

#endif
