#ifndef _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATED_DRIVER_h
#define _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATED_DRIVER_h

#include "Model.h"
#include "../../Core/DataSource/Model.h"
#include "../../Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			namespace PwmActuator
			{
				using namespace IntegerSignal::FixedPoint::FactorScale;

				template<uint8_t PwmCount>
				class CalibratedDriver : public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Inertia::Components::PowerTrain::IActuatorDriver
				{
				private:
					struct cached_pwm_calibration_t
					{
						Inertia::Components::PowerTrain::PwmActuator::pwm_calibration_t Calibration{};
						scale32_t ScaleFactor = 0;

						void ComputeScale()
						{
							const uint32_t range = Calibration.top >= Calibration.bottom
								? (Calibration.top - Calibration.bottom)
								: 0;
							ScaleFactor = range > 0 ? GetFactor32(range, static_cast<uint32_t>(Inertia::Components::PowerTrain::MAX_ACTUATE_VALUE)) : 0;
						}
					};

				private:
					Inertia::Components::PowerTrain::Pwm::IPwmDriver& PwmDriver;
					Inertia::Components::PowerTrain::PwmActuator::IPwmCalibration& DefaultCalibration;
					Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository& CalibrationRepository;

				private:
					cached_pwm_calibration_t PwmCalibrations[PwmCount]{};

				public:
					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0;

				public:
					CalibratedDriver(Inertia::Components::PowerTrain::Pwm::IPwmDriver& pwmDriver,
						Inertia::Components::PowerTrain::PwmActuator::IPwmCalibration& defaultCalibration,
						Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository& calibrationRepository)
						: Inertia::Components::Lifecycle::ILifecycleDriver()
						, Inertia::Components::PowerTrain::IActuatorDriver()
						, PwmDriver(pwmDriver)
						, DefaultCalibration(defaultCalibration)
						, CalibrationRepository(calibrationRepository)
					{}

					~CalibratedDriver() = default;

					bool Start() override
					{
						if (PwmDriver.GetPwmCount() != PwmCount)
						{
							return false;
						}

						LoadCalibrations();
						return true;
					}

					void Stop() override
					{
					}

					uint8_t GetDriveCount() const
					{
						return PwmCount;
					}

					void LoadCalibrations()
					{
						for (uint8_t i = 0; i < PwmCount; i++)
						{
							if (!CalibrationRepository.GetPwmCalibration(i, PwmCalibrations[i].Calibration))
							{
								if (LogListener != nullptr)
									LogListener->OnLog(Inertia::Model::LogEntryStruct{
										.Tag = LOG_TAG,
										.Instance = InstanceId,
										.Type = Inertia::Model::LogTypeEnum::Warning,
										.Code = static_cast<uint8_t>(LogCodeEnum::CalibrationLoadFailed),
										.Value = i });

								PwmCalibrations[i].Calibration = DefaultCalibration.GetPwmCalibration(i);

								if (!CalibrationRepository.SetPwmCalibration(i, PwmCalibrations[i].Calibration))
								{
									if (LogListener != nullptr)
										LogListener->OnLog(Inertia::Model::LogEntryStruct{
											.Tag = LOG_TAG,
											.Instance = InstanceId,
											.Type = Inertia::Model::LogTypeEnum::Error,
											.Code = static_cast<uint8_t>(LogCodeEnum::CalibrationSaveFailed),
											.Value = i });
								}
							}

							PwmCalibrations[i].ComputeScale();
						}
					}

					void SetActuator(const uint8_t index, const Inertia::Components::PowerTrain::actuate_t value, const Inertia::Components::PowerTrain::ActuationModeEnum mode) override
					{
						if (index >= PwmCount)
						{
							return;
						}

						const bool enabled = mode != Inertia::Components::PowerTrain::ActuationModeEnum::Disabled;
						const auto& calibration = PwmCalibrations[index].Calibration;

						const uint32_t level = static_cast<uint32_t>(calibration.bottom)
							+ Scale(PwmCalibrations[index].ScaleFactor, value);

						PwmDriver.SetPwm(index,
							Inertia::Components::PowerTrain::Pwm::LimitLevel(
								IntegerSignal::LimitValue<uint32_t>(level, calibration.bottom, calibration.top)),
							enabled);
					}
				};
			}
		}
	}
}
#endif