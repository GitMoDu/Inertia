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
			class QuadController
				: public Inertia::Components::PowerTrain::IQuadDriver
				, public Inertia::Components::Lifecycle::IPeriodicDriver
			{
			protected:
				using actuate_t = ::Inertia::Components::PowerTrain::actuate_t;
				using scale32_t = ::IntegerSignal::FixedPoint::FactorScale::scale32_t;

				using signed_angle_t = IntegerSignal::TypeTraits::TypeSign::make_signed<angle_t>::type;
				using actuate_intermedia_t = IntegerSignal::TypeTraits::TypeNext::next_int_type<actuate_t>::type;
				using fraction_intermediate_t = IntegerSignal::TypeTraits::TypeNext::next_int_type<fraction16_t>::type;

			private:
				static constexpr int8_t FrontLeftPitchPolarity = 1;
				static constexpr int8_t FrontRightPitchPolarity = 1;
				static constexpr int8_t BackLeftPitchPolarity = -1;
				static constexpr int8_t BackRightPitchPolarity = -1;

				static constexpr int8_t FrontLeftRollPolarity = -1;
				static constexpr int8_t FrontRightRollPolarity = 1;
				static constexpr int8_t BackLeftRollPolarity = -1;
				static constexpr int8_t BackRightRollPolarity = 1;

				static constexpr int8_t FrontLeftYawPolarity = -1;
				static constexpr int8_t FrontRightYawPolarity = 1;
				static constexpr int8_t BackLeftYawPolarity = 1;
				static constexpr int8_t BackRightYawPolarity = -1;

			private:
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_euler_angle_t>& OrientationDataSource;
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>& AngularVelocityDataSource;
				Inertia::Components::PowerTrain::IActuatorDriver& ActuatorDriver;

			public:
				////TODO: Move default gains to external configuration.
				Inertia::Components::PowerTrain::quad_pid_gains_t QuadGains
				{
					Inertia::Components::Control::Pid::euler_pid_gain_t{
						Inertia::Components::Control::Pid::GetPidGains(
						Inertia::Components::Control::Pid::GetPidGain(1500, 1000),
						Inertia::Components::Control::Pid::GetPidGain(1000, 1000),
						Inertia::Components::Control::Pid::GetPidGain(500, 1000))
					},
					Inertia::Components::Control::Pid::euler_pid_gain_t{
						Inertia::Components::Control::Pid::GetPidGains(
						Inertia::Components::Control::Pid::GetPidGain(200, 1000),
						Inertia::Components::Control::Pid::GetPidGain(0, 1000),
						Inertia::Components::Control::Pid::GetPidGain(0, 1000))
					}
				};

			private:
				uint8_t FrontLeftActuatorIndex = 0;
				uint8_t FrontRightActuatorIndex = 1;
				uint8_t BackLeftActuatorIndex = 2;
				uint8_t BackRightActuatorIndex = 3;


			private:
				Inertia::Components::PowerTrain::quad_stabilization_effort_t StabilizationEffort{};
				Inertia::Components::PowerTrain::quad_power_effort_t PowerEffort{};
				Inertia::Components::PowerTrain::quad_pitch_effort_t PitchEffort{};
				Inertia::Components::PowerTrain::quad_roll_effort_t RollEffort{};
				Inertia::Components::PowerTrain::quad_yaw_effort_t YawEffort{};

				Inertia::Model::euler_angle_t OrientationTarget{};
				Inertia::Model::vector32_t AngularVelocityTarget{};
				actuate_t PowerTarget = 0;
				bool Enabled = true;

				bool HasEfforts = false;

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

				void SetActuatorIndices(const uint8_t frontLeftIndex, const uint8_t frontRightIndex, const uint8_t backLeftIndex, const uint8_t backRightIndex)
				{
					FrontLeftActuatorIndex = frontLeftIndex;
					FrontRightActuatorIndex = frontRightIndex;
					BackLeftActuatorIndex = backLeftIndex;
					BackRightActuatorIndex = backRightIndex;
				}

				virtual void SetQuadCommand(const Inertia::Model::euler_angle_t& orientationTarget,
					const Inertia::Model::vector32_t& angularVelocityTarget,
					const actuate_t powerTarget,
					const bool enabled) override
				{
					OrientationTarget = orientationTarget;
					AngularVelocityTarget = angularVelocityTarget;
					PowerTarget = powerTarget;
					Enabled = enabled;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_pitch_effort_t& data) override
				{
					if (HasEfforts)
					{
						data = PitchEffort;
						return true;
					}
					return false;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_power_effort_t& data) override
				{
					if (HasEfforts)
					{
						data = PowerEffort;
						return true;
					}
					return false;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_roll_effort_t& data) override
				{
					if (HasEfforts)
					{
						data = RollEffort;
						return true;
					}
					return false;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_yaw_effort_t& data) override
				{
					if (HasEfforts)
					{
						data = YawEffort;
						return true;
					}
					return false;
				}

				virtual bool GetData(Inertia::Components::PowerTrain::quad_stabilization_effort_t& data) override
				{
					if (HasEfforts)
					{
						data = StabilizationEffort;
						return true;
					}
					return false;
				}

				virtual bool Start() override
				{
					HasEfforts = false;

					Enabled = false;

					return true;
				}

				virtual void Stop() override
				{
					HasEfforts = false;
					Enabled = false;
				}

				virtual void Step() override
				{
					Inertia::Model::timestamped_euler_angle_t orientation{};
					Inertia::Model::timestamped_angular_velocity_t angularVelocity{};
					if (OrientationDataSource.GetData(orientation)
						&& AngularVelocityDataSource.GetData(angularVelocity)
						&& Enabled)
					{
						const int16_t pitchError = FoldError180(static_cast<int32_t>(orientation.pitch) - static_cast<int32_t>(OrientationTarget.pitch));
						const int16_t rollError = FoldError180(static_cast<int32_t>(orientation.roll) - static_cast<int32_t>(OrientationTarget.roll));
						const int16_t yawError = FoldError180(static_cast<int32_t>(orientation.yaw) - static_cast<int32_t>(OrientationTarget.yaw));
						const int32_t pitchRateError = static_cast<int32_t>(angularVelocity.x) - AngularVelocityTarget.x;
						const int32_t rollRateError = static_cast<int32_t>(angularVelocity.y) - AngularVelocityTarget.y;
						const int32_t yawRateError = static_cast<int32_t>(angularVelocity.z) - AngularVelocityTarget.z;
						const actuate_t powerTarget = PowerTarget;

						const int32_t pitchContribution = IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.OrientationGains.Pitch.Kp, pitchError)
							+ IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.AngularVelocityGains.Pitch.Kd, pitchRateError);
						const int32_t rollContribution = IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.OrientationGains.Roll.Kp, rollError)
							+ IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.AngularVelocityGains.Roll.Kd, rollRateError);
						const int32_t yawContribution = IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.OrientationGains.Yaw.Kp, yawError)
							+ IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(QuadGains.AngularVelocityGains.Yaw.Kd, yawRateError);

						const fraction16_t deltaFl = MixDelta(FrontLeftPitchPolarity, FrontLeftRollPolarity, FrontLeftYawPolarity,
							pitchContribution, rollContribution, yawContribution);
						const fraction16_t deltaFr = MixDelta(FrontRightPitchPolarity, FrontRightRollPolarity, FrontRightYawPolarity,
							pitchContribution, rollContribution, yawContribution);
						const fraction16_t deltaBl = MixDelta(BackLeftPitchPolarity, BackLeftRollPolarity, BackLeftYawPolarity,
							pitchContribution, rollContribution, yawContribution);
						const fraction16_t deltaBr = MixDelta(BackRightPitchPolarity, BackRightRollPolarity, BackRightYawPolarity,
							pitchContribution, rollContribution, yawContribution);

						const fraction16_t deltaMax = DeltaMax(deltaFl, deltaFr, deltaBl, deltaBr);

						{
							const fraction16_t deltaFlNormalized = NormalizeToDownscale(deltaFl, deltaMax);
							const actuate_t actuationValue = MixActuation(powerTarget, deltaFlNormalized);
							ActuatorDriver.SetActuator(
								FrontLeftActuatorIndex,
								actuationValue,
								Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							PowerEffort.Fl = static_cast<uint8_t>(actuationValue >> 8); // Convert 16-bit actuation value to 8-bit effort.
						}

						{
							const fraction16_t deltaFrNormalized = NormalizeToDownscale(deltaFr, deltaMax);
							const actuate_t actuationValue = MixActuation(powerTarget, deltaFrNormalized);
							ActuatorDriver.SetActuator(
								FrontRightActuatorIndex,
								actuationValue,
								Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							PowerEffort.Fr = static_cast<uint8_t>(actuationValue >> 8); // Convert 16-bit actuation value to 8-bit effort.
						}

						{
							const fraction16_t deltaBlNormalized = NormalizeToDownscale(deltaBl, deltaMax);
							const actuate_t actuationValue = MixActuation(powerTarget, deltaBlNormalized);
							ActuatorDriver.SetActuator(
								BackLeftActuatorIndex,
								actuationValue,
								Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							PowerEffort.Bl = static_cast<uint8_t>(actuationValue >> 8); // Convert 16-bit actuation value to 8-bit effort.
						}

						{
							const fraction16_t deltaBrNormalized = NormalizeToDownscale(deltaBr, deltaMax);
							const actuate_t actuationValue = MixActuation(powerTarget, deltaBrNormalized);
							ActuatorDriver.SetActuator(
								BackRightActuatorIndex,
								actuationValue,
								Inertia::Components::PowerTrain::ActuationModeEnum::Forward);
							PowerEffort.Br = static_cast<uint8_t>(actuationValue >> 8); // Convert 16-bit actuation value to 8-bit effort.
						}


						// For stabilization effort output, we can use the maximum absolute delta as a representation of the overall stabilization effort required.
						const fraction16_t deltaMin = DeltaMin(deltaFl, deltaFr, deltaBl, deltaBr);
						const fraction16_t deltaRange = IntegerSignal::LimitValue<fraction_intermediate_t, 0, FRACTION16_1X>(static_cast<fraction_intermediate_t>(deltaMax) - deltaMin);
						StabilizationEffort.value = Fraction<uint8_t>(deltaRange, UINT8_MAX);

						// For pitch, roll and yaw effort outputs, we can use the absolute contributions normalized to the maximum possible contribution to represent the effort required for each axis.
						const fraction16_t pitchEffortValue = IntegerSignal::LimitValue<fraction_intermediate_t, 0, FRACTION16_1X>(IntegerSignal::AbsValue(pitchContribution));
						PitchEffort.value = Fraction<uint8_t>(pitchEffortValue, UINT8_MAX);
						const fraction16_t rollEffortValue = IntegerSignal::LimitValue<fraction_intermediate_t, 0, FRACTION16_1X>(IntegerSignal::AbsValue(rollContribution));
						RollEffort.value = Fraction<uint8_t>(rollEffortValue, UINT8_MAX);
						const fraction16_t yawEffortValue = IntegerSignal::LimitValue<fraction_intermediate_t, 0, FRACTION16_1X>(IntegerSignal::AbsValue(yawContribution));
						YawEffort.value = Fraction<uint8_t>(yawEffortValue, UINT8_MAX);

						HasEfforts = true;
					}
					else
					{
						StopOutput();
					}
				}


			private:
				static constexpr actuate_t MixActuation(const actuate_t power, const fraction16_t delta)
				{
					return static_cast<actuate_t>(IntegerSignal::LimitValue<actuate_intermedia_t, 0, Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE>(
						static_cast<actuate_intermedia_t>(power)
						+ Fraction<actuate_intermedia_t>(delta, power)));
				}

				static constexpr fraction16_t DeltaMax(const fraction16_t deltaFl, const fraction16_t& deltaFr, const fraction16_t deltaBl, const fraction16_t deltaBr)
				{
					return IntegerSignal::MaxValue<fraction16_t>(deltaFl, IntegerSignal::MaxValue<fraction16_t>(deltaFr, IntegerSignal::MaxValue<fraction16_t>(deltaBl, deltaBr)));
				}

				static constexpr fraction16_t DeltaMin(const fraction16_t deltaFl, const fraction16_t& deltaFr, const fraction16_t deltaBl, const fraction16_t deltaBr)
				{
					return IntegerSignal::MinValue<fraction16_t>(deltaFl, IntegerSignal::MinValue<fraction16_t>(deltaFr, IntegerSignal::MinValue<fraction16_t>(deltaBl, deltaBr)));
				}

				static constexpr fraction16_t NormalizeToDownscale(const fraction16_t delta, const fraction16_t deltaMax)
				{
					return static_cast<fraction16_t>(
						IntegerSignal::LimitValue<int32_t, Fraction16::SCALAR_UNIT_NEGATIVE, 0>(
							static_cast<int32_t>(delta)
							- static_cast<int32_t>(deltaMax)));
				}

				static constexpr signed_angle_t FoldError180(const int32_t error)
				{
					return static_cast<signed_angle_t>(static_cast<uint16_t>(error));
				}

				static constexpr fraction16_t MixDelta(const int8_t pitchPolarity,
					const int8_t rollPolarity,
					const int8_t yawPolarity,
					const int32_t pitchContribution,
					const int32_t rollContribution,
					const int32_t yawContribution)
				{
					return static_cast<fraction16_t>(
						IntegerSignal::LimitValue<int32_t, Fraction16::SCALAR_UNIT_NEGATIVE, Fraction16::SCALAR_UNIT>(
							(int32_t(pitchPolarity) * pitchContribution)
							+ (int32_t(rollPolarity) * rollContribution)
							+ (int32_t(yawPolarity) * yawContribution)));
				}


				void StopOutput()
				{
					HasEfforts = false;

					ActuatorDriver.SetActuator(FrontLeftActuatorIndex, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
					ActuatorDriver.SetActuator(FrontRightActuatorIndex, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
					ActuatorDriver.SetActuator(BackLeftActuatorIndex, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
					ActuatorDriver.SetActuator(BackRightActuatorIndex, 0, Inertia::Components::PowerTrain::ActuationModeEnum::Disabled);
				}
			};
		}
	}
}

#endif