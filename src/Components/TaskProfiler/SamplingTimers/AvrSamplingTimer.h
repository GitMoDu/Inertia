#ifndef _INERTIA_COMPONENTS_TASK_PROFILER_AVR_SAMPLING_TIMER_h
#define _INERTIA_COMPONENTS_TASK_PROFILER_AVR_SAMPLING_TIMER_h

#if defined(ARDUINO_ARCH_AVR)
#include <avr/interrupt.h>
#include <avr/io.h>

#include "../Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace TaskProfiler
		{
			namespace SamplingTimers
			{
				enum class AvrTimerEnum : uint8_t
				{
					//Timer0ChannelA,
					//Timer0ChannelB,
					Timer1ChannelA,
					Timer1ChannelB,
					//Timer2ChannelA,
					//Timer2ChannelB,
					EnumCount
				};

				template<AvrTimerEnum SelectedTimer>
				class AvrSamplingTimer : public ISamplingTimer
				{
				public:
					virtual bool Start(const uint32_t, IInterruptCallback*) override
					{
						static_assert(SelectedTimer == AvrTimerEnum::Timer1ChannelA,
							"AVR sampling timer is currently implemented only for Timer1ChannelA.");
						return false;
					}

					virtual void Stop() override
					{}
				};

				template<>
				class AvrSamplingTimer<AvrTimerEnum::Timer1ChannelA> : public ISamplingTimer
				{
				private:
					struct TimerConfig
					{
						uint16_t CompareValue = 0;
						uint8_t PrescalerBits = 0;
						bool IsValid = false;
					};

				private:
					IInterruptCallback* Callback = nullptr;
					uint8_t SavedTccr1a = 0;
					uint8_t SavedTccr1b = 0;
					uint8_t SavedTimsk1 = 0;
					uint16_t SavedOcr1a = 0;
					uint16_t SavedTcnt1 = 0;
					bool Running = false;

				private:
					static AvrSamplingTimer*& ActiveInstance()
					{
						static AvrSamplingTimer* instance = nullptr;
						return instance;
					}

					static TimerConfig BuildTimerConfig(const uint32_t periodMicros)
					{
						struct PrescalerOption
						{
							uint16_t Divisor;
							uint8_t Bits;
						};

						static constexpr PrescalerOption Options[] = {
							{ 1, _BV(CS10) },
							{ 8, _BV(CS11) },
							{ 64, _BV(CS11) | _BV(CS10) },
							{ 256, _BV(CS12) },
							{ 1024, _BV(CS12) | _BV(CS10) },
						};

						for (const auto& option : Options)
						{
							const uint32_t ticks = static_cast<uint32_t>(
								(static_cast<uint64_t>(F_CPU) * periodMicros)
								/ (static_cast<uint64_t>(option.Divisor) * 1000000ULL));

							if (ticks > 0 && ticks <= 65536UL)
							{
								TimerConfig config{};
								config.CompareValue = static_cast<uint16_t>(ticks - 1UL);
								config.PrescalerBits = option.Bits;
								config.IsValid = true;
								return config;
							}
						}

						return {};
					}

				public:
					static void HandleCompareMatch()
					{
						auto* const activeInstance = ActiveInstance();
						if (activeInstance != nullptr
							&& activeInstance->Callback != nullptr)
						{
							activeInstance->Callback->OnInterrupt();
						}
					}

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

						const TimerConfig config = BuildTimerConfig(periodMicros);
						if (!config.IsValid)
						{
							return false;
						}

						const uint8_t savedSreg = SREG;
						cli();

						SavedTccr1a = TCCR1A;
						SavedTccr1b = TCCR1B;
						SavedTimsk1 = TIMSK1;
						SavedOcr1a = OCR1A;
						SavedTcnt1 = TCNT1;

						Callback = callback;
						ActiveInstance() = this;
						Running = true;

						TCCR1A = 0;
						TCCR1B = 0;
						TCNT1 = 0;
						OCR1A = config.CompareValue;
						TIFR1 = _BV(OCF1A);
						TIMSK1 = SavedTimsk1 | _BV(OCIE1A);
						TCCR1B = _BV(WGM12) | config.PrescalerBits;

						SREG = savedSreg;
						return true;
					}

					virtual void Stop() override
					{
						const uint8_t savedSreg = SREG;
						cli();

						if (Running)
						{
							TIMSK1 = SavedTimsk1;
							TCCR1A = SavedTccr1a;
							TCCR1B = SavedTccr1b;
							OCR1A = SavedOcr1a;
							TCNT1 = SavedTcnt1;
						}

						Callback = nullptr;
						if (ActiveInstance() == this)
						{
							ActiveInstance() = nullptr;
						}
						Running = false;

						SREG = savedSreg;
					}
				};

				ISR(TIMER1_COMPA_vect)
				{
					AvrSamplingTimer<AvrTimerEnum::Timer1ChannelA>::HandleCompareMatch();
				}
			}
		}
	}
}
#endif
#endif
