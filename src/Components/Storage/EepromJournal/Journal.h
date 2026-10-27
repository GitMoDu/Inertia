#ifndef _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_h

#include <Fletcher32.h>
#include <stdint.h>
#include <string.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace EepromJournal
			{
				enum class LoadStatusEnum : uint8_t
				{
					Loaded,
					NoValidSnapshot,
					EnumCount
				};

				/// <summary>Stores the latest valid snapshot across rotating byte-addressable storage slots.</summary>
				/// <typeparam name="Entry">Trivially copyable snapshot type stored in each slot.</typeparam>
				/// <typeparam name="ByteStore">Policy providing CapacityBytes, ReadByte(address), WriteByte(address, value), and IsWriteAvailable().</typeparam>
				/// <typeparam name="BaseAddress">First byte address reserved for the journal.</typeparam>
				/// <typeparam name="SlotCount">Number of slots reserved for rotating snapshots.</typeparam>
				/// <typeparam name="Key">Checksum seed used to distinguish this journal's records.</typeparam>
				/// <remarks>Each slot stores a sequence, entry, Fletcher32 checksum, and commit marker. Save invalidates
				/// the target slot first and writes the commit marker last. Load scans the slots and returns the newest
				/// valid sequence.</remarks>
				template<typename Entry, typename ByteStore, uint32_t BaseAddress, uint16_t SlotCount,
					uint32_t Key = sizeof(Entry)>
				class Journal
				{
				private:
					static constexpr uint32_t SequenceSize = sizeof(uint32_t);
					static constexpr uint32_t ChecksumSize = sizeof(uint32_t);
					static constexpr uint32_t CommitSize = sizeof(uint8_t);
					static constexpr uint32_t SlotSize = SequenceSize + sizeof(Entry) + ChecksumSize + CommitSize;
					static constexpr uint8_t CommitMarker = 0xA5;
					static constexpr uint8_t BitsPerByte = 8;

					static_assert(__is_trivially_copyable(Entry), "Journal entries must be trivially copyable.");
					static_assert(SlotCount >= 2, "Journal requires at least two slots.");
					static_assert(SlotSize <= UINT16_MAX, "Journal slot size must fit the address calculations.");
					static_assert(BaseAddress + SlotSize * SlotCount <= ByteStore::CapacityBytes,
						"Journal exceeds the byte store capacity.");

					static uint32_t Address(const uint16_t slot, const uint16_t offset = 0)
					{
						return BaseAddress + uint32_t(slot) * SlotSize + offset;
					}

					static uint32_t ReadSequence(const uint16_t slot)
					{
						uint32_t sequence = 0;
						for (uint8_t index = 0; index < SequenceSize; ++index)
						{
							sequence |= uint32_t(ByteStore::ReadByte(Address(slot, index))) << (index * BitsPerByte);
						}
						return sequence;
					}

					static uint32_t ComputeChecksum(const uint32_t sequence, const Entry& entry)
					{
						Fletcher32 checksum;
						checksum.begin(static_cast<uint16_t>(Key), static_cast<uint16_t>(Key >> 16));
						checksum.add(static_cast<uint16_t>(sequence));
						checksum.add(static_cast<uint16_t>(sequence >> 16));
						const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&entry);
						for (size_t index = 0; index < sizeof(Entry); index += sizeof(uint16_t))
						{
							uint16_t word = bytes[index];
							if (index + 1 < sizeof(Entry))
							{
								word |= static_cast<uint16_t>(bytes[index + 1]) << BitsPerByte;
							}
							checksum.add(word);
						}
						return checksum.getFletcher();
					}

					static uint32_t ReadChecksum(const uint16_t slot)
					{
						const uint16_t offset = static_cast<uint16_t>(SequenceSize + sizeof(Entry));
						uint32_t checksum = 0;
						for (uint8_t index = 0; index < ChecksumSize; ++index)
						{
							checksum |= uint32_t(ByteStore::ReadByte(Address(slot, offset + index)))
								<< (index * BitsPerByte);
						}
						return checksum;
					}

					static bool IsNewer(const uint32_t candidate, const uint32_t current)
					{
						return candidate != current && uint32_t(candidate - current) < 0x80000000UL;
					}

					static bool ReadSlot(const uint16_t slot, Entry& entry, uint32_t& sequence)
					{
						if (ByteStore::ReadByte(Address(slot, SlotSize - 1)) != CommitMarker) { return false; }
						sequence = ReadSequence(slot);
						for (size_t index = 0; index < sizeof(Entry); ++index)
						{
							reinterpret_cast<uint8_t*>(&entry)[index]
								= ByteStore::ReadByte(Address(slot, SequenceSize + index));
						}
						return ReadChecksum(slot) == ComputeChecksum(sequence, entry);
					}

					bool FindLatest(uint16_t& slot, Entry& entry, uint32_t& sequence) const
					{
						bool found = false;
						Entry candidate{};
						uint32_t candidateSequence = 0;
						for (uint16_t index = 0; index < SlotCount; ++index)
						{
							if (ReadSlot(index, candidate, candidateSequence)
								&& (!found || IsNewer(candidateSequence, sequence)))
							{
								found = true;
								slot = index;
								sequence = candidateSequence;
								entry = candidate;
							}
						}
						return found;
					}

				public:
					using EntryType = Entry;
					using LoadStatus = LoadStatusEnum;

					struct LoadResult
					{
						LoadStatus Status;
						Entry Value{};
						uint32_t Sequence = 0;

						bool HasSnapshot() const { return Status == LoadStatus::Loaded; }
					};

					static constexpr uint32_t Base = BaseAddress;
					static constexpr uint16_t Capacity = SlotCount;
					static constexpr uint16_t EntrySize = sizeof(Entry);
					static constexpr uint16_t SlotSizeBytes = SlotSize;
					static constexpr uint32_t UsedSize = SlotSize * SlotCount;

					LoadResult Load() const
					{
						LoadResult result{};
						uint16_t slot = 0;
						if (FindLatest(slot, result.Value, result.Sequence))
						{
							result.Status = LoadStatus::Loaded;
						}
						else
						{
							result.Status = LoadStatus::NoValidSnapshot;
						}
						return result;
					}

					bool Save(const Entry& entry)
					{
						if (!ByteStore::IsWriteAvailable()) { return false; }

						uint16_t latestSlot = 0;
						Entry latest{};
						uint32_t latestSequence = 0;
						const bool hasLatest = FindLatest(latestSlot, latest, latestSequence);
						const uint16_t targetSlot = hasLatest ? static_cast<uint16_t>((latestSlot + 1) % SlotCount) : 0;
						const uint32_t nextSequence = hasLatest ? latestSequence + 1 : 1;
						const uint16_t checksumOffset = static_cast<uint16_t>(SequenceSize + sizeof(Entry));

						if (!ByteStore::WriteByte(Address(targetSlot, SlotSize - 1), 0)) { return false; }
						for (uint8_t index = 0; index < SequenceSize; ++index)
						{
							if (!ByteStore::WriteByte(Address(targetSlot, index),
								static_cast<uint8_t>(nextSequence >> (index * BitsPerByte)))) {
								return false;
							}
						}
						const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&entry);
						for (size_t index = 0; index < sizeof(Entry); ++index)
						{
							if (!ByteStore::WriteByte(Address(targetSlot, SequenceSize + index), bytes[index])) { return false; }
						}
						const uint32_t checksum = ComputeChecksum(nextSequence, entry);
						for (uint8_t index = 0; index < ChecksumSize; ++index)
						{
							if (!ByteStore::WriteByte(Address(targetSlot, checksumOffset + index),
								static_cast<uint8_t>(checksum >> (index * BitsPerByte)))) {
								return false;
							}
						}
						if (!ByteStore::WriteByte(Address(targetSlot, SlotSize - 1), CommitMarker)) { return false; }

						Entry verification{};
						uint32_t verifiedSequence = 0;
						return ReadSlot(targetSlot, verification, verifiedSequence)
							&& verifiedSequence == nextSequence
							&& memcmp(&verification, &entry, sizeof(Entry)) == 0;
					}
				};
			}
		}
	}
}

#endif