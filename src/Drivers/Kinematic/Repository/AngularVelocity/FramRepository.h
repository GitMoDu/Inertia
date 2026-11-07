#ifndef _INERTIA_COMPONENTS_KINEMATIC_ANGULAR_VELOCITY_CALIBRATION_FRAM_REPOSITORY_h
#define _INERTIA_COMPONENTS_KINEMATIC_ANGULAR_VELOCITY_CALIBRATION_FRAM_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <Fletcher16.h>
#include "../../../../Components/Kinematic/Model.h"
#include "../../../Storage/Fram/StructStore.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Kinematic
		{
			namespace Repository
			{
				namespace AngularVelocity
				{
					template<uint16_t BaseAddress = 0>
					class FramRepository : public Inertia::Components::Kinematic::IAngularVelocityCalibrationRepository
					{
					private:
						struct calibration_store_t
						{
							Inertia::Components::Kinematic::angular_velocity_calibration_t Calibration{};
						};

						static constexpr uint16_t StoreVersion = 1;
						using Store = Inertia::Drivers::Storage::Fram::StructStore<calibration_store_t, StoreVersion, BaseAddress>;

					public:
						static constexpr size_t UsedSize = Store::UsedSize;

					private:
						Store FileStore;
						bool Started = false;

					public:
						explicit FramRepository(Inertia::Components::Storage::Fram::IFramDriver& driver)
							: Inertia::Components::Kinematic::IAngularVelocityCalibrationRepository()
							, FileStore(driver)
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

						bool GetAngularVelocityCalibration(Inertia::Components::Kinematic::angular_velocity_calibration_t& calibration) override
						{
							if (!Started)
							{
								return false;
							}

							calibration_store_t store{};
							if (!FileStore.Read(store))
							{
								return false;
							}

							calibration = store.Calibration;
							return true;
						}

						bool SetAngularVelocityCalibration(const Inertia::Components::Kinematic::angular_velocity_calibration_t& calibration) override
						{
							if (!Started)
							{
								return false;
							}

							calibration_store_t store{};
							store.Calibration = calibration;
							return FileStore.Write(store);
						}

						bool ClearAngularVelocityCalibration() override
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
}

#endif