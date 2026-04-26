#ifndef _INERTIA_DRIVERS_UART_VG6328A_WATCH_DOG_TASK_h
#define _INERTIA_DRIVERS_UART_VG6328A_WATCH_DOG_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "Model.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Uart
		{
			/// <summary>
		 /// Watchdog support for a VG6328A Bluetooth module connection indicator.
			/// </summary>
			namespace VG6328A
			{
				/// <summary>
				/// The VG6328A module can sometimes get stuck, where it stops responding to serial passthrough while still asserting the connection indicator pin.
				/// Provided a protocol connection state source, the watchdog only arms after a real protocol connection has been observed and checks for the indicator pin remaining asserted after disconnect.
				/// </summary>
				/// <typeparam name="BleConnectedPin">The digital pin number used to read the BLE connection indicator.</typeparam>
				template<uint8_t BleConnectedPin>
				class WatchDogTask
					: public Inertia::Model::IObserver<Inertia::Model::ConnectionEnum>
					, public TS::Task
				{
				private:
					enum class StateEnum : uint8_t
					{
						Disabled,
						Ready,
						Idle,
						Checking
					};

				private:
#if defined(ARDUINO_ARCH_NRF52840)
					static constexpr uint8_t PIN_INPUT_MODE = INPUT;
#else
					static constexpr uint8_t PIN_INPUT_MODE = INPUT_PULLUP;
#endif
					static constexpr uint8_t PIN_CONNECTED_STATE = LOW;

				private:
					static constexpr uint32_t DisconnectedTimeout = 1600; // Time in milliseconds to wait after a protocol disconnect before checking for a stuck module.

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0;

				private:
					Inertia::Model::ConnectionEnum LastConnectionState = Inertia::Model::ConnectionEnum::Disconnected;
					StateEnum CurrentState = StateEnum::Disabled;

				private:
					Inertia::Model::ILifecycleDriver& Driver;

				public:
					WatchDogTask(TS::Scheduler& scheduler, Inertia::Model::ILifecycleDriver& driver)
						: TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
						, Driver(driver)
					{}

					bool Callback() override
					{
						switch (CurrentState)
						{
						case StateEnum::Checking:
							// Only runs after DisconnectedTimeout.
							if (LastConnectionState == Inertia::Model::ConnectionEnum::Disconnected
								&& IsBleConnected())
							{
								// Attempt to restore the connection by restarting the driver.
								Driver.Stop();
								Driver.Start();
								OnStuckStateDetected();
							}

							CurrentState = StateEnum::Idle;
							TS::Task::disable();
							break;
						case StateEnum::Idle:
						default:
							TS::Task::disable();
							break;
						}

						return true;
					}

					bool Setup(Inertia::Model::IObservable<Inertia::Model::ConnectionEnum>& observable)
					{
						pinMode(BleConnectedPin, PIN_INPUT_MODE);

						LastConnectionState = Inertia::Model::ConnectionEnum::Disconnected;
						CurrentState = StateEnum::Disabled;

						if (observable.SetObserver(this))
						{
							CurrentState = StateEnum::Ready;
							return true;
						}
						else
						{
							CurrentState = StateEnum::Disabled;
							TS::Task::disable();
							return false;
						}
					}

					void Start()
					{
						if (CurrentState == StateEnum::Ready)
						{
							CurrentState = StateEnum::Idle;
						}
					}

					void Stop()
					{
						switch (CurrentState)
						{
						case StateEnum::Idle:
						case StateEnum::Checking:
							CurrentState = StateEnum::Ready;
							break;
						case StateEnum::Ready:
							break;
						case StateEnum::Disabled:
						default:
							break;
						}

						TS::Task::disable();
					}

					virtual void OnDataUpdate(const Inertia::Model::ConnectionEnum& connectionState) override
					{
						const bool disconnected = LastConnectionState == Inertia::Model::ConnectionEnum::Connected
							&& connectionState == Inertia::Model::ConnectionEnum::Disconnected;

						LastConnectionState = connectionState;

						switch (CurrentState)
						{
						case StateEnum::Idle:
							if (disconnected)
							{
								CurrentState = StateEnum::Checking;
								TS::Task::enableDelayed(DisconnectedTimeout);
							}
							break;
						case StateEnum::Checking:
							if (connectionState == Inertia::Model::ConnectionEnum::Connected)
							{
								CurrentState = StateEnum::Idle;
								TS::Task::disable();
							}
							break;
						case StateEnum::Disabled:
						case StateEnum::Ready:
						default:
							break;
						}
					}

				private:
					bool IsBleConnected() const
					{
						return digitalRead(BleConnectedPin) == PIN_CONNECTED_STATE;
					}

					void OnStuckStateDetected()
					{
						if (LogListener != nullptr)
							LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = InstanceId,
									.Type = Inertia::Model::LogTypeEnum::Error,
									.Code = static_cast<uint8_t>(LogCodeEnum::WatchDogDetectedStuckState),
									.Value = 0 });
					}
				};
			}
		}
	}
}
#endif