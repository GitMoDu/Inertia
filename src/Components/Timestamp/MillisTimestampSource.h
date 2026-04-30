#ifndef _INERTIA_COMPONENTS_TIMESTAMP_MILLIS_TIMESTAMP_SOURCE_h
#define _INERTIA_COMPONENTS_TIMESTAMP_MILLIS_TIMESTAMP_SOURCE_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Timestamp
		{
			struct MillisTimestampSource : public IMillisTimestampSource
				, public TS::Task
			{
			private:
				enum class StateEnum
				{
					Uninitialized,
					Running,
					Overflowed
				};

			private:
				static constexpr uint32_t CHECK_TOLERANCE = 100;

			private:
				uint32_t BootTimestamp = 0;
				uint32_t LastCheckTimestamp = 0;
				uint16_t OverflowCount = 0;

			public:
				MillisTimestampSource()
					: IMillisTimestampSource()
					, TS::Task(TASK_IMMEDIATE, TASK_FOREVER, nullptr, false)
				{}

				~MillisTimestampSource() = default;

				void Start()
				{
					BootTimestamp = millis();
					LastCheckTimestamp = 0;
					OverflowCount = 0;
					TS::Task::enableDelayed(1);
				}

				bool Callback() override
				{
					const uint32_t currentTimestamp = millis() - BootTimestamp;
					if (currentTimestamp < LastCheckTimestamp)
					{
						OverflowCount++;
					}

					LastCheckTimestamp = currentTimestamp;

					const uint32_t timeUntilOverflow = UINT32_MAX - currentTimestamp;

					// Check every millisecond when we are close to overflow, to minimize the delay after overflow.
					const uint32_t waitDelay = timeUntilOverflow > CHECK_TOLERANCE ? timeUntilOverflow - CHECK_TOLERANCE : 1;
					TS::Task::delay(waitDelay);

					return true;
				}

				virtual millis_timestamp_t GetMillisTimestamp() override
				{
					const uint32_t currentTimestamp = millis() - BootTimestamp;

					// Opportunity to catch an overflow that might have happened since the last check in Callback, to ensure we never return a timestamp that goes backwards.
					if (currentTimestamp < LastCheckTimestamp)
					{
						OverflowCount++;
						TS::Task::enableDelayed(0); // Trigger an immediate check in Callback to update the overflow count and last timestamp.
					}

					LastCheckTimestamp = currentTimestamp;

					return millis_timestamp_t(
						currentTimestamp,
						OverflowCount);
				}
			};
		}
	}
}
#endif