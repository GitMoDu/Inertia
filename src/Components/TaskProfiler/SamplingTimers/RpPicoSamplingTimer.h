#ifndef _INERTIA_COMPONENTS_TASK_PROFILER_RPICO_SAMPLING_TIMER_h
#define _INERTIA_COMPONENTS_TASK_PROFILER_RPICO_SAMPLING_TIMER_h

#if defined(ARDUINO_ARCH_RP2040)
#include "../Model.h"

#include <pico/time.h>

namespace Inertia
{
	namespace Components
	{
		namespace TaskProfiler
		{
			namespace SamplingTimers
			{
				class RpPicoSamplingTimer : public ISamplingTimer
				{
				private:
					repeating_timer_t Timer{};
					IInterruptCallback* Callback = nullptr;
					bool Running = false;

				private:
					// Static trampoline is LOCAL to this class -- bridges RP2040 C API to instance.
					// user_data carries 'this' (TimerInterruptSource*), no globals needed.
					static bool TimerISR(repeating_timer_t* rt)
					{
						auto* self = static_cast<RpPicoSamplingTimer*>(rt->user_data);
						if (self->Callback != nullptr)
						{
							self->Callback->OnInterrupt();
						}
						return true; // keep repeating
					}

				public:
					virtual bool Start(const uint32_t periodMicros, IInterruptCallback* callback) override
					{
						if (periodMicros == 0 || callback == nullptr)
						{
							return false;
						}

						if (Running)
						{
							Stop();
						}

						Callback = callback;
						Running = add_repeating_timer_us(
							-static_cast<int64_t>(periodMicros),
							&RpPicoSamplingTimer::TimerISR,
							this,
							&Timer);

						if (!Running)
						{
							Callback = nullptr;
						}

						return Running;
					}

					virtual void Stop() override
					{
						if (Running)
						{
							cancel_repeating_timer(&Timer);
							Callback = nullptr;
							Running = false;
						}
					}
				};
			}
		}
	}
}
#endif
#endif