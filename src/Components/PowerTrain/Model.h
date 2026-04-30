#ifndef _INERTIA_COMPONENTS_POWERTRAIN_MODEL_h
#define _INERTIA_COMPONENTS_POWERTRAIN_MODEL_h

#include <IntegerSignal.h>

#include "../../Components/Core/DataSource/Model.h"
#include "../../Components/Core/Lifecycle/Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace PowerTrain
		{
			static constexpr uint32_t LOG_TAG = 54356945; // Random unique tag for PowerTrain.

			enum class LogCodeEnum : uint8_t
			{};

			enum class DriveMode
			{
				Disabled,
				Enabled
			};

			enum class ActuationModeEnum : uint8_t
			{
				Disabled, // No actuation, regardless of magnitude.
				Forward, // Actuation in the forward/right direction, with magnitude representing the level of effort or speed.
				Backward, // Actuation in the backward/left direction, with magnitude representing the level of effort or speed.
				Custom
				// Additional modes can be defined as needed, such as different control schemes or safety limits
			};

			/// <summary>
			/// Internal representation of drive magnitude for actuators.
			/// Wide range allows for filtering, curving and chaining of drive commands while maintaining output precision.
			/// </summary>
			//using magnitude_t = int32_t;



			/// <summary>
			/// Generic end point actuator drive command, with a mode and magnitude. 
			/// Typically used for servo motors, where the mode can indicate whether the drive is active or not
			/// and the magnitude can represent the desired position or speed within the calibrated range. 
			/// The specific interpretation of the mode and magnitude can be defined by the implementation 
			/// of the actuator driver allowing for flexibility in how different types of actuators are 
			/// controlled using this common command structure.
			/// </summary>
			using actuate_t = uint16_t;

			static constexpr actuate_t MAX_ACTUATE_VALUE = UINT16_MAX;

			struct IActuatorDriver
			{
				~IActuatorDriver() = default;

				virtual void SetActuator(const uint8_t index, const actuate_t value, const ActuationModeEnum mode) = 0;
			};


			struct quad_stabilization_effort_t : public Inertia::Model::effort_t
			{};

			struct power_effort_t : public Inertia::Model::effort_t
			{};


			/// <summary>
			/// Represents a command structure for controlling a quadcopter.
			/// </summary>
			struct QuadCommandStruct
			{
				Inertia::Model::euler_angle_t OrientationTarget{};
				Inertia::Model::vector32_t AngularVelocityTarget{};
				actuate_t PowerTarget = 0;
				bool Enabled = true;
			};

			struct IQuadDriver
				: public Inertia::Components::DataSource::IDataSource<quad_stabilization_effort_t>
				, public Inertia::Components::DataSource::IDataSource<power_effort_t>
			{
				virtual void SetQuadCommand(const QuadCommandStruct& command) = 0;

				//virtual bool GetQuadStabilizationEffort(effort_t& data) = 0;

				//virtual bool GetQuadPowerEffort(effort_t& data) = 0;
			};
		}
	}
}
#endif