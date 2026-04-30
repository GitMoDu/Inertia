#ifndef _INERTIA_DRIVERS_TASK_PROFILER_STM_SAMPLING_TIMER_h
#define _INERTIA_DRIVERS_TASK_PROFILER_STM_SAMPLING_TIMER_h

#if defined(ARDUINO_ARCH_STM32)
#include <HardwareTimer.h>

#include "../../../Components/TaskProfiler/Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace TaskProfiler
		{
			using namespace Inertia::Components::TaskProfiler;

			namespace SamplingTimers
			{
				template<uint8_t Index>
				class StmSamplingTimer : public ISamplingTimer
				{
				private:
					IInterruptCallback* Callback = nullptr;
					HardwareTimer* Timer = nullptr;
					bool Running = false;

				private:
					static TIM_TypeDef* GetTimerInstance()
					{
						static TIM_TypeDef* const TimerInstances[] = {
						#if defined(TIM1)
							TIM1,
						#endif
						#if defined(TIM2)
							TIM2,
						#endif
						#if defined(TIM3)
							TIM3,
						#endif
						#if defined(TIM4)
							TIM4,
						#endif
						#if defined(TIM5)
							TIM5,
						#endif
						#if defined(TIM6)
							TIM6,
						#endif
						#if defined(TIM7)
							TIM7,
						#endif
						#if defined(TIM8)
							TIM8,
						#endif
						#if defined(TIM9)
							TIM9,
						#endif
						#if defined(TIM10)
							TIM10,
						#endif
						#if defined(TIM11)
							TIM11,
						#endif
						#if defined(TIM12)
							TIM12,
						#endif
						#if defined(TIM13)
							TIM13,
						#endif
						#if defined(TIM14)
							TIM14,
						#endif
						#if defined(TIM15)
							TIM15,
						#endif
						#if defined(TIM16)
							TIM16,
						#endif
						#if defined(TIM17)
							TIM17,
						#endif
						#if defined(TIM18)
							TIM18,
						#endif
						#if defined(TIM19)
							TIM19,
						#endif
						#if defined(TIM20)
							TIM20,
						#endif
						#if defined(TIM21)
							TIM21,
						#endif
						#if defined(TIM22)
							TIM22,
						#endif
						};

						return Index < (sizeof(TimerInstances) / sizeof(TimerInstances[0]))
							? TimerInstances[Index]
							: nullptr;
					}

					static HardwareTimer* GetHardwareTimer()
					{
						if (Timer == nullptr)
						{
							auto* const timerInstance = GetTimerInstance();
							if (timerInstance == nullptr)
							{
								return nullptr;
							}

							Timer = new HardwareTimer(timerInstance);
						}

						return Timer;
					}

				public:
					~StmSamplingTimer()
					{
						Stop();
						if (Timer != nullptr)
						{
							delete Timer;
							Timer = nullptr;
						}
					}

					virtual bool Start(const uint32_t periodMicros, IInterruptCallback* callback) override
					{
						if (periodMicros == 0 || callback == nullptr)
						{
							return false;
						}

						auto* const hardwareTimer = GetHardwareTimer();
						if (hardwareTimer == nullptr)
						{
							return false;
						}

						if (Running)
						{
							Stop();
						}

						ActiveInstance = this;
						Callback = callback;
						hardwareTimer->pause();
						hardwareTimer->detachInterrupt();
						hardwareTimer->setOverflow(periodMicros, MICROSEC_FORMAT);
						hardwareTimer->attachInterrupt(&StmSamplingTimer::TimerISR);
						Running = true;
						hardwareTimer->resume();

						return true;
					}

					virtual void Stop() override
					{
						auto* const hardwareTimer = GetHardwareTimer();
						if (hardwareTimer != nullptr)
						{
							hardwareTimer->pause();
							hardwareTimer->detachInterrupt();
						}

						Callback = nullptr;
						Running = false;
					}
				};
			}
		}
	}
#endif
#endif