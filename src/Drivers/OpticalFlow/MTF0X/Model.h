#ifndef _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_MODEL_h
#define _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_MODEL_h

#include "../../../Framework/Model.h"
#include "../../../Framework/Interface.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace OpticalFlow
		{
			namespace MTF0X
			{
				namespace Model
				{
					using ILifecycleDriver = Inertia::Model::ILifecycleDriver;

					template<typename TData>
					using IDataSource = Inertia::Model::IDataSource<TData>;

					using range16_t = Inertia::Model::range16_t;
					using timestamped_quality_flow_translation_t = Inertia::Model::timestamped_quality_flow_translation_t;
					using timestamped_quality_range16_t = Inertia::Model::timestamped_quality_range16_t;

					static constexpr uint32_t LOG_TAG = 4140776039;

					enum class LogCodeEnum : uint8_t
					{
						TaskDriverStartFailed,
						WarningSerialReadLimitReached,
						ErrorOversizePayload,
						ErrorPayloadCrc,
						ErrorUnexpectedPayloadSize
					};
				}
			}
		}
	}
}

#endif