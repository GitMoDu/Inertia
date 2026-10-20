#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_h

#include "Model.h"
#include "ByteFlashJournal.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				/// <summary>
				/// Stores trivially copyable entries as snapshots in a circular flash journal. Trivially copyable does not
				/// imply a stable cross-firmware format: Entry layout, padding, and endianness still matter.
				///
				/// On-flash record:
				/// +----------------+------------------+---------------+----------------+
				/// | sequence (32b) | Entry (padded)   | checksum      | commit marker  |
				/// +----------------+------------------+---------------+----------------+
				/// The checksum covers the sequence and unpadded entry bytes. The commit marker is programmed last;
				/// recovery accepts only committed records with a valid checksum.
				///
				/// Append while the active page has room. When full, the journal erases the circular successor page
				/// before appending there, keeping the page with the latest snapshot intact.
				///
				/// Records can only be loaded with the seed used to write them; the seed is not authentication.
				/// A save operation must not outlive its journal. Destruction abandons in-memory progress but cannot undo
				/// flash writes; any partially programmed slot remains occupied and is skipped during recovery.
				/// Separate instances targeting the same physical region require external coordination.
				/// </summary>
				template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t JournalPageCount, typename Entry>
				class FlashJournal
				{
				private:
					using ByteJournal = Detail::ByteFlashJournal<Flash, TargetFlashSizeBytes, JournalPageCount, sizeof(Entry)>;
					using size_type = typename ByteJournal::size_type;
					ByteJournal Storage;

				public:
					using entry_type = Entry;
					using SaveOperation = typename ByteJournal::SaveOperation;
					using LoadStatus = LoadStatusEnum;
					using SaveStatus = SaveStatusEnum;
					using SaveError = SaveErrorEnum;

					struct Snapshot
					{
						Entry Value;
						uint32_t Sequence;
					};

					class LoadResult
					{
					public:
						LoadStatus Status() const
						{
							return CurrentStatus;
						}

						bool HasSnapshot() const
						{
							return CurrentStatus == LoadStatus::Loaded;
						}

						const Snapshot* GetSnapshot() const
						{
							return HasSnapshot() ? &Value : nullptr;
						}

						LoadResult(const Snapshot& value, const LoadStatus status)
							: Value(value), CurrentStatus(status)
						{}

					private:
						Snapshot Value;
						LoadStatus CurrentStatus;
					};

					static_assert(!IntegerSignal::TypeTraits::TypeProperties::is_trivially_copyable<Entry>::is_supported ||
						IntegerSignal::TypeTraits::TypeProperties::is_trivially_copyable<Entry>::value,
						"FlashJournal entries must be trivially copyable.");
					static_assert(sizeof(Entry) <= 0xFFFF,
						"FlashJournal entries must fit the journal payload size.");

					FlashJournal(const uint32_t seed)
						: Storage(seed)
					{}

					LoadResult Load() const
					{
						Snapshot snapshot{};
						uint32_t sequence = 0;
						LoadStatus status = LoadStatus::NoValidSnapshot;
						Storage.Load(reinterpret_cast<uint8_t*>(&snapshot.Value), sequence, status);
						snapshot.Sequence = sequence;
						return LoadResult(snapshot, status);
					}

					bool Save(const Entry& entry)
					{
						return Storage.Save(reinterpret_cast<const uint8_t*>(&entry));
					}

					void BeginSave(const Entry& entry, SaveOperation& operation)
					{
						Storage.BeginSave(reinterpret_cast<const uint8_t*>(&entry), operation);
					}

					/// <summary>
					/// Destructive maintenance operation. Erasing a page can destroy the latest snapshot.
					/// </summary>
					/// <param name="pageIndex">Page index to erase.</param>
					/// <returns>True if the page was successfully erased; otherwise, false.</returns>
					bool ErasePageForMaintenance(const uint16_t pageIndex)
					{
						return Storage.EraseJournalPage(pageIndex);
					}

					static constexpr size_type GetPayloadSize()
					{
						return sizeof(Entry);
					}

					static constexpr size_type GetPaddedPayloadSize()
					{
						return ByteJournal::GetPaddedPayloadSize();
					}

					static constexpr uint32_t GetJournalStartAddress()
					{
						return ByteJournal::GetJournalStartAddress();
					}

					static constexpr uint32_t GetJournalEndAddress()
					{
						return ByteJournal::GetJournalEndAddress();
					}

					static constexpr uint32_t GetRecordSize()
					{
						return ByteJournal::GetRecordSize();
					}

					static constexpr uint32_t GetSlotsPerPage()
					{
						return ByteJournal::GetSlotsPerPage();
					}

					static constexpr uint16_t GetPageCount()
					{
						return ByteJournal::GetPageCount();
					}

					static constexpr JournalGeometry GetGeometry()
					{
						return JournalGeometry{ GetJournalStartAddress(), GetJournalEndAddress(), GetRecordSize(),
							GetSlotsPerPage(), GetPageCount(), GetPayloadSize(), GetPaddedPayloadSize() };
					}
				};
			}
		}
	}
}

#endif
