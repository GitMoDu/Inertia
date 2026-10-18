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
				static constexpr uint16_t CommitMarker = 0xA55A;

				/// <summary>Result reported while an incremental journal save is being processed.</summary>
				enum class SaveStepResultEnum
				{
					InProgress,
					Completed,
					Failed,
					EnumCount
				};

				/// <summary>
				/// Current stage of an incremental flash-journal save:
				/// NotStarted -> [Erase] -> ProgramSequence -> ProgramPayload -> ProgramChecksum
				///            -> ProgramCommit -> Verify -> Completed
				/// Erase is skipped when the current page has an available slot; any failed step ends in Failed.
				/// </summary>
				enum class SavePhaseEnum
				{
					NotStarted,
					Erase,
					ProgramSequence,
					ProgramPayload,
					ProgramChecksum,
					ProgramCommit,
					Verify,
					Completed,
					Failed,
					EnumCount
				};

				/// <summary>
				/// State passed from BeginSave to each SaveStep call. Address, page, and sequence are selected once;
				/// Offset tracks progress through the sequence and payload fields.
				/// </summary>
				struct SaveOperation
				{
					SavePhaseEnum CurrentPhase = SavePhaseEnum::NotStarted;
					uint32_t Address = 0;
					uint32_t PageAddress = 0;
					uint32_t Sequence = 0;
					uint16_t Offset = 0;
				};
			}
		}
	}
}

#endif
