#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATION_FRAM_REPOSITORY_h
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATION_FRAM_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <Fletcher16.h>
#include "../../../Components/PowerTrain/ServoActuator/Model.h"
#include "../../Storage/Fram/StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Servo
		{
			namespace Repository
			{
				template<uint8_t ServoCount, uint16_t BaseAddress = 0>
				class FramRepository : public Inertia::Components::PowerTrain::ServoActuator::IServoCalibrationRepository
				{
				private:
					struct calibration_store_t
					{
						Inertia::Components::PowerTrain::ServoActuator::servo_calibration_t Calibrations[ServoCount]{};
					};

					static constexpr uint16_t StoreVersion = 1;
					using Store = Inertia::Drivers::Storage::Fram::StructStore<calibration_store_t, StoreVersion, BaseAddress>;

				public:
					static constexpr size_t UsedSize = Store::UsedSize;

				private:
					Store FileStore;
					Inertia::Components::PowerTrain::ServoActuator::IServoCalibration& DefaultCalibration;
					bool Started = false;

				private:
					void SeedDefaults(calibration_store_t& store)
					{
						for (uint8_t i = 0; i < ServoCount; ++i)
						{
							store.Calibrations[i] = DefaultCalibration.GetServoCalibration(i);
						}
					}

				public:
					explicit FramRepository(Inertia::Components::Storage::Fram::IFramDriver& driver,
						Inertia::Components::PowerTrain::ServoActuator::IServoCalibration& defaultCalibration)
						: Inertia::Components::PowerTrain::ServoActuator::IServoCalibrationRepository()
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

					bool GetServoCalibration(const uint8_t index, Inertia::Components::PowerTrain::ServoActuator::servo_calibration_t& calibration) override
					{
						if (!Started || index >= ServoCount)
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

					bool SetServoCalibration(const uint8_t index, const Inertia::Components::PowerTrain::ServoActuator::servo_calibration_t& calibration) override
					{
						if (!Started || index >= ServoCount)
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

					bool ClearServoCalibrations() override
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