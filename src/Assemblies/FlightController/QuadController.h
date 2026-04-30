#ifndef _INERTIA_ASSEMBLIES_FLIGHT_CONTROLLER_QUAD_CONTROLLER_h
#define _INERTIA_ASSEMBLIES_FLIGHT_CONTROLLER_QUAD_CONTROLLER_h

#include "../../Components/Core/Lifecycle/Model.h"
#include "../../Components/Core/DataSource/Model.h"
#include "../../Components/PowerTrain/Model.h"
#include "../../Components/Control/Pid/Model.h"

namespace Inertia
{
	namespace Assemblies
	{
		namespace FlightController
		{
			using namespace ::IntegerSignal::FixedPoint::ScalarFraction;
			using namespace ::IntegerSignal::FixedPoint::FactorScale;
			using ::IntegerSignal::AbsValue;
			using ::IntegerSignal::LimitValue;
			using ::IntegerSignal::MaxValue;
			using ::IntegerSignal::MinValue;

			using ::Inertia::Components::PowerTrain::actuate_t;

			enum class ActuatorQuadEnum : uint8_t
			{
				FrontLeft,
				FrontRight,
				BackLeft,
				BackRight,
				EnumCount
			};

			class QuadController
				: public Inertia::Components::PowerTrain::IQuadDriver
				, public Inertia::Components::Lifecycle::IPeriodicDriver
			{
			private:
				using signed_actuate_t = IntegerSignal::TypeTraits::TypeNext::next_int_type<actuate_t>::type;

			private:
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_euler_angle_t>& OrientationDataSource;
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>& AngularVelocityDataSource;

				Inertia::Components::PowerTrain::IActuatorDriver& ActuatorDriver;

			private:
				static constexpr int8_t FrontLeftPitchPolarity = 1;
				static constexpr int8_t FrontLeftRollPolarity = -1;
				static constexpr int8_t FrontLeftYawPolarity = 1;
				static constexpr int8_t FrontRightPitchPolarity = 1;
				static constexpr int8_t FrontRightRollPolarity = 1;
				static constexpr int8_t FrontRightYawPolarity = -1;
				static constexpr int8_t BackLeftPitchPolarity = -1;
				static constexpr int8_t BackLeftRollPolarity = -1;
				static constexpr int8_t BackLeftYawPolarity = -1;
				static constexpr int8_t BackRightPitchPolarity = -1;
				static constexpr int8_t BackRightRollPolarity = 1;
				static constexpr int8_t BackRightYawPolarity = 1;


			public:

				struct QuadGainsStruct
				{
					Inertia::Components::Control::Pid::pid_gain_t Pitch = Inertia::Components::Control::Pid::PidGainUnity;
					Inertia::Components::Control::Pid::pid_gain_t Roll = Inertia::Components::Control::Pid::PidGainUnity;
					Inertia::Components::Control::Pid::pid_gain_t Yaw = Inertia::Components::Control::Pid::PidGainUnity;
				};

				// Configurable PID gains for pitch and roll, and P gain for yaw velocity, represented as scale factors of the respective error values.
				scale32_t Kp = Scale32::GetFactor<int16_t>(1000, 1000);
				scale32_t Ki = Scale32::GetFactor<int16_t>(5, 1000);
				scale32_t Kd = Scale32::GetFactor<int16_t>(10, 1000);

				// Configurable P gain for yaw velocity, represented as a scale factor of the yaw velocity error value.
				scale32_t YawP = Scale32::GetFactor<int16_t>(500, 1000);

			private:
				Inertia::Components::PowerTrain::QuadCommandStruct Command{};

				Inertia::Components::PowerTrain::quad_stabilization_effort_t QuadEffort{};
				Inertia::Components::PowerTrain::power_effort_t PowerEffort{};

				bool HasQuadEffort = false;
				bool HasPowerEffort = false;

			public:
				QuadController(Inertia::Model::IDataSource<Inertia::Model::timestamped_euler_angle_t>& orientationDataSource,
					Inertia::Model::IDataSource<Inertia::Model::timestamped_angular_velocity_t>& angularVelocityDataSource,
					Inertia::Components::PowerTrain::IActuatorDriver& actuatorDriver)
					: Inertia::Components::PowerTrain::IQuadDriver()
					, Inertia::Components::Lifecycle::IPeriodicDriver()
					, OrientationDataSource(orientationDataSource)
					, AngularVelocityDataSource(angularVelocityDataSource)
					, ActuatorDriver(actuatorDriver)
				{}

				virtual void SetQuadCommand(const Inertia::Components::PowerTrain::QuadCommandStruct& command) override
				{
					memcpy(&Command, &command, sizeof(Inertia::Components::PowerTrain::QuadCommandStruct));
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_stabilization_effort_t& data) override
				{
					if (HasQuadEffort)
					{
						data = QuadEffort;
						return true;
					}
					return false;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::power_effort_t& data) override
				{
					if (HasPowerEffort)
					{
						data = PowerEffort;
						return true;
					}
					return false;
				}

				virtual bool Start() override
				{
					HasQuadEffort = false;
					HasPowerEffort = false;
					Command.Enabled = false;

					return true;
				}

				virtual void Stop() override
				{
					HasQuadEffort = false;
					HasPowerEffort = false;
					Command.Enabled = false;
				}

				virtual void Step() override
				{
					Inertia::Model::timestamped_euler_angle_t orientation;
					Inertia::Model::timestamped_angular_velocity_t angularVelocity;
					if (OrientationDataSource.GetData(orientation)
						&& AngularVelocityDataSource.GetData(angularVelocity))
					{
						if (Command.Enabled)
						{
							// Calculate the error between the current orientation and the target orientation for pitch and roll, and the error between the current angular velocity and the target angular velocity for yaw.{
							const int16_t pitchError =
								FoldError180(static_cast<int32_t>(Command.OrientationTarget.pitch) - static_cast<int32_t>(orientation.pitch));

							const int16_t rollError =
								FoldError180(static_cast<int32_t>(Command.OrientationTarget.roll) - static_cast<int32_t>(orientation.roll));

							const int32_t yawVelocityError = (static_cast<int32_t>(Command.AngularVelocityTarget.y) - static_cast<int32_t>(angularVelocity.y));

							int16_t maxDelta{};
							if (AbsValue(pitchError) >= AbsValue(rollError))
							{
								maxDelta = AbsValue(pitchError);
							}
							else
							{
								maxDelta = AbsValue(rollError);
							}
							if (AbsValue(yawVelocityError) > AbsValue(pitchError) && AbsValue(yawVelocityError) > AbsValue(rollError))
							{
								maxDelta = LimitValue<int32_t, INT16_MIN, INT16_MAX>(AbsValue(yawVelocityError));
							}

							// Use the maximum of the pitch and roll deltas to determine the quad stabilization effort level.
							QuadEffort.value = LimitValue<int32_t, 0, INT16_MAX>(maxDelta) / (INT16_MAX / 255);
							if (!HasQuadEffort)
								HasQuadEffort = true;

							// Use the maximum of the pitch and roll deltas to determine the power demand magnitude, which can be used to scale the power target up to the required configurable level.
							//const actuate_t demandMagnitude = Fraction16::Fraction(maxDelta, Inertia::Model::MAX_ACTUATE_VALUE);

							// Ensure that the power demand is at least as high as demand magnitude and power target, to ensure that there is sufficient power to achieve the required stabilization effort level.
							//const actuate_t power = MaxValue(demandMagnitude, PowerTarget);//TODO: Enable after testing with fixed power target.
							const actuate_t power = Command.PowerTarget;

							// Use the effective power level to determine the power effort level.
							PowerEffort.value = LimitValue<actuate_t, 0, Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE>(power);
							if (!HasPowerEffort)
								HasPowerEffort = true;

							// Scale the pitch, roll, and yaw velocity errors by their respective PID gains to calculate the contribution of each error to the overall control effort.
							const int32_t pitchContribution = Scale<int32_t>(Kp, pitchError);
							const int32_t rollContribution = Scale<int32_t>(Kp, rollError);
							const int32_t yawContribution = Scale<int32_t>(YawP, yawVelocityError);

							// Mix the contributions of the pitch, roll, and yaw velocity errors according to the configured polarities.
							// Resulting value should fit within the fraction16_t range, by adjusting the PID gains as needed.
							const fraction16_t deltaFl = MixDelta(
								FrontLeftPitchPolarity,
								FrontLeftRollPolarity,
								FrontLeftYawPolarity,
								pitchContribution,
								rollContribution,
								yawContribution);
							const fraction16_t deltaFr = MixDelta(
								FrontRightPitchPolarity,
								FrontRightRollPolarity,
								FrontRightYawPolarity,
								pitchContribution,
								rollContribution,
								yawContribution);
							const fraction16_t deltaBl = MixDelta(
								BackLeftPitchPolarity,
								BackLeftRollPolarity,
								BackLeftYawPolarity,
								pitchContribution,
								rollContribution,
								yawContribution);
							const fraction16_t deltaBr = MixDelta(
								BackRightPitchPolarity,
								BackRightRollPolarity,
								BackRightYawPolarity,
								pitchContribution,
								rollContribution,
								yawContribution);

							// Calculate the final actuation value for each motor by adding the scaled delta to the base power level.
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::FrontLeft, MixActuation(power, deltaFl), Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::FrontRight, MixActuation(power, deltaFr), Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::BackLeft, MixActuation(power, deltaBl), Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::BackRight, MixActuation(power, deltaBr), Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
						}
						else
						{
							PowerEffort.value = 0;
							if (!HasPowerEffort)
								HasPowerEffort = true;
							QuadEffort.value = 0;
							if (!HasQuadEffort)
								HasQuadEffort = true;

							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::FrontLeft, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::FrontRight, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::BackLeft, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
							ActuatorDriver.SetActuator((uint8_t)ActuatorQuadEnum::BackRight, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
						}
					}
				}

			private:
				static constexpr fraction16_t MixDelta(const int8_t pitchPolarity,
					const int8_t rollPolarity,
					const int8_t yawPolarity,
					const int32_t pitchContribution,
					const int32_t rollContribution,
					const int32_t yawContribution)
				{
					return static_cast<fraction16_t>(LimitValue<int32_t,
						Fraction16::SCALAR_UNIT_NEGATIVE,
						Fraction16::SCALAR_UNIT>(
							(int32_t(pitchPolarity) * pitchContribution)
							+ (int32_t(rollPolarity) * rollContribution)
							+ (int32_t(yawPolarity) * yawContribution)));
				}

				static constexpr actuate_t MixActuation(const actuate_t power, const fraction16_t delta)
				{
					return static_cast<actuate_t>(LimitValue<signed_actuate_t, 0,
						Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE>(
							static_cast<signed_actuate_t>(power)
							+ Fraction(delta, static_cast<signed_actuate_t>(power))));
				}

				static int16_t FoldError180(const int32_t error)
				{
					if (error > ANGLE_90)
					{
						return error - ANGLE_180;
					}

					if (error < -ANGLE_90)
					{
						return error + ANGLE_180;
					}

					return error;
				}
			};
		}
	}
}

#endif