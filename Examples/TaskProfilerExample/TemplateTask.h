#ifndef _TEMPLATE_TASK_h
#define _TEMPLATE_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

template<uint32_t TaskIntervalMillis,
	uint32_t BusyDurationMicros>
class TemplateTask : public TS::Task
{
public:
	TemplateTask(TS::Scheduler& scheduler) :
		TS::Task(TaskIntervalMillis, TASK_FOREVER, &scheduler, false)
	{}

	void Start()
	{
		TS::Task::enable();
		TS::Task::forceNextIteration();
	}

	void Stop()
	{
		TS::Task::disable();
	}

	virtual bool Callback() override
	{
		// Simulate some work by delaying for the specified busy duration.
		delayMicroseconds(BusyDurationMicros);

		return true;
	}
};


#endif

