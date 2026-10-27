#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_BYTE_FLASH_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_BYTE_FLASH_JOURNAL_h

#include <Fletcher32.h>

#include "Model.h"
#include "SaveAction.h"

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
					/// Implements the byte-level record storage and recovery for FlashJournal.
					/// </summary>
					template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t PageCount, uint16_t PayloadSize>
					class ByteFlashJournal : public SaveActionOwner
					{
					public:
						using size_type = uint16_t;

					private:
						static constexpr uint32_t FlashWordSizeBytes = sizeof(uint16_t);
						static constexpr uint32_t SequenceSizeBytes = sizeof(uint32_t);
						static constexpr uint32_t ChecksumSizeBytes = sizeof(uint32_t);
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
						static constexpr uint8_t BitsPerByte = 8;

						struct RecordInfo
						{
							uint32_t Sequence;
							uint16_t PageIndex;
							uint32_t SlotIndex;
						};

						using Phase = SaveActionPhaseEnum;

					public:
						using SaveOperation = SaveAction<PayloadSize>;

					private:
						static_assert(PageCount >= 2, "The journal requires at least two flash pages.");
						static_assert(PayloadSize > 0, "The journal payload cannot be empty.");
						static_assert(SlotsPerPage > 0, "The payload and journal metadata must fit in one flash page.");
						static_assert(PaddedPayloadSize <= 0xFFFF, "The padded payload must fit the journal size type.");
						static_assert(TargetFlashSizeBytes >= JournalSize, "The target flash size is smaller than the journal region.");
						static_assert(TargetFlashSizeBytes% Flash::PageSizeBytes == 0, "The target flash size must be page aligned.");

					private:
						SaveActionState* ActiveSave = nullptr;
						mutable Fletcher32 Hasher;
						uint32_t Seed;

					public:
						ByteFlashJournal(const uint32_t seed)
							: Seed(seed)
						{}

						ByteFlashJournal(const ByteFlashJournal& other)
							: Seed(other.Seed)
						{}

						ByteFlashJournal& operator=(const ByteFlashJournal& other)
						{
							if (this != &other && ActiveSave == nullptr)
							{
								Seed = other.Seed;
								Hasher.begin();
							}
							return *this;
						}

						static constexpr uint32_t GetJournalStartAddress()
						{
							return Flash::GetFlashBaseAddress() + TargetFlashSizeBytes - JournalSize;
						}

						static constexpr uint32_t GetJournalEndAddress()
						{
							return GetJournalStartAddress() + JournalSize;
						}

						static constexpr uint32_t GetRecordSize()
						{
							return RecordSize;
						}

						static constexpr uint32_t GetSlotsPerPage()
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

						bool Load(uint8_t* payload, uint32_t& sequence, LoadStatusEnum& status) const
						{
							if (!Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
							{
								status = LoadStatusEnum::StorageUnavailable;
								return false;
							}

							RecordInfo latest{};
							if (!FindLatest(latest))
							{
								status = LoadStatusEnum::NoValidSnapshot;
								return false;
							}

							ReadPayload(GetRecordAddress(latest.PageIndex, latest.SlotIndex), payload);
							sequence = latest.Sequence;
							status = LoadStatusEnum::Loaded;
							return true;
						}

						bool Save(const uint8_t* payload)
						{
							if (ActiveSave != nullptr)
								return false;
							SaveOperation operation;
							BeginSave(payload, operation);
							while (operation.Status() == SaveStatusEnum::InProgress)
								operation.Step();
							return operation.Status() == SaveStatusEnum::Completed;
						}

						void BeginSave(const uint8_t* payload, SaveOperation& operation)
						{
							operation.Abandon();
							operation.SetPayload(payload);
							auto& state = operation.GetState();
							state.CurrentStatus = SaveStatusEnum::InProgress;
							state.CurrentError = SaveErrorEnum::None;
							state.CurrentPhase = Phase::NotStarted;
							state.Address = 0;
							state.PageAddress = 0;
							state.SequenceValue = 0;
							state.Offset = 0;
							state.Active = false;
							if (!Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
							{
								Fail(state, SaveErrorEnum::StorageUnavailable);
								return;
							}
							operation.Attach(this);
						}

						SaveStatusEnum StepSave(SaveActionState& state, const uint8_t* payload) override
						{
							if (state.CurrentStatus != SaveStatusEnum::InProgress)
								return state.CurrentStatus;
							if (ActiveSave != nullptr && ActiveSave != &state)
								return SaveStatusEnum::InProgress;
							if (!state.Active)
							{
								ActiveSave = &state;
								state.Active = true;
								SelectAppendLocation(state);
							}

							switch (state.CurrentPhase)
							{
							case Phase::Erase:
								if (!Flash::ErasePage(state.PageAddress))
									Fail(state, SaveErrorEnum::FlashFailure);
								else
									state.CurrentPhase = Phase::ProgramSequence;
								break;
							case Phase::ProgramSequence:
							{
								const uint32_t address = state.Address + SequenceOffset + uint32_t(state.Offset);
								const size_type shift = state.Offset * BitsPerByte;
								if (!ProgramBytePair(address, uint8_t(state.SequenceValue >> shift),
									uint8_t(state.SequenceValue >> (shift + BitsPerByte))))
									Fail(state, SaveErrorEnum::FlashFailure);
								else if ((state.Offset += FlashWordSizeBytes) >= SequenceSizeBytes)
								{
									state.Offset = 0;
									state.CurrentPhase = Phase::ProgramPayload;
								}
								break;
							}
							case Phase::ProgramPayload:
							{
								const uint8_t low = state.Offset < PayloadSize ? payload[state.Offset] : 0xFF;
								const uint8_t high = state.Offset + 1 < PayloadSize ? payload[state.Offset + 1] : 0xFF;
								if (!ProgramBytePair(state.Address + PayloadOffset + uint32_t(state.Offset), low, high))
									Fail(state, SaveErrorEnum::FlashFailure);
								else if ((state.Offset += FlashWordSizeBytes) >= PaddedPayloadSize)
								{
									state.Offset = 0;
									state.CurrentPhase = Phase::ProgramChecksum;
								}
								break;
							}
							case Phase::ProgramChecksum:
							{
								const uint32_t checksum = ComputePayloadChecksum(state.SequenceValue, payload);
								const uint32_t checksumAddress = state.Address + ChecksumOffset + uint32_t(state.Offset);
								const uint16_t checksumWord = state.Offset == 0
									? uint16_t(checksum) : uint16_t(checksum >> (2 * BitsPerByte));
								if (!Flash::ProgramHalfWord(checksumAddress, checksumWord))
									Fail(state, SaveErrorEnum::FlashFailure);
								else if ((state.Offset += FlashWordSizeBytes) >= ChecksumSizeBytes)
								{
									state.Offset = 0;
									state.CurrentPhase = Phase::ProgramCommit;
								}
								break;
							}
							case Phase::ProgramCommit:
								if (!Flash::ProgramHalfWord(state.Address + CommitMarkerOffset, CommitMarker))
									Fail(state, SaveErrorEnum::FlashFailure);
								else
									state.CurrentPhase = Phase::Verify;
								break;
							case Phase::Verify:
							{
								RecordInfo verified{};
								if (ReadRecord(state.Address, verified))
									Complete(state);
								else
									Fail(state, SaveErrorEnum::VerificationFailure);
								break;
							}
							case Phase::NotStarted:
							default:
								Fail(state, SaveErrorEnum::FlashFailure);
								break;
							}

							return state.CurrentStatus;
						}

						bool EraseJournalPage(const uint16_t pageIndex)
						{
							if (ActiveSave != nullptr || pageIndex >= PageCount
								|| !Flash::IsStorageRegionAvailable(GetJournalStartAddress(), GetJournalEndAddress()))
								return false;
							return Flash::ErasePage(GetPageAddress(pageIndex));
						}

						void Release(SaveActionState& state) override
						{
							if (ActiveSave == &state)
								ActiveSave = nullptr;
							state.Active = false;
						}

					private:
						void SelectAppendLocation(SaveActionState& state)
						{
							RecordInfo latest{};
							uint16_t pageIndex = 0;
							uint32_t slotIndex = 0;
							bool needsErase = !FindLatest(latest);
							state.SequenceValue = needsErase ? 1 : latest.Sequence + 1;
							if (!needsErase)
							{
								pageIndex = latest.PageIndex;
								if (!FindAppendSlot(pageIndex, slotIndex))
								{
									pageIndex = uint16_t((pageIndex + 1) % PageCount);
									slotIndex = 0;
									needsErase = true;
								}
							}
							state.Address = GetRecordAddress(pageIndex, slotIndex);
							state.PageAddress = GetPageAddress(pageIndex);
							state.CurrentPhase = needsErase ? Phase::Erase : Phase::ProgramSequence;
						}

						void Fail(SaveActionState& state, const SaveErrorEnum error)
						{
							state.CurrentStatus = SaveStatusEnum::Failed;
							state.CurrentError = error;
							Release(state);
						}

						void Complete(SaveActionState& state)
						{
							state.CurrentStatus = SaveStatusEnum::Completed;
							state.CurrentError = SaveErrorEnum::None;
							Release(state);
						}

						static uint32_t GetPageAddress(const uint16_t pageIndex)
						{
							return GetJournalStartAddress() + uint32_t(pageIndex) * Flash::PageSizeBytes;
						}

						static uint32_t GetRecordAddress(const uint16_t pageIndex, const uint32_t slotIndex)
						{
							return GetPageAddress(pageIndex) + slotIndex * RecordSize;
						}

						static uint32_t Read32(const uint32_t address)
						{
							return uint32_t(Flash::ReadHalfWord(address))
								| (uint32_t(Flash::ReadHalfWord(address + sizeof(uint16_t))) << 16);
						}

						static void ReadPayload(const uint32_t address, uint8_t* payload)
						{
							const uint32_t payloadAddress = address + PayloadOffset;
							for (size_type index = 0; index < PayloadSize; index += FlashWordSizeBytes)
							{
								const uint16_t value = Flash::ReadHalfWord(payloadAddress + uint32_t(index));
								payload[index] = uint8_t(value);
								if (index + 1 < PayloadSize)
									payload[index + 1] = uint8_t(value >> BitsPerByte);
							}
						}

						static bool IsErasedSlot(const uint32_t address)
						{
							for (uint32_t offset = 0; offset < RecordSize; offset += FlashWordSizeBytes)
							{
								if (Flash::ReadHalfWord(address + offset) != 0xFFFF)
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
							if (Flash::ReadHalfWord(address + CommitMarkerOffset) != CommitMarker)
								return false;
							if (ComputeRecordChecksum(address, sequence) != Read32(address + ChecksumOffset))
								return false;

							record.Sequence = sequence;
							return true;
						}

						uint32_t ComputeRecordChecksum(const uint32_t address, const uint32_t sequence) const
						{
							Hasher.begin(uint16_t(Seed), uint16_t(Seed >> (2 * BitsPerByte)));
							Hasher.add(uint16_t(sequence));
							Hasher.add(uint16_t(sequence >> (2 * BitsPerByte)));

							const uint32_t payloadAddress = address + PayloadOffset;
							for (size_type index = 0; index < PayloadSize; index += FlashWordSizeBytes)
							{
								uint16_t value = Flash::ReadHalfWord(payloadAddress + uint32_t(index));
								if (index + 1 >= PayloadSize)
									value = uint8_t(value);
								Hasher.add(value);
							}
							return Hasher.getFletcher();
						}

						uint32_t ComputePayloadChecksum(const uint32_t sequence, const uint8_t* payload) const
						{
							Hasher.begin(uint16_t(Seed), uint16_t(Seed >> (2 * BitsPerByte)));
							Hasher.add(uint16_t(sequence));
							Hasher.add(uint16_t(sequence >> (2 * BitsPerByte)));
							for (size_type index = 0; index < PayloadSize; index += FlashWordSizeBytes)
							{
								uint16_t value = payload[index];
								if (index + 1 < PayloadSize)
									value |= uint16_t(payload[index + 1]) << BitsPerByte;
								Hasher.add(value);
							}
							return Hasher.getFletcher();
						}

						static bool ProgramBytePair(const uint32_t address, const uint8_t low, const uint8_t high)
						{
							return Flash::ProgramHalfWord(address, uint16_t(low | (uint16_t(high) << BitsPerByte)));
						}

					};
				}
			}
		}
	}
}

#endif
