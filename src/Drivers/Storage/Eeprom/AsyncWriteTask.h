#ifndef _INERTIA_DRIVERS_STORAGE_EEPROM_ASYNC_WRITE_TASK_h
#define _INERTIA_DRIVERS_STORAGE_EEPROM_ASYNC_WRITE_TASK_h

#ifndef _TASK_OO_CALLBACKS
#define _TASK_OO_CALLBACKS
#endif
#include <TSchedulerDeclarations.hpp>
#include <stdint.h>

#include "StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Eeprom
			{
				enum class AsyncWriteRequestPolicyEnum : uint8_t
				{
					RejectWhileBusy = 0,
					ReplacePending,
					EnumCount
				};

				namespace Internal
				{
					template<typename DataType, bool Enabled>
					class PendingWriteSlot;

					template<typename DataType>
					class PendingWriteSlot<DataType, false>
					{
					public:
						bool HasValue() const { return false; }
						void Set(const DataType&) {}
						void Clear() {}
						DataType Get() const { return DataType{}; }
					};

					template<typename DataType>
					class PendingWriteSlot<DataType, true>
					{
					private:
						DataType Value{};
						bool HasPendingValue = false;

					public:
						bool HasValue() const { return HasPendingValue; }
						void Set(const DataType& value)
						{
							Value = value;
							HasPendingValue = true;
						}
						void Clear() { HasPendingValue = false; }
						DataType Get() const { return Value; }
					};
				}

				/// <summary>Runs cooperative EEPROM writes from a TaskScheduler task.</summary>
				/// <typeparam name="StoreType">An AsyncStructStore type.</typeparam>
				/// <typeparam name="RequestPolicyEnum">Reject new writes while busy, or retain only the latest pending request.</typeparam>
				template<typename StoreType,
					AsyncWriteRequestPolicyEnum RequestPolicyEnum = AsyncWriteRequestPolicyEnum::RejectWhileBusy>
				class AsyncWriteTask : public TS::Task
				{
				private:
					using DataType = typename StoreType::DataType;
					using AsyncWriteStatus = typename StoreType::AsyncWriteStatus;
					static constexpr bool StoresPendingRequest = RequestPolicyEnum == AsyncWriteRequestPolicyEnum::ReplacePending;
					StoreType& Store;
					Internal::PendingWriteSlot<DataType, StoresPendingRequest> PendingWrite{};
					AsyncWriteStatus CurrentWriteStatus = AsyncWriteStatus::Idle;
					AsyncWriteStatus LastCompletedWriteStatus = AsyncWriteStatus::Idle;
					uint32_t CompletedWriteCount = 0;
					bool WriteActive = false;

					bool StartWrite(const DataType& data)
					{
						if (!Store.BeginWrite(data))
						{
							CurrentWriteStatus = AsyncWriteStatus::Failed;
							return false;
						}

						WriteActive = true;
						CurrentWriteStatus = AsyncWriteStatus::Pending;
						TS::Task::enableDelayed(0);
						return true;
					}

				public:
					static_assert(RequestPolicyEnum == AsyncWriteRequestPolicyEnum::RejectWhileBusy
						|| RequestPolicyEnum == AsyncWriteRequestPolicyEnum::ReplacePending,
						"Unsupported EEPROM async write request policy.");

					AsyncWriteTask(TS::Scheduler& scheduler, StoreType& store, const uint32_t pollPeriodMillis = 1)
						: TS::Task(pollPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Store(store)
					{}

					/// <summary>Requests a write. ReplacePending retains only the latest request while a write is active.</summary>
					/// <param name="data">Record copied by the store when the write starts.</param>
					/// <returns>True if the write was started or accepted as the pending latest request.</returns>
					bool RequestWrite(const DataType& data)
					{
						if (!WriteActive) { return StartWrite(data); }

						if (RequestPolicyEnum == AsyncWriteRequestPolicyEnum::ReplacePending)
						{
							PendingWrite.Set(data);
							return true;
						}

						return false;
					}

					bool IsWriteActive() const { return WriteActive; }
					bool HasPendingWrite() const { return PendingWrite.HasValue(); }
					AsyncWriteStatus GetCurrentWriteStatus() const { return CurrentWriteStatus; }
					AsyncWriteStatus GetLastCompletedWriteStatus() const { return LastCompletedWriteStatus; }
					uint32_t GetCompletedWriteCount() const { return CompletedWriteCount; }

					/// <summary>Polls the active EEPROM write and starts a queued latest request, if configured.</summary>
					bool Callback() final
					{
						if (!WriteActive)
						{
							TS::Task::disable();
							return true;
						}

						CurrentWriteStatus = Store.PollWrite();
						if (CurrentWriteStatus == AsyncWriteStatus::Pending) { return true; }

						LastCompletedWriteStatus = CurrentWriteStatus;
						WriteActive = false;
						++CompletedWriteCount;
						if (PendingWrite.HasValue())
						{
							const DataType next = PendingWrite.Get();
							PendingWrite.Clear();
							StartWrite(next);
							return true;
						}

						TS::Task::disable();
						return true;
					}
				};
			}
		}
	}
}

#endif
