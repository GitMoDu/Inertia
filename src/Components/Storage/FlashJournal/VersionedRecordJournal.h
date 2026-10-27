#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_VERSIONED_RECORD_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_VERSIONED_RECORD_JOURNAL_h

#include "Journal.h"
#include "AsyncTask.h"
#include "../VersionedRecord/Codec.h"
#include <string.h>
#include <limits.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				template<typename JournalType, typename CurrentCodec, typename Dispatcher, size_t Capacity>
				class VersionedRecordJournal
				{
			public:
					using RecordType = typename CurrentCodec::StorageType;
					using EnvelopeType = VersionedRecord::Envelope<Capacity>;
					using SaveOperation = typename JournalType::SaveOperation;
					using LoadStatus = typename JournalType::LoadStatus;
					using CodecResult = VersionedRecord::CodecResultEnum;

					struct LoadResult
					{
						CodecResult CodecStatus;
						LoadStatus JournalStatus;
						RecordType Record{};
						uint32_t Sequence = 0;

						bool HasRecord() const
						{
						return JournalStatus == LoadStatus::Loaded && CodecStatus == CodecResult::Success;
						}
					};

			private:
				JournalType& Journal;

				static_assert(Capacity >= CurrentCodec::RecordSize,
					"FlashJournal versioned-record capacity must fit the current codec record.");
				static_assert(Capacity <= UINT16_MAX,
					"FlashJournal versioned-record capacity must fit the envelope size field.");
				static_assert(sizeof(typename JournalType::entry_type) == sizeof(EnvelopeType),
					"FlashJournal entry must use the fixed-capacity versioned-record envelope.");

				bool Encode(const RecordType& record, EnvelopeType& envelope) const
				{
					envelope = {};
					envelope.Size = static_cast<uint16_t>(CurrentCodec::RecordSize);
					return CurrentCodec::Encode(record, envelope.Bytes, Capacity) == CodecResult::Success;
				}

			public:
				static constexpr size_t RecordCapacity = Capacity;

				explicit VersionedRecordJournal(JournalType& journal)
					: Journal(journal)
				{}

				LoadResult Load() const
				{
					const auto result = Journal.Load();
					LoadResult decoded{};
					decoded.JournalStatus = result.Status();
					if (!result.HasSnapshot())
					{
						decoded.CodecStatus = CodecResult::InvalidSize;
						return decoded;
					}

					const auto* snapshot = result.GetSnapshot();
					decoded.Sequence = snapshot->Sequence;
					EnvelopeType envelope{};
					memcpy(&envelope, &snapshot->Value, sizeof(envelope));
					if (envelope.Size == 0 || envelope.Size > Capacity)
					{
						decoded.CodecStatus = CodecResult::InvalidSize;
						return decoded;
					}
					decoded.CodecStatus = Dispatcher::Decode(envelope.Bytes, envelope.Size, decoded.Record);
					return decoded;
				}

				bool Save(const RecordType& record)
				{
					EnvelopeType envelope{};
					return Encode(record, envelope) && Journal.Save(envelope);
				}

				CodecResult BeginSave(const RecordType& record, SaveOperation& operation)
				{
					EnvelopeType envelope{};
					if (!Encode(record, envelope)) { return CodecResult::BufferTooSmall; }
					Journal.BeginSave(envelope, operation);
					return CodecResult::Success;
				}

				template<uint16_t QueueCapacity>
				bool QueueSave(AsyncFlashJournalTask<JournalType, QueueCapacity>& task, const RecordType& record)
				{
					EnvelopeType envelope{};
					return Encode(record, envelope) && task.QueueSave(envelope);
				}
			};
			}
		}
	}
}

#endif