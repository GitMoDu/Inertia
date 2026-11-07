#ifndef _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_VERSIONED_RECORD_JOURNAL_h
#define _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_VERSIONED_RECORD_JOURNAL_h

#include "Journal.h"
#include "../VersionedRecord/Codec.h"
#include <string.h>
#include <limits.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace EepromJournal
			{
				template<typename JournalType, typename CurrentCodec, typename Dispatcher, size_t Capacity>
				class VersionedRecordJournal
				{
				public:
					using RecordType = typename CurrentCodec::StorageType;
					using SaveOperation = typename JournalType::SaveOperation;
					using LoadStatus = typename JournalType::LoadStatus;
					using SaveStatus = typename JournalType::SaveStatus;
					using CodecResult = VersionedRecord::CodecResultEnum;

					struct LoadResult
					{
						CodecResult CodecStatus = CodecResult::InvalidSize;
						LoadStatus JournalStatus = LoadStatus::NoValidSnapshot;
						RecordType Record{};
						uint32_t Sequence = 0;

						bool HasRecord() const
						{
							return JournalStatus == LoadStatus::Loaded && CodecStatus == CodecResult::Success;
						}
					};

				private:
					using EnvelopeType = VersionedRecord::Envelope<Capacity>;

					JournalType& Journal;

					static_assert(Capacity >= CurrentCodec::RecordSize,
						"EepromJournal versioned-record capacity must fit the current codec record.");
					static_assert(Capacity <= UINT16_MAX,
						"EepromJournal versioned-record capacity must fit the envelope size field.");
					static_assert(sizeof(typename JournalType::EntryType) == sizeof(EnvelopeType),
						"EepromJournal entry must use the fixed-capacity versioned-record envelope.");

					bool Encode(const RecordType& record, EnvelopeType& envelope) const
					{
						envelope = {};
						envelope.Size = static_cast<uint16_t>(CurrentCodec::RecordSize);
						return CurrentCodec::Encode(record, envelope.Bytes, Capacity) == CodecResult::Success;
					}

				public:
					static constexpr size_t RecordCapacity = Capacity;

					/// <summary>Wraps a journal with versioned-record encoding and decoding.</summary>
					explicit VersionedRecordJournal(JournalType& journal)
						: Journal(journal)
					{}

					/// <summary>Loads the latest record and reports journal and codec status separately.</summary>
					LoadResult Load() const
					{
						const auto result = Journal.Load();
						LoadResult decoded{};
						decoded.JournalStatus = result.Status;
						if (!result.HasSnapshot()) { return decoded; }

						decoded.Sequence = result.Sequence;
						EnvelopeType envelope{};
						memcpy(&envelope, &result.Value, sizeof(envelope));
						if (envelope.Size == 0 || envelope.Size > Capacity) { return decoded; }
						decoded.CodecStatus = Dispatcher::Decode(envelope.Bytes, envelope.Size, decoded.Record);
						return decoded;
					}

					bool Save(const RecordType& record)
					{
						EnvelopeType envelope{};
						return Encode(record, envelope) && Journal.Save(envelope);
					}

					/// <summary>Starts a typed save; false means encoding failed or the journal could not be acquired.</summary>
					bool BeginSave(const RecordType& record, SaveOperation& operation)
					{
						EnvelopeType envelope{};
						return Encode(record, envelope) && Journal.BeginSave(envelope, operation);
					}
				};
			}
		}
	}
}

#endif
