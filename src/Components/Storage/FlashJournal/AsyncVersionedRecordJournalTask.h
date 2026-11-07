#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_ASYNC_VERSIONED_RECORD_JOURNAL_TASK_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_ASYNC_VERSIONED_RECORD_JOURNAL_TASK_h

#ifndef _TASK_OO_CALLBACKS
#define _TASK_OO_CALLBACKS
#endif
#include <TScheduler.hpp>
#include <stdint.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				template<typename VersionedJournalType>
				class AsyncVersionedRecordJournalTask : public TS::Task
				{
				private:
					using SaveStatus = typename VersionedJournalType::SaveStatus;
					using SaveOperation = typename VersionedJournalType::SaveOperation;

					VersionedJournalType& Storage;
					SaveOperation Operation{};
					SaveStatus LastCompletedStatus = SaveStatus::Idle;

				public:
					AsyncVersionedRecordJournalTask(TS::Scheduler& scheduler, VersionedJournalType& storage,
						const uint32_t callbackPeriodMillis = 1)
						: TS::Task(callbackPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Storage(storage)
					{}

					bool RequestSave(const typename VersionedJournalType::RecordType& record)
					{
						if (Operation.Status() == SaveStatus::Pending || !Storage.BeginSave(record, Operation)) { return false; }
						TS::Task::enableDelayed(0);
						return true;
					}

					bool IsSaveActive() const { return Operation.Status() == SaveStatus::Pending; }
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
