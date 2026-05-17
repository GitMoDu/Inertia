#ifndef _INERTIA_DRIVERS_UART_VG6328A_WATCH_DOG_TASK_h
#define _INERTIA_DRIVERS_UART_VG6328A_WATCH_DOG_TASK_h

#define _TASK_OO_CALLBACKS
#include <TSchedulerDeclarations.hpp>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "Model.h"

#include "../../../Components/Core/DataSource/Model.h"
#include "../../../Components/Core/Lifecycle/Model.h"

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
				template<typename DriverType,
					uint8_t BleConnectedPin>
				class WatchDogTask
					: public Inertia::Components::DataSource::IObserver<Inertia::Components::Link::LinkStateStruct>
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
					Inertia::Components::Link::LinkEnum LastLinkState = Inertia::Components::Link::LinkEnum::NoLink;
					StateEnum CurrentState = StateEnum::Disabled;

				private:
					DriverType& Driver;

				public:
					WatchDogTask(TS::Scheduler& scheduler, DriverType& driver)
						: Inertia::Components::DataSource::IObserver<Inertia::Components::Link::LinkStateStruct>()
						, TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
						, Driver(driver)
					{}

					bool Callback() override
					{
						switch (CurrentState)
						{
						case StateEnum::Checking:
							// Only runs after DisconnectedTimeout.
							if (LastLinkState == Inertia::Components::Link::LinkEnum::NoLink
								&& IsBleConnected())
							{
								// Attempt to restore the Link by restarting the driver.
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

					bool Setup(Inertia::Model::IObservable<Inertia::Components::Link::LinkStateStruct>& observable)
					{
						pinMode(BleConnectedPin, PIN_INPUT_MODE);

						LastLinkState = Inertia::Components::Link::LinkEnum::NoLink;
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

					virtual void OnDataUpdate(const Inertia::Components::Link::LinkStateStruct& linkState) override
					{
						const bool disconnected = LastLinkState == Inertia::Components::Link::LinkEnum::Linked
							&& linkState.State == Inertia::Components::Link::LinkEnum::NoLink;

						LastLinkState = linkState.State;

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
							if (linkState.State == Inertia::Components::Link::LinkEnum::Linked)
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