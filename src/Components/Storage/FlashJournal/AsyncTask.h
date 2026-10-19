#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_ASYNC_TASK_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_ASYNC_TASK_h

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
				/// <summary>
				/// Processes journal requests in FIFO order:
				/// Head -> [Save A] -> [Erase page] -> [Save B] -> Tail.
				/// Each save owns its enqueue-time snapshot. Queue acceptance does not guarantee persistence success.
				/// Programming advances by at most one flash mutation per callback; scanning, erase, checksum, and
				/// verification work may still take synchronous time, so this is cooperative rather than hard real-time.
					/// Direct saves and maintenance erases must not run while this task has pending operations.
				/// </summary>
				template<typename Journal, uint16_t QueueCapacity>
				class AsyncFlashJournalTask : public TS::Task
				{
				private:
					using Entry = typename Journal::entry_type;
					using SaveStatus = typename Journal::SaveStatus;
					using SaveError = typename Journal::SaveError;
					using SaveOperation = typename Journal::SaveOperation;
					enum class OperationType : uint8_t
					{
						Save,
						ErasePage
					};

					struct Operation
					{
						SaveOperation Save{};
						OperationType Type = OperationType::Save;
						uint16_t PageIndex = 0;
					};

					Operation Operations[QueueCapacity];
					uint16_t Head = 0;
					uint16_t Tail = 0;
					uint16_t Count = 0;
					Journal& Storage;

					bool LastOperationSucceeded = true;
					SaveError LastOperationFailure = SaveError::None;
					uint32_t CompletedOperationCount = 0;

					bool EnqueueSave(const Entry& value)
					{
						if (Count == QueueCapacity)
							return false;

						Operation& operation = Operations[Tail];
						operation.Type = OperationType::Save;
						Storage.BeginSave(value, operation.Save);
						AdvanceTail();
						return true;
					}

					bool EnqueuePageErase(const uint16_t pageIndex)
					{
						if (Count == QueueCapacity)
							return false;

						Operation& operation = Operations[Tail];
						operation.Type = OperationType::ErasePage;
						operation.PageIndex = pageIndex;
						AdvanceTail();
						return true;
					}

					void AdvanceTail()
					{
						Tail = uint16_t((Tail + 1) % QueueCapacity);
						++Count;
						TS::Task::enableDelayed(0);
					}

				public:
					static_assert(QueueCapacity > 0, "The asynchronous journal queue must have at least one slot.");

					AsyncFlashJournalTask(TS::Scheduler& scheduler, Journal& storage, const uint32_t callbackPeriodMillis = 1)
						: TS::Task(callbackPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Storage(storage)
					{}

					bool QueueSave(const Entry& entry)
					{
						return EnqueueSave(entry);
					}

					bool QueuePageErase(const uint16_t pageIndex)
					{
						if (pageIndex >= Journal::GetPageCount())
							return false;
						return EnqueuePageErase(pageIndex);
					}

					uint16_t GetPendingOperationCount() const
					{
						return Count;
					}

					bool GetLastOperationSucceeded() const
					{
						return LastOperationSucceeded;
					}

					SaveError GetLastOperationError() const
					{
						return LastOperationFailure;
					}

					uint32_t GetCompletedOperationCount() const
					{
						return CompletedOperationCount;
					}

					virtual bool Callback() final
					{
						if (Count == 0)
						{
							TS::Task::disable();
							return true;
						}

						Operation& operation = Operations[Head];
						bool operationComplete = false;
						if (operation.Type == OperationType::ErasePage)
						{
							LastOperationSucceeded = Storage.ErasePageForMaintenance(operation.PageIndex);
							LastOperationFailure = LastOperationSucceeded ? SaveError::None : SaveError::FlashFailure;
							operationComplete = true;
						}
						else
						{
							const SaveStatus status = operation.Save.Step();
							if (status == SaveStatus::Failed)
							{
								LastOperationSucceeded = false;
								LastOperationFailure = operation.Save.Error();
								operationComplete = true;
							}
							else if (status == SaveStatus::Completed)
							{
								LastOperationSucceeded = true;
								LastOperationFailure = SaveError::None;
								operationComplete = true;
							}
						}

						if (operationComplete)
						{
							++CompletedOperationCount;
							Head = uint16_t((Head + 1) % QueueCapacity);
							--Count;
							if (Count == 0)
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