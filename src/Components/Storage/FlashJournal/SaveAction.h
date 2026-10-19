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
					template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t PageCount, uint16_t PayloadSize>
					class ByteFlashJournal;

					template<uint16_t PayloadSize>
					class SaveAction
					{
					private:
						template<typename Flash, uint32_t TargetFlashSizeBytes, uint16_t PageCount, uint16_t JournalPayloadSize>
						friend class ByteFlashJournal;

						SaveActionOwner* Owner = nullptr;
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

					private:
						void SetPayload(const uint8_t* payload)
						{
							for (uint16_t index = 0; index < PayloadSize; ++index)
								Payload[index] = payload[index];
						}

						SaveActionState& GetState()
						{
							return CurrentState;
						}

					public:
						void Abandon()
						{
							if (Owner != nullptr)
							{
								Owner->Release(CurrentState);
								Detach();
							}
						}

					private:
						void Attach(SaveActionOwner* owner)
						{
							Owner = owner;
						}

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
