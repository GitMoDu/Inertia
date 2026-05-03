#ifndef _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATION_FRAM_REPOSITORY_h
#define _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATION_FRAM_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <Fletcher16.h>
#include "../../../Components/PowerTrain/PwmActuator/Model.h"
#include "../../Storage/Fram/StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Pwm
		{
			namespace Repository
			{
				template<uint8_t PwmCount, uint16_t BaseAddress = 0>
				class FramRepository : public Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository
				{
				private:
					struct calibration_store_t
					{
						Inertia::Components::PowerTrain::PwmActuator::pwm_calibration_t Calibrations[PwmCount]{};
					};

					static constexpr uint16_t StoreVersion = 1;
					using Store = Inertia::Drivers::Storage::Fram::StructStore<calibration_store_t, StoreVersion, BaseAddress>;

				public:
					static constexpr size_t UsedSize = Store::UsedSize;

				private:
					Store FileStore;
					Inertia::Components::PowerTrain::PwmActuator::IPwmCalibration& DefaultCalibration;
					bool Started = false;

				private:
					void SeedDefaults(calibration_store_t& store)
					{
						for (uint8_t i = 0; i < PwmCount; ++i)
						{
							store.Calibrations[i] = DefaultCalibration.GetPwmCalibration(i);
						}
					}

				public:
					explicit FramRepository(Inertia::Components::Storage::Fram::IFramDriver& driver,
						Inertia::Components::PowerTrain::PwmActuator::IPwmCalibration& defaultCalibration)
						: Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository()
						, FileStore(driver)
						, DefaultCalibration(defaultCalibration)
					{}

					bool Start() override
					{
						Started = true;
						return true;
					}

					void Stop() override
					{
						Started = false;
					}

					bool GetPwmCalibration(const uint8_t index, Inertia::Components::PowerTrain::PwmActuator::pwm_calibration_t& calibration) override
					{
						if (!Started || index >= PwmCount)
						{
							return false;
						}

						calibration_store_t store{};
						if (!FileStore.Read(store))
						{
							return false;
						}

						calibration = store.Calibrations[index];
						return true;
					}

					bool SetPwmCalibration(const uint8_t index, const Inertia::Components::PowerTrain::PwmActuator::pwm_calibration_t& calibration) override
					{
						if (!Started || index >= PwmCount)
						{
							return false;
						}

						calibration_store_t store{};
						if (!FileStore.Read(store))
						{
							SeedDefaults(store);
						}
						store.Calibrations[index] = calibration;
						return FileStore.Write(store);
					}

					bool ClearPwmCalibrations() override
					{
						if (!Started)
						{
							return false;
						}

						const calibration_store_t store{};
						return FileStore.Write(store);
					}
				};
			}
		}
	}
}

#endif