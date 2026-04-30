#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATED_DRIVER_h
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATED_DRIVER_h

#include "Model.h"
#include "../../Core/DataSource/Model.h"
#include "../../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace ServoActuator
			{
				using namespace IntegerSignal::FixedPoint::FactorScale;

				template<uint8_t ServoCount>
				class CalibratedDriver : public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Inertia::Components::PowerTrain::IActuatorDriver
				{
				private:
					struct cached_servo_calibration_t
					{
						Inertia::Components::PowerTrain::Servo::servo_calibration_t Calibration;

						scale32_t TopScale = 0; // Precomputed scale factor for top range.
						scale32_t BottomScale = 0; // Precomputed scale factor for bottom range.

						void ComputeScales()
						{
							const int32_t topRange = static_cast<int32_t>(Calibration.top) - static_cast<int32_t>(Calibration.center);
							const int32_t bottomRange = static_cast<int32_t>(Calibration.center) - static_cast<int32_t>(Calibration.bottom);
							TopScale = topRange > 0 ? GetFactor32(topRange, static_cast<int32_t>(INT16_MAX)) : 0; // Scale for positive actuator values.
							BottomScale = bottomRange > 0 ? GetFactor32(bottomRange, static_cast<int32_t>(INT16_MAX)) : 0; // Scale for negative actuator values.
						}
					};

				private:
					Inertia::Components::PowerTrain::Servo::IServoDriver& ServoDriver;
					Inertia::Components::PowerTrain::Servo::IServoCalibration& DefaultCalibration;
					Inertia::Components::PowerTrain::Servo::IServoCalibrationRepository& CalibrationRepository;

				private:
					cached_servo_calibration_t ServoCalibrations[ServoCount]{};

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0;

				public:
					CalibratedDriver(Inertia::Components::PowerTrain::Servo::IServoDriver& servoDriver,
						Inertia::Components::PowerTrain::Servo::IServoCalibration& defaultCalibration,
						Inertia::Components::PowerTrain::Servo::IServoCalibrationRepository& calibrationRepository)
						: Inertia::Components::Lifecycle::ILifecycleDriver()
						, Inertia::Components::PowerTrain::IActuatorDriver()
						, ServoDriver(servoDriver)
						, DefaultCalibration(defaultCalibration)
						, CalibrationRepository(calibrationRepository)
					{}

					~CalibratedDriver() = default;

					bool Start() override
					{
						if (ServoDriver.GetServoCount() != ServoCount)
						{
							// Servo count mismatch between driver and calibrated wrapper, fail start.
							return false;
						}

						LoadCalibrations();

						return true;
					}

					void Stop() override
					{
						// No ongoing operations to stop, so just return.
					}

					uint8_t GetDriveCount() const
					{
						return ServoCount;
					}

					void LoadCalibrations()
					{
						// Load servo calibrations from the repository.
						for (uint8_t i = 0; i < ServoCount; i++)
						{
							if (!CalibrationRepository.GetServoCalibration(i, ServoCalibrations[i].Calibration))
							{
								if (LogListener != nullptr)
									LogListener->OnLog(Inertia::Model::LogEntryStruct{
										.Tag = LOG_TAG,
										.Instance = InstanceId,
										.Type = Inertia::Model::LogTypeEnum::Warning,
										.Code = LogCodeEnum::CalibrationLoadFailed,
										.Value = i // Log the index of the servo for which calibration load failed, to help with troubleshooting
										});

								ServoCalibrations[i].Calibration = DefaultCalibration.GetServoCalibration(i);

								// Save default calibration to repository for future use.
								if (!CalibrationRepository.SetServoCalibration(i, ServoCalibrations[i].Calibration))
								{
									if (LogListener != nullptr)
										LogListener->OnLog(Inertia::Model::LogEntryStruct{
											.Tag = LOG_TAG,
											.Instance = InstanceId,
											.Type = Inertia::Model::LogTypeEnum::Error,
											.Code = LogCodeEnum::CalibrationSaveFailed,
											.Value = i // Log the index of the servo for which calibration save failed, to help with troubleshooting
											});
								}
							}
						}

						for (uint8_t i = 0; i < ServoCount; i++)
						{
							ServoCalibrations[i].ComputeScales(); // Precompute scale factors for each servo calibration.
						}
					}

					void SetActuator(const uint8_t index, const Inertia::Components::PowerTrain::actuate_t value, const Inertia::Components::PowerTrain::ActuationModeEnum mode) override
					{
						if (index >= ServoCount)
						{
							// Invalid servo index, ignore command.
							return;
						}
						const bool enabled = mode != Inertia::Components::PowerTrain::ActuationModeEnum::Disabled;

						const auto& servoCalibration = ServoCalibrations[index];
						const auto& calibration = servoCalibration.Calibration;

						uint32_t pulseWidthNanos;
						if (value == INT16_MIN)
						{
							pulseWidthNanos = calibration.bottom; // Minimum drive magnitude, set to bottom position.
						}
						else if (value < 0)
						{
							// Negative actuator value, scale within bottom range.
							pulseWidthNanos = static_cast<uint32_t>(static_cast<int32_t>(calibration.center)
								- Scale(servoCalibration.BottomScale, static_cast<uint32_t>(-static_cast<int32_t>(value))));
						}
						else if (value == 0)
						{
							pulseWidthNanos = calibration.center; // Zero actuator value, set to center position.
						}
						else if (value == INT16_MAX)
						{
							pulseWidthNanos = calibration.top; // Maximum actuator value, set to top position.
						}
						else //if (value > 0)
						{
							// Positive actuator value, scale within top range.
							pulseWidthNanos = static_cast<uint32_t>(static_cast<int32_t>(calibration.center)
								+ Scale(servoCalibration.TopScale, static_cast<uint32_t>(value)));
						}

						if (!ServoDriver.SetServo(index, pulseWidthNanos, enabled))
						{
							if (LogListener != nullptr)
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									.Tag = LOG_TAG,
									.Instance = InstanceId,
									.Type = Inertia::Model::LogTypeEnum::Error,
									.Code = LogCodeEnum::ServoSetFailed,
									.Value = index // Log the index of the servo for which the set command failed, to help with troubleshooting
									});

						}
					}
				};
			}
		}
	}
}
#endif