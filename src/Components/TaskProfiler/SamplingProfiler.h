#ifndef _INERTIA_COMPONENTS_TASK_PROFILER_SAMPLING_PROFILER_h
#define _INERTIA_COMPONENTS_TASK_PROFILER_SAMPLING_PROFILER_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#include "Model.h"

#include "../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace TaskProfiler
		{
			template<uint32_t SlotSize,
				uint8_t SamplesPerMillisecond
			>
			class SamplingProfiler
				: public Inertia::Components::Lifecycle::ILifecycleDriver
				, public IInterruptCallback
				, public TS::Task
			{
			public:
				static constexpr uint32_t SamplingPeriodMicros = 1000 / SamplesPerMillisecond;

			private:
				template<uint8_t SamplesPerSlot>
				struct SampleBucket
				{
					uint32_t Timestamp = 0;
					uint8_t Ids[SamplesPerSlot]{};
					uint8_t Count = 0;

					void AddId(const uint32_t id)
					{
						if (Count < SamplesPerSlot)
						{
							Ids[Count++] = id;
						}
					}
				};

			private:
				enum class MeasureStateEnum : uint8_t
				{
					Disabled,
					Starting,
					Measuring,
					Completed
				};

			private:
				static constexpr uint8_t SamplesPerSlot = SamplesPerMillisecond >= UINT8_MAX
					? UINT8_MAX
					: SamplesPerMillisecond + 1;    // +1 for overlap margin

			private:
				SampleBucket<SamplesPerSlot> Slots[SlotSize]{};
				uint32_t SlotIndex = 0;

				uint32_t StartDelayMillis = 0;
				MeasureStateEnum State = MeasureStateEnum::Disabled;

			private:
				TS::Scheduler& Scheduler;
				ISamplingTimer& SamplingTimer;

				IMeasureOutput* Output;


			public:
				SamplingProfiler(TS::Scheduler& scheduler,
					ISamplingTimer& samplingTimer,
					IMeasureOutput* output = nullptr)
					: Inertia::Components::Lifecycle::ILifecycleDriver()
					, IInterruptCallback()
					, TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
					, Scheduler(scheduler)
					, SamplingTimer(samplingTimer)
					, Output(output)
				{}

				~SamplingProfiler() = default;

				void SetStartDelay(const uint32_t delayMillis)
				{
					StartDelayMillis = delayMillis;
				}

				virtual bool Callback() override
				{
					TS::Task::disable();

					switch (State)
					{
					case MeasureStateEnum::Starting:
						SlotIndex = 0;
						Slots[0].Timestamp = millis();
						if (SamplingTimer.Start(SamplingPeriodMicros, this))  // 'this' IS the callback
						{
							State = MeasureStateEnum::Measuring;
						}
						else
						{
							State = MeasureStateEnum::Disabled;
						}
						break;
					case MeasureStateEnum::Completed:
						if (Output != nullptr)
						{
							Output->OutputStart();
							for (uint32_t i = 0; i < SlotIndex; i++)
							{
								if (Slots[i].Count > 0)
								{
									Output->EmitSamples(Slots[i].Timestamp, Slots[i].Ids, Slots[i].Count);
								}
							}

							Output->OutputStop();
						}
						break;
					default:
						break;
					}


					return true;
				}

				virtual bool Start() override
				{
					for (uint32_t i = 0; i < SlotSize; i++)
					{
						Slots[i].Timestamp = 0;
						Slots[i].Count = 0;
					}

					State = MeasureStateEnum::Starting;
					TS::Task::enableDelayed(StartDelayMillis);

					return true;
				}

				virtual void Stop() override
				{
					SamplingTimer.Stop();
				}

				// IInterruptCallback -- called directly by the interrupt source, no static trampoline needed
				virtual void OnInterrupt() override
				{
					const uint32_t timestamp = millis();

					if (Slots[SlotIndex].Timestamp != timestamp)
					{
						SlotIndex++;
						if (SlotIndex >= SlotSize)
						{
							TS::Task::enable();
							SamplingTimer.Stop();
							State = MeasureStateEnum::Completed;
							return;
						}
						Slots[SlotIndex].Timestamp = timestamp;
					}

					auto* task = Scheduler.getCurrentTask();
					if (task != nullptr
						&& task->iStatus.enabled
						&& (task->iStatus.waiting == 0
							|| task->iStatus.inonenable)
						)
					{
						Slots[SlotIndex].AddId(task->getId());
					}
					else
					{
						Slots[SlotIndex].AddId(0);
					}
				}
			};

			class MeasurePrintOutput : public IMeasureOutput
			{
			private:
				Print& SerialInstance;

			public:
				MeasurePrintOutput(Print& serialInstance)
					: IMeasureOutput()
					, SerialInstance(serialInstance)
				{}

				~MeasurePrintOutput() = default;

				virtual void OutputStart() override
				{
					SerialInstance.println(F("Measurement Start"));
				}

				virtual void OutputStop() override
				{
					SerialInstance.println(F("Measurement Stop"));
					SerialInstance.flush();
				}

				virtual void EmitSamples(const uint32_t timestamp, const uint8_t* ids, const uint8_t count) override
				{
					SerialInstance.print(timestamp);
					SerialInstance.print(':');
					for (uint8_t i = 0; i < count; i++)
					{
						SerialInstance.print(ids[i]);
						if (i < count - 1)
						{
							SerialInstance.print(',');
						}
					}
					SerialInstance.println();
				}
			};
		}
	}
}
#endif