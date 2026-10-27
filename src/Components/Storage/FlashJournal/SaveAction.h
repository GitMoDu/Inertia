#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_SAVE_ACTION_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_SAVE_ACTION_h

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				namespace Detail
				{
					template<uint16_t PayloadSize>
					class SaveAction
					{
					private:
						ISaveActionOwner* Owner = nullptr;
						uint8_t Payload[PayloadSize];
						SaveActionState CurrentState;

					public:
						SaveAction() = default;

						SaveAction(const SaveAction&) = delete;
						SaveAction& operator=(const SaveAction&) = delete;
						SaveAction(SaveAction&&) = delete;
						SaveAction& operator=(SaveAction&&) = delete;

						~SaveAction()
						{
							Abandon();
						}

						SaveStatusEnum Step()
						{
							if (Owner == nullptr)
								return CurrentState.CurrentStatus;
							const SaveStatusEnum status = Owner->StepSave(CurrentState, Payload);
							if (status != SaveStatusEnum::InProgress)
								Detach();
							return status;
						}

						SaveStatusEnum Status() const
						{
							return CurrentState.CurrentStatus;
						}

						SaveErrorEnum Error() const
						{
							return CurrentState.CurrentError;
						}

						uint32_t Sequence() const
						{
							return CurrentState.SequenceValue;
						}

						void Start(ISaveActionOwner* owner, const uint8_t* payload)
						{
							Abandon();
							for (uint16_t index = 0; index < PayloadSize; ++index)
								Payload[index] = payload[index];
							CurrentState = {};
							if (owner->InitializeSave(CurrentState))
								Owner = owner;
						}

						void Abandon()
						{
							if (Owner != nullptr)
							{
								CurrentState.CurrentStatus = SaveStatusEnum::Failed;
								CurrentState.CurrentError = SaveErrorEnum::Abandoned;
								Owner->Release(CurrentState);
								Detach();
							}
						}

					private:
						void Detach()
						{
							Owner = nullptr;
						}
					};
				}
			}
		}
	}
}

#endif
