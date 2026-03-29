/*
* UART Ping Pong Test
* This sketch is meant to test simple UART communication using a ping-pong mechanism.
* It periodically sends a "PING" message over the UART interface and waits for a "PONG" response.
* The test is designed to validate the functionality of the UART module and ensure that data can be transmitted and received correctly.
*/

//#define SERIAL_LOG // Enable serial logging.
#define SERIAL_BAUD_RATE 115200


#define _TASK_OO_CALLBACKS
#include <TScheduler.hpp>

#include <InertiaDrivers.h>
#include "VG6328APing.h"

using UartDriverType = Inertia::Drivers::Uart::VG6328ASerial<HardwareSerial>;


// Process scheduler.
TS::Scheduler SchedulerBase{};


// UART driver on hardware serial.
UartDriverType UartDriver(Serial);

// Hardware validation test task for the UART module.
PingPongTask PingPong(SchedulerBase, UartDriver);


void halt()
{
#if defined(SERIAL_LOG)
	Serial.println(F("Setup Failed."));
#endif
	while (true)
		;
}

void setup()
{
	// Always start serial to enable loopback to display.
	Serial.begin(SERIAL_BAUD_RATE);

#if defined(SERIAL_LOG)
	while (!Serial)
		;
	delay(1000);
	Serial.println(F("UART PingPong setup..."));
#endif


#if defined(SERIAL_LOG)
	Serial.println(F("UART PingPong Start."));
#endif

	// Initialize uart driver serial communication.
	if (!UartDriver.Start("XGV2"))
	{
#if defined(SERIAL_LOG)
		Serial.println(F("UART driver start failed."));
#endif
		halt();
	}

	// Start test ping pong task.
	PingPong.Start();
}

void loop()
{
	if (Serial.available() > 0)
	{
		PingPong.OnSerialAvailable();
	}

	SchedulerBase.execute();
}

#if defined(USE_DISPLAY)
void BufferTaskCallback(void* parameter)
{
	DisplayEngine.BufferTaskCallback(parameter);
}
#endif