#ifndef _INERTIA_COMPONENTS_AHRS_XIO_MODEL_h
#define _INERTIA_COMPONENTS_AHRS_XIO_MODEL_h

#include <IntegerSignal.h>

#include "../Model.h"

extern "C"
{
#include "Source/FusionAhrs.h"
#include "Source/FusionBias.h"
}

namespace Inertia
{
	namespace Components
	{
		namespace Ahrs
		{
			namespace Xio
			{
				static constexpr uint32_t LOG_TAG = 787473685; // Random unique tag for Xio AHRS.

				enum class LogCodeEnum : uint8_t
				{};

				struct diagnostics_t
				{
					Inertia::Model::timestamped_acceleration_t Acceleration{};
					Inertia::Model::timestamped_angular_velocity_t AngularVelocity{};
					Inertia::Model::timestamped_magnet_t Magnetometer{};
					FusionEuler Orientation{};
					FusionVector GyroscopeBiasDegreesPerSecond{};
					FusionAhrsInternalStates States{};
					FusionAhrsFlags Flags{};
					uint32_t SamplePeriodMicros = 0;
				};
			}
		}
	}
}
#endif