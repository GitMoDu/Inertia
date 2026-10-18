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
				/// Head -> [Save A] -> [Erase page] -> [Save B] -> Tail
				/// Save: BeginSave once -> SaveStep on later callbacks -> complete and remove from queue.
				/// Erase: erase the requested page -> complete and remove from queue in one callback.
				/// </summary>
				template<typename Journal, uint16_t QueueCapacity>
				class AsyncFlashJournalTask : public TS::Task
				{
				private:
					using Entry = typename Journal::entry_type;

					enum class OperationType : uint8_t
					{
						Save,
						ErasePage
					};

					struct Operation
					{
						OperationType Type;
						Entry Value;
						uint16_t PageIndex;
						bool SaveStarted;
						typename Journal::SaveOperation SaveState;
					};

					Operation Operations[QueueCapacity]{};
					uint16_t Head = 0;
					uint16_t Tail = 0;
					uint16_t Count = 0;
					Journal& Storage;
					using SaveStepResult = typename Journal::SaveStepResult;
					bool LastOperationSucceeded = true;
					uint32_t CompletedOperationCount = 0;

					bool Enqueue(const OperationType type, const Entry* value, const uint16_t pageIndex)
					{
						if (Count == QueueCapacity)
							return false;

						Operation& operation = Operations[Tail];
						operation.Type = type;
						if (value != nullptr)
							operation.Value = *value;
						operation.PageIndex = pageIndex;
						operation.SaveStarted = false;
						operation.SaveState = typename Journal::SaveOperation{};
						Tail = uint16_t((Tail + 1) % QueueCapacity);
						++Count;
						TS::Task::enableDelayed(0);
						return true;
					}

				public:
					static_assert(QueueCapacity > 0, "The asynchronous journal queue must have at least one slot.");

					AsyncFlashJournalTask(TS::Scheduler& scheduler, Journal& storage, const uint32_t callbackPeriodMillis = 1)
						: TS::Task(callbackPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Storage(storage)
					{}

					bool QueueSave(const Entry& entry)
					{
						return Enqueue(OperationType::Save, &entry, 0);
					}

					bool QueuePageErase(const uint16_t pageIndex)
					{
						if (pageIndex >= Journal::GetPageCount())
							return false;
						return Enqueue(OperationType::ErasePage, nullptr, pageIndex);
					}

					uint16_t GetPendingOperationCount() const
					{
						return Count;
					}

					bool GetLastOperationSucceeded() const
					{
						return LastOperationSucceeded;
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
							LastOperationSucceeded = Storage.EraseJournalPage(operation.PageIndex);
							operationComplete = true;
						}
						else
						{
							if (!operation.SaveStarted)
							{
								operation.SaveStarted = true;
								LastOperationSucceeded = Storage.BeginSave(operation.Value, operation.SaveState);
								if (!LastOperationSucceeded)
									operationComplete = true;
								else
									return true;
							}
							else
							{
								const SaveStepResult result = Storage.SaveStep(operation.Value, operation.SaveState);
								if (result == SaveStepResult::Failed)
								{
									LastOperationSucceeded = false;
									operationComplete = true;
								}
								else if (result == SaveStepResult::Completed)
								{
									LastOperationSucceeded = true;
									operationComplete = true;
								}
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