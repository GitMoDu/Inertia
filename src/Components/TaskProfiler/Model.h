#ifndef _INERTIA_COMPONENTS_TASK_PROFILER_MODEL_h
#define _INERTIA_COMPONENTS_TASK_PROFILER_MODEL_h

#include <IntegerSignal.h>

#include "../../Framework/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace TaskProfiler
		{
			static constexpr uint32_t LOG_TAG = 3527493342; // Random unique tag for TaskProfiler.

			enum class LogCodeEnum : uint8_t
			{
				EnumCount
			};

			struct IInterruptCallback
			{
				~IInterruptCallback() = default;
				virtual void OnInterrupt() = 0;
			};

			struct ISamplingTimer
			{
				~ISamplingTimer() = default;

				virtual bool Start(const uint32_t periodMicros, IInterruptCallback* callback) = 0;
				virtual void Stop() = 0;
			};

			struct IMeasureOutput
			{
				~IMeasureOutput() = default;
				virtual void OutputStart() = 0;
				virtual void OutputStop() = 0;
				virtual void EmitSamples(const uint32_t timestamp, const uint8_t* ids, const uint8_t count) = 0;
			};
		}
	}
}
#endif