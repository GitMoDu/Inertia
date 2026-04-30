/*
* Task Profiler Example
*
* This example demonstrates how to use the Inertia TS::Task Profiler component,
* to profile task execution in an Arduino application.
* It sets up a sampling profiler that collects task execution samples at a specified rate
* and outputs them to the Serial console for analysis.
* Use Components/TaskProfiler/Viewer/TaskProfiler-TraceViewer.html to visualize the output data.
*/


#define SERIAL_BAUD_RATE 115200

// Task instrumentation for profiling. Required as first include before any TS::Task usage.
#include <InertiaTaskInstrumentation.h>

// Inertia components use OO callbacks.
#define _TASK_OO_CALLBACKS
#include <TScheduler.hpp>


#include <Arduino.h>
#include <InertiaModel.h>
#include <InertiaDrivers.h>

#include "TemplateTask.h"
#include "TaskTag.h"

static constexpr uint32_t StartDelay = 0;
static constexpr uint32_t SampleCount = 32;
static constexpr uint8_t SamplesPerMs = 25;

// Task scheduler.
TS::Scheduler SchedulerBase{};

// Platform-specific sampling timer. Adjust SamplesPerMs and SamplingTimerChoice as needed for your platform and desired sampling rate.
#if defined(ARDUINO_ARCH_AVR)
static constexpr uint8_t SamplesPerMs = 5;
static constexpr auto SamplingTimerChoice = Inertia::Drivers::TaskProfiler::SamplingTimers::AvrTimerEnum::Timer1ChannelA; // Use AVR Timer 1 Channel A for sampling.
Inertia::Drivers::TaskProfiler::SamplingTimers::AvrSamplingTimer<SamplingTimerChoice> SamplingTimer{}; // Use internal AVR Timer 1 for sampling.
#elif defined(ARDUINO_ARCH_RP2040)
static constexpr uint8_t SamplesPerMs = 100;
Inertia::Drivers::TaskProfiler::SamplingTimers::RpPicoSamplingTimer SamplingTimer{}; // Use internal RP2040 timer for sampling.
#elif defined(ARDUINO_ARCH_STM32)
static constexpr uint8_t SamplesPerMs = 100;
Inertia::Drivers::TaskProfiler::SamplingTimers::StmSamplingTimer SamplingTimer{}; // Use internal STM32 timer for sampling.
#else 
#error "No sampling timer implementation available for this platform."
#endif

// Profiler output to Serial in a simple text format for debugging/analysis.
Inertia::Components::TaskProfiler::MeasurePrintOutput MeasurePrintOutput(Serial); // Output profiler data to Serial for debugging/analysis.

// Sampling profiler task that collects samples and outputs them to the IMeasureOutput.
Inertia::Components::TaskProfiler::SamplingProfiler<SampleCount, SamplesPerMs> SamplingProfilerTask(SchedulerBase, SamplingTimer, &MeasurePrintOutput);

TemplateTask<20, 5000> LongTask(SchedulerBase);
TemplateTask<20, 1000> ShortTask1(SchedulerBase);
TemplateTask<20, 1000> ShortTask2(SchedulerBase);


void setup()
{
	Serial.begin(SERIAL_BAUD_RATE);
	while (!Serial)
		;
	delay(1000);

	LongTask.Start();
	ShortTask1.Start();
	ShortTask2.Start();

	Serial.println();
	Serial.println();
	Serial.println(F("Task Profiler Example starting..."));
	Serial.flush();

	TagTasks();
	SamplingProfilerTask.SetStartDelay(StartDelay);
	SamplingProfilerTask.Start();
}

void loop()
{
	SchedulerBase.execute();
}

void TagTasks()
{
	LongTask.setId(uint8_t(TaskTagEnum::LongTask));
	ShortTask1.setId(uint8_t(TaskTagEnum::ShortTask1));
	ShortTask2.setId(uint8_t(TaskTagEnum::ShortTask2));

	Serial.println(F("Task tags:"));
	for (uint8_t i = 0; i < uint8_t(TaskTagEnum::EnumCount); i++)
	{
		Serial.print(uint8_t(i));
		Serial.print(F(" = "));
		Serial.println(GetTaskTagName(TaskTagEnum(i)));
	}

	Serial.println();
}

