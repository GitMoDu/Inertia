#ifndef _INERTIA_ASSEMBLIES_FLIGHT_CONTROLLER_QUAD_ALTITUDE_HOLD_CONTROLLER_h
#define _INERTIA_ASSEMBLIES_FLIGHT_CONTROLLER_QUAD_ALTITUDE_HOLD_CONTROLLER_h

#include "QuadController.h"

namespace Inertia
{
	namespace Assemblies
	{
		namespace FlightController
		{
			class QuadAltitudeHoldController
				: public QuadController
				, public Inertia::Components::DataSource::IDataSource<Inertia::Components::PowerTrain::altitude_effort_t>
			{
			private:
				using altitude_effort_t = Inertia::Components::PowerTrain::altitude_effort_t;
				using range_t = Inertia::Model::range16_t;

			private:
				Inertia::Model::IDataSource<Inertia::Model::timestamped_quality_range16_t>& AltitudeSource;

			private:
				////TODO: Move default gains to external configuration.
				Inertia::Components::Control::Pid::pid_gain_t AltitudeGain = Inertia::Components::Control::Pid::GetPidGains(
					Inertia::Components::Control::Pid::GetPidGain(250, 1000),
					Inertia::Components::Control::Pid::GetPidGain(0, 1000),
					Inertia::Components::Control::Pid::GetPidGain(0, 1000));

				range_t AltitudeTarget = 0;
				actuate_t AltitudePowerTarget = 0;
				actuate_t AltitudeHoldPowerTarget = Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE / 2; // Default to mid-range power for altitude hold.
				altitude_effort_t AltitudeHoldEffort{};
				bool HasEffort = false;
				bool AltitudeHoldEnabled = false;

			public:
				QuadAltitudeHoldController(Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_euler_angle_t>& orientationDataSource,
					Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>& angularVelocityDataSource,
					Inertia::Components::PowerTrain::IActuatorDriver& actuatorDriver,
					Inertia::Model::IDataSource<Inertia::Model::timestamped_quality_range16_t>& altitudeSource
				)
					: QuadController(orientationDataSource, angularVelocityDataSource, actuatorDriver)
					, AltitudeSource(altitudeSource)
				{}

				void SetAltitudeGain(const Inertia::Components::Control::Pid::pid_gain_t& gain)
				{
					AltitudeGain = gain;
				}

				Inertia::Components::Control::Pid::pid_gain_t GetAltitudeGain() const
				{
					return AltitudeGain;
				}

				void SetAltitudeHoldPowerTarget(const actuate_t powerTarget)
				{
					AltitudeHoldPowerTarget = powerTarget;
				}

				actuate_t GetAltitudeHoldPowerTarget() const
				{
					return AltitudeHoldPowerTarget;
				}

				void SetAltitudeHoldEnabled(const bool enabled)
				{
					AltitudeHoldEnabled = enabled;
				}

				bool IsAltitudeHoldEnabled() const
				{
					return AltitudeHoldEnabled;
				}

				virtual bool Start() override
				{
					AltitudePowerTarget = 0;
					HasEffort = false;

					return QuadController::Start();
				}

				virtual void Stop() override
				{
					AltitudePowerTarget = 0;
					HasEffort = false;
					QuadController::Stop();
				}

				/// <summary>
				/// Override the Step function to include altitude hold control logic. This will read the current altitude from the AltitudeSource,
				/// calculate the altitude error, and adjust the altitude power target accordingly.
				/// </summary>
				virtual void Step() override
				{
					Inertia::Model::timestamped_quality_range16_t altitudeReading{};
					if (AltitudeSource.GetData(altitudeReading))
					{
						const auto altitudeError = static_cast<int32_t>(AltitudeTarget) - static_cast<int32_t>(altitudeReading.distance);

						const actuate_intermedia_t actuateDelta = +IntegerSignal::FixedPoint::FactorScale::Scale<int32_t>(AltitudeGain.Kp, altitudeError);

						AltitudePowerTarget = static_cast<actuate_t>(IntegerSignal::LimitValue<int32_t, 0, Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE>(
							static_cast<actuate_intermedia_t>(AltitudeHoldPowerTarget) + actuateDelta
						));

						// Store the altitude hold effort as a fraction of the maximum actuation for telemetry purposes.
						AltitudeHoldEffort.value = static_cast<actuate_t>(IntegerSignal::AbsValue(actuateDelta)) >> 8;
						HasEffort = true;
					}
					else
					{
						HasEffort = false;
					}

					// Base class Step will handle the stabilization efforts based on orientation and angular velocity.
					QuadController::Step();
				}

				virtual void SetQuadCommand(const Inertia::Model::euler_angle_t& orientationTarget,
					const Inertia::Model::vector32_t& angularVelocityTarget,
					const actuate_t powerTarget,
					const bool enabled) override
				{
					if (AltitudeHoldEnabled)
					{
						QuadController::SetQuadCommand(orientationTarget, angularVelocityTarget, AltitudePowerTarget, enabled);
					}
					else
					{
						QuadController::SetQuadCommand(orientationTarget, angularVelocityTarget, powerTarget, enabled);
					}
				}

				void SetAltitudeTarget(const range_t altitudeTarget)
				{
					AltitudeTarget = altitudeTarget;
				}

				virtual bool GetData(altitude_effort_t& data) override
				{
					if (HasEffort)
					{
						data = AltitudeHoldEffort;
						return true;
					}
					return false;
				}
			};
		}
	}
}

#endif