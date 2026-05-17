#ifndef _INERTIA_COMPONENTS_LINK_MODEL_h
#define _INERTIA_COMPONENTS_LINK_MODEL_h

#include "../../Components/Core/Primitives.h"
#include "../../Components/Core/Lifecycle/Model.h"
#include "../../Components/Core/DataSource/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Link
		{
			static constexpr uint32_t LOG_TAG = 813426707; // Random unique tag for Link logs.

			enum class LinkEnum : uint8_t
			{
				NoLink,
				Linking,
				Linked,
				EnumCount
			};

			enum class LinkingEnum : uint8_t
			{
				None,
				ScanningNew,
				PairingNew,
				ScanningExisting,
				EnumCount
			};

			struct LinkStateStruct
			{
				uint32_t SessionId = 0;
				LinkEnum State = LinkEnum::NoLink;
				LinkingEnum LinkingState = LinkingEnum::None;

				void Clear()
				{
					SessionId = 0;
					State = LinkEnum::NoLink;
					LinkingState = LinkingEnum::None;
				}
			};

			struct LinkCountersStruct
			{
				uint32_t RxBytes = 0;
				uint32_t TxBytes = 0;
				uint32_t RxCount = 0;
				uint32_t TxCount = 0;
				uint16_t RxError = 0;
				uint16_t TxError = 0;

				void Clear()
				{
					memset(this, 0, sizeof(LinkCountersStruct));
				}
			};

			struct ILinkDriver : public Inertia::Components::Lifecycle::ILifecycleDriver
				, public Inertia::Components::DataSource::IObservable<LinkStateStruct>
				, public Inertia::Components::DataSource::IDataSource<LinkCountersStruct>
			{
				~ILinkDriver() = default;
			};
		}
	}
}
#endif