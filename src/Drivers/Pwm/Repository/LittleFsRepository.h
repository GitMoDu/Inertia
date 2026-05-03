#ifndef _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATION_LITTLEFS_REPOSITORY_h
#define _INERTIA_COMPONENTS_POWERTRAIN_PWM_ACTUATOR_CALIBRATION_LITTLEFS_REPOSITORY_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "../../../Components/PowerTrain/PwmActuator/Model.h"
#include "../../Storage/LittleFs/StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Pwm
		{
			namespace Repository
			{
				template<uint8_t PwmCount>
				class LittleFsRepository : public Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository
				{
				private:
					struct calibration_store_t
					{
						Inertia::Components::PowerTrain::PwmActuator::pwm_calibration_t Calibrations[PwmCount]{};
					};

					static constexpr uint16_t StoreVersion = 1;
					using Store = Inertia::Drivers::Storage::LittleFs::StructStore<calibration_store_t, StoreVersion>;

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
					explicit LittleFsRepository(Inertia::Components::PowerTrain::PwmActuator::IPwmCalibration& defaultCalibration,
						const char* path = "/pwm-calibration.bin")
						: Inertia::Components::PowerTrain::PwmActuator::IPwmCalibrationRepository()
						, FileStore(path)
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
						if (FileStore.Exists())
						{
							if (!FileStore.Read(store))
							{
								SeedDefaults(store);
							}
						}
						else
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

						return FileStore.Delete();
					}
				};
			}
		}
	}
}

#endif
#endif