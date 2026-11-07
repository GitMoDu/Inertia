#ifndef _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_ASYNC_VERSIONED_RECORD_JOURNAL_TASK_h
#define _INERTIA_COMPONENTS_STORAGE_EEPROM_JOURNAL_ASYNC_VERSIONED_RECORD_JOURNAL_TASK_h

#ifndef _TASK_OO_CALLBACKS
#define _TASK_OO_CALLBACKS
#endif
#include <TScheduler.hpp>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace EepromJournal
			{
				/// <summary>Runs one accepted typed journal save at a time; requests are rejected while busy, never queued or replaced.</summary>
				template<typename VersionedJournalType>
				class AsyncVersionedRecordJournalTask : public TS::Task
				{
				private:
					using RecordType = typename VersionedJournalType::RecordType;
					using SaveOperation = typename VersionedJournalType::SaveOperation;
					using SaveStatus = typename VersionedJournalType::SaveStatus;

					VersionedJournalType& Storage;
					SaveOperation Operation{};
					SaveStatus LastCompletedStatus = SaveStatus::Idle;

				public:
					AsyncVersionedRecordJournalTask(TS::Scheduler& scheduler, VersionedJournalType& storage,
						const uint32_t callbackPeriodMillis = 1)
						: TS::Task(callbackPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Storage(storage)
					{}

					/// <summary>Returns true only when this record starts immediately; false requests are not retained.</summary>
					bool RequestSave(const RecordType& record)
					{
						if (Operation.Status() == SaveStatus::Pending) { return false; }
						if (!Storage.BeginSave(record, Operation)) { return false; }
						TS::Task::enableDelayed(0);
						return true;
					}

					bool IsSaveActive() const { return Operation.Status() == SaveStatus::Pending; }
					/// <summary>Returns the last terminal result for accepted requests only.</summary>
					SaveStatus GetLastCompletedStatus() const { return LastCompletedStatus; }

					bool Callback() final
					{
						if (Operation.Status() == SaveStatus::Pending)
						{
							const SaveStatus status = Operation.Step();
							if (status == SaveStatus::Completed || status == SaveStatus::Failed)
							{
								LastCompletedStatus = status;
								TS::Task::disable();
							}
						}
						else
						{
							TS::Task::disable();
						}
						return true;
					}
				};
			}
		}
	}
}

#endif

