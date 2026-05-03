#ifndef _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATION_LITTLEFS_REPOSITORY_h
#define _INERTIA_COMPONENTS_POWERTRAIN_SERVO_ACTUATOR_CALIBRATION_LITTLEFS_REPOSITORY_h

#if defined(ARDUINO_ARCH_RP2040) \
 || defined(ARDUINO_ARCH_ESP8266) \
 || defined(ARDUINO_ARCH_ESP32)

#include "../../../Components/PowerTrain/ServoActuator/Model.h"
#include "../../Storage/LittleFs/StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Servo
		{
			namespace Repository
			{
				template<uint8_t ServoCount>
				class LittleFsRepository : public Inertia::Components::PowerTrain::ServoActuator::IServoCalibrationRepository
				{
				private:
					struct calibration_store_t
					{
						Inertia::Components::PowerTrain::ServoActuator::servo_calibration_t Calibrations[ServoCount]{};
					};

					static constexpr uint16_t StoreVersion = 1;
					using Store = Inertia::Drivers::Storage::LittleFs::StructStore<calibration_store_t, StoreVersion>;

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
					explicit LittleFsRepository(Inertia::Components::PowerTrain::ServoActuator::IServoCalibration& defaultCalibration,
						const char* path = "/servo-calibration.bin")
						: Inertia::Components::PowerTrain::ServoActuator::IServoCalibrationRepository()
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

					bool ClearServoCalibrations() override
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