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
					template<typename TData, bool TEnabled>
					class PendingWriteSlot;

					template<typename TData>
					class PendingWriteSlot<TData, false>
					{
					public:
						bool HasValue() const { return false; }
						void Set(const TData&) {}
						void Clear() {}
						TData Get() const { return TData{}; }
					};

					template<typename TData>
					class PendingWriteSlot<TData, true>
					{
					private:
						TData Value{};
						bool HasPendingValue = false;

					public:
						bool HasValue() const { return HasPendingValue; }
						void Set(const TData& value)
						{
							Value = value;
							HasPendingValue = true;
						}
						void Clear() { HasPendingValue = false; }
						TData Get() const { return Value; }
					};
				}

				/// <summary>Runs cooperative EEPROM writes from a TaskScheduler task.</summary>
				/// <typeparam name="TStore">An AsyncStructStore type.</typeparam>
				/// <typeparam name="TRequestPolicy">Reject new writes while busy, or retain only the latest pending request.</typeparam>
				template<typename TStore,
					AsyncWriteRequestPolicyEnum TRequestPolicy = AsyncWriteRequestPolicyEnum::RejectWhileBusy>
				class AsyncWriteTask : public TS::Task
				{
				private:
					using TData = typename TStore::DataType;
					using AsyncWriteStatus = typename TStore::AsyncWriteStatus;
					static constexpr bool StoresPendingRequest = TRequestPolicy == AsyncWriteRequestPolicyEnum::ReplacePending;
					TStore& Store;
					Internal::PendingWriteSlot<TData, StoresPendingRequest> PendingWrite{};
					AsyncWriteStatus CurrentWriteStatus = AsyncWriteStatus::Idle;
					AsyncWriteStatus LastCompletedWriteStatus = AsyncWriteStatus::Idle;
					uint32_t CompletedWriteCount = 0;
					bool WriteActive = false;

					bool StartWrite(const TData& data)
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
					static_assert(TRequestPolicy == AsyncWriteRequestPolicyEnum::RejectWhileBusy
						|| TRequestPolicy == AsyncWriteRequestPolicyEnum::ReplacePending,
						"Unsupported EEPROM async write request policy.");

					AsyncWriteTask(TS::Scheduler& scheduler, TStore& store, const uint32_t pollPeriodMillis = 1)
						: TS::Task(pollPeriodMillis, TASK_FOREVER, &scheduler, false)
						, Store(store)
					{}

					/// <summary>Requests a write. ReplacePending retains only the latest request while a write is active.</summary>
					/// <param name="data">Record copied by the store when the write starts.</param>
					/// <returns>True if the write was started or accepted as the pending latest request.</returns>
					bool RequestWrite(const TData& data)
					{
						if (!WriteActive) { return StartWrite(data); }

						if (TRequestPolicy == AsyncWriteRequestPolicyEnum::ReplacePending)
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
							const TData next = PendingWrite.Get();
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
