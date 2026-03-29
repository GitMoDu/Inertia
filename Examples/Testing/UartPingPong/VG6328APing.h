#ifndef _VG6328A_PING_H
#define _VG6328A_PING_H

#include <Arduino.h>

/// @brief Periodic ping task for the VG6328A BLE module.
/// Enters command mode, verifies response, and returns to data mode.
/// Logs connectivity status to a Print output.
/// @tparam SerialType VG6328ADriver instantiation type.
/// @tparam PingIntervalMillis Delay between pings.
template<typename SerialType>
class PingPongTask : private TS::Task
{
private:
	static constexpr uint8_t PollPeriod = 50;
	static constexpr uint32_t PingIntervalMillis = 1000;

private:
	SerialType& SerialInstance;
	Print* Log;

private:
	uint32_t LastPing = 0;

	volatile bool PongPending = false;

public:
	PingPongTask(TS::Scheduler& scheduler, SerialType& serial)
		: TS::Task(PingIntervalMillis, TASK_FOREVER, &scheduler, false)
		, SerialInstance(serial)
		, Log(nullptr)
	{}

	PingPongTask(TS::Scheduler& scheduler, SerialType& serial, Print& log)
		: TS::Task(PingIntervalMillis, TASK_FOREVER, &scheduler, false)
		, SerialInstance(serial)
		, Log(&log)
	{}

	void Start()
	{
		TS::Task::enableDelayed(PingIntervalMillis);
		PongPending = false;
	}

	void Stop()
	{
		TS::Task::disable();
	}

	void OnSerialAvailable()
	{
		if (!PongPending)
		{
			PongPending = true;
			TS::Task::enableDelayed(0);
		}
	}

private:
	bool Callback() final
	{
		if (PongPending)
		{
			PongPending = false;
			if (Log != nullptr)
				Log->print(F("RX: "));
			while (SerialInstance.available())
			{
				if (Log != nullptr)
					Log->write(SerialInstance.read());
				else
					SerialInstance.read();
			}
			if (Log != nullptr)
				Log->println();

			SerialInstance.write("PONG");
			SerialInstance.flush();
			if (Log != nullptr)
				Log->println(F("TX: PONG"));
			TS::Task::enableDelayed(0);
		}
		else
		{
			const uint32_t timestamp = millis();
			const uint32_t elapsed = timestamp - LastPing;
			if (SerialInstance.availableForWrite()
				&& (elapsed > PingIntervalMillis))
			{
				LastPing = timestamp;
				SerialInstance.write("PING");
				SerialInstance.flush();
				if (Log != nullptr)
					Log->println(F("TX: PING"));

				if (PongPending)
				{
					TS::Task::enableDelayed(0);
				}
				else
				{
					TS::Task::enableDelayed(PingIntervalMillis);
				}
			}
			else
			{
				if (PongPending)
				{
					TS::Task::enableDelayed(0);
				}
				else if (elapsed >= PingIntervalMillis)
				{
					TS::Task::enableDelayed(0);
				}
				else
				{
					TS::Task::enableDelayed(PingIntervalMillis - elapsed);
				}
			}
		}

		return true;
	}
};

#endif