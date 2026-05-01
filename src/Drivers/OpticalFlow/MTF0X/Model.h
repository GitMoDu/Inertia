#ifndef _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_MODEL_h
#define _INERTIA_DRIVERS_OPTICAL_FLOW_MTF_0X_MODEL_h

#include "../../../Components/Core/Primitives.h"
#include "../../../Components/Core/Lifecycle/Model.h"

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
					static constexpr uint32_t LOG_TAG = 4140776039;

					enum class LogCodeEnum : uint8_t
					{
						TaskDriverStartFailed,
						WarningSerialReadLimitReached,
						ErrorOversizePayload,
						ErrorPayloadCrc,
						ErrorUnexpectedPayloadSize
					};

					using ILifecycleDriver = Inertia::Components::Lifecycle::ILifecycleDriver;

					template<typename TData>
					using IDataSource = Inertia::Components::DataSource::IDataSource<TData>;

					using range16_t = Inertia::Model::range16_t;
					using timestamped_quality_flow_translation_t = Inertia::Model::timestamped_quality_flow_translation_t;
					using timestamped_quality_range16_t = Inertia::Model::timestamped_quality_range16_t;
				}
			}
		}
	}
}

#endif