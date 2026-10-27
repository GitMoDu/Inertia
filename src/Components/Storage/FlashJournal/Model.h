#ifndef _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_MODEL_h
#define _INERTIA_COMPONENTS_STORAGE_FLASH_JOURNAL_MODEL_h

#include "../../../Components/Core/Primitives.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace FlashJournal
			{
				static constexpr uint32_t LOG_TAG = 2697222751;

				/// <summary>Commit marker written last to identify a complete flash-journal record.</summary>
				static constexpr uint16_t CommitMarker = 0xAA55;

				/// <summary>Flash region and record layout reported by a FlashJournal.</summary>
				struct JournalGeometry
				{
					uint32_t StartAddress;
					uint32_t EndAddress;
					uint32_t RecordSizeBytes;
					uint32_t SlotsPerPage;
					uint16_t PageCount;
					uint16_t PayloadSizeBytes;
					uint16_t PaddedPayloadSizeBytes;
				};

				/// <summary>Result of loading the latest committed journal snapshot.</summary>
				enum class LoadStatusEnum
				{
					Loaded,
					NoValidSnapshot,
					StorageUnavailable,
					EnumCount
				};

				/// <summary>
				/// Result of an incremental save operation. Failed means the backend rejected a required operation;
				/// the backend cannot report a more specific hardware cause.
				/// </summary>
				enum class SaveStatusEnum
				{
					InProgress,
					Completed,
					Failed,
					EnumCount
				};

				/// <summary>Failure information supported by the journal and its statically bound backend.</summary>
				enum class SaveErrorEnum
				{
					None,
					StorageUnavailable,
					FlashFailure,
					VerificationFailure,
					Abandoned,
					EnumCount
				};

				enum class SaveActionPhaseEnum : uint8_t
				{
					NotStarted,
					Erase,
					ProgramSequence,
					ProgramPayload,
					ProgramChecksum,
					ProgramCommit,
					Verify,
					EnumCount
				};

				struct SaveActionState
				{
					uint32_t Address = 0;
					uint32_t PageAddress = 0;
					uint32_t SequenceValue = 0;
					uint16_t Offset = 0;
					SaveStatusEnum CurrentStatus = SaveStatusEnum::Failed;
					SaveErrorEnum CurrentError = SaveErrorEnum::None;
					SaveActionPhaseEnum CurrentPhase = SaveActionPhaseEnum::NotStarted;
					bool Active = false;
				};

				struct ISaveActionOwner
				{
					virtual bool InitializeSave(SaveActionState& state) = 0;
					virtual SaveStatusEnum StepSave(SaveActionState& state, const uint8_t* payload) = 0;
					virtual void Release(SaveActionState& state) = 0;
					virtual ~ISaveActionOwner() {}
				};
			}
		}
	}
}
#endif