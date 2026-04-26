#ifndef _TASK_TAG_h
#define _TASK_TAG_h

enum class TaskTagEnum : uint8_t
{
	Idle,// 0 is reserved for idle.
	LongTask,
	ShortTask1,
	ShortTask2,

	EnumCount
};

const char* GetTaskTagName(const TaskTagEnum tag)
{
	switch (tag)
	{
	case TaskTagEnum::Idle: return "Idle";
	case TaskTagEnum::LongTask: return "LongTask";
	case TaskTagEnum::ShortTask1: return "ShortTask1";
	case TaskTagEnum::ShortTask2: return "ShortTask2";

	default: return "Unknown";
	}
}

#endif