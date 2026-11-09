#ifndef _INERTIA_DRIVERS_KINEMATIC_DISTANCE_FRAM_REPOSITORY_h
#define _INERTIA_DRIVERS_KINEMATIC_DISTANCE_FRAM_REPOSITORY_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <Fletcher16.h>
#include "../../../../Components/Storage/Fram/Model.h"
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
				namespace Distance
				{
					template<uint16_t BaseAddress = 0>
					class FramRepository
						: public Inertia::Components::Kinematic::IDistanceCalibrationRepository
					{
					private:
						static constexpr uint16_t StoreVersion = 1;
						using Store = Inertia::Drivers::Storage::Fram::StructStore<
							Inertia::Components::Kinematic::distance_calibration_t,
							StoreVersion,
							BaseAddress>;

						Store FileStore;
						bool Started = false;

						static bool IsValid(
							const Inertia::Components::Kinematic::distance_calibration_t& calibration)
						{
							return calibration.BaseDistance < calibration.MaxDistance;
						}

					public:
						static constexpr size_t UsedSize = Store::UsedSize;

						explicit FramRepository(
							Inertia::Components::Storage::Fram::IFramDriver& driver)
							: FileStore(driver)
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

						bool GetDistanceCalibration(
							Inertia::Components::Kinematic::distance_calibration_t& calibration) override
						{
							return Started && FileStore.Read(calibration) && IsValid(calibration);
						}

						bool SetDistanceCalibration(
							const Inertia::Components::Kinematic::distance_calibration_t& calibration) override
						{
							return Started && IsValid(calibration) && FileStore.Write(calibration);
						}

						bool ClearDistanceCalibration() override
						{
							const Inertia::Components::Kinematic::distance_calibration_t calibration{};
							return Started && FileStore.Write(calibration);
						}
					};
				}
			}
		}
	}
}

#endif