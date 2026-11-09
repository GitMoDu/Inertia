#ifndef _INERTIA_KINEMATIC_MODEL_h
#define _INERTIA_KINEMATIC_MODEL_h

#include "../../Components/Core/Primitives.h"
#include "../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Kinematic
		{
			struct distance_calibration_t
			{
				uint16_t BaseDistance;
				uint16_t MaxDistance;
			};

			struct IDistanceCalibrationRepository : public Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~IDistanceCalibrationRepository() = default;

				virtual bool GetDistanceCalibration(distance_calibration_t& calibration) = 0;
				virtual bool SetDistanceCalibration(const distance_calibration_t& calibration) = 0;
				virtual bool ClearDistanceCalibration() = 0;
			};

			struct acceleration_calibration_t
			{
				Inertia::Model::vector16_t Offset{};
				Inertia::Model::euler_angle_t Orientation{};
			};

			struct angular_velocity_calibration_t
			{
				Inertia::Model::vector32_t Offset{};
				Inertia::Model::euler_angle_t Orientation{};
				Inertia::Model::euler_angle_t BaseOrientation{};
			};

			struct IAccelerationCalibration
			{
				~IAccelerationCalibration() = default;

				virtual acceleration_calibration_t GetAccelerationCalibration() = 0;
			};

			struct IAngularVelocityCalibration
			{
				~IAngularVelocityCalibration() = default;

				virtual angular_velocity_calibration_t GetAngularVelocityCalibration() = 0;
			};

			struct IAccelerationCalibrationRepository : public Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~IAccelerationCalibrationRepository() = default;

				virtual bool GetAccelerationCalibration(acceleration_calibration_t& calibration) = 0;
				virtual bool SetAccelerationCalibration(const acceleration_calibration_t& calibration) = 0;
				virtual bool ClearAccelerationCalibration() = 0;
			};

			struct IAngularVelocityCalibrationRepository : public Inertia::Components::Lifecycle::ILifecycleDriver
			{
				~IAngularVelocityCalibrationRepository() = default;

				virtual bool GetAngularVelocityCalibration(angular_velocity_calibration_t& calibration) = 0;
				virtual bool SetAngularVelocityCalibration(const angular_velocity_calibration_t& calibration) = 0;
				virtual bool ClearAngularVelocityCalibration() = 0;
			};
		}
	}

}
#endif