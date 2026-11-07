#ifndef _INERTIA_COMPONENTS_KINEMATIC_CALIBRATED_ACCELERATION_h
#define _INERTIA_COMPONENTS_KINEMATIC_CALIBRATED_ACCELERATION_h

#include "Model.h"
#include "../../Components/Core/DataSource/Model.h"

#include <IntegerTrigonometry16.h>
#include <stdint.h>

namespace Inertia
{
	namespace Components
	{
		namespace Kinematic
		{
			class CalibratedAcceleration
				: public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>
			{
			private:
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>* AccelerationSource;
				IAccelerationCalibration* CalibrationSource;

			private:
				static int32_t RotateX(const int32_t x, const int32_t y, const angle_t angle)
				{
					const auto cosine = IntegerSignal::Trigonometry::Cosine16(angle);
					const auto sine = IntegerSignal::Trigonometry::Sine16(angle);

					return IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(cosine, x)
						- IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(sine, y);
				}

				static int32_t RotateY(const int32_t x, const int32_t y, const angle_t angle)
				{
					const auto cosine = IntegerSignal::Trigonometry::Cosine16(angle);
					const auto sine = IntegerSignal::Trigonometry::Sine16(angle);

					return IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(sine, x)
						+ IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(cosine, y);
				}

				static Inertia::Model::vector32_t RotateRoll(const Inertia::Model::vector32_t& input, const angle_t angle)
				{
					return Inertia::Model::vector32_t{
						.x = input.x,
						.y = RotateX(input.y, input.z, angle),
						.z = RotateY(input.y, input.z, angle)
					};
				}

				static Inertia::Model::vector32_t RotatePitch(const Inertia::Model::vector32_t& input, const angle_t angle)
				{
					return Inertia::Model::vector32_t{
						.x = RotateX(input.x, input.z, angle),
						.y = input.y,
						.z = RotateY(input.x, input.z, angle)
					};
				}

				static Inertia::Model::vector32_t RotateYaw(const Inertia::Model::vector32_t& input, const angle_t angle)
				{
					return Inertia::Model::vector32_t{
						.x = RotateX(input.x, input.y, angle),
						.y = RotateY(input.x, input.y, angle),
						.z = input.z
					};
				}

				static Inertia::Model::vector32_t RotateVector(const Inertia::Model::vector32_t& input, const Inertia::Model::euler_angle_t& orientation)
				{
					return RotateYaw(
						RotatePitch(
							RotateRoll(input, orientation.roll),
							orientation.pitch),
						orientation.yaw);
				}

			public:
				explicit CalibratedAcceleration(
					Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>* accelerationSource,
					IAccelerationCalibration* calibrationSource = nullptr)
					: Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_acceleration_t>()
					, AccelerationSource(accelerationSource)
					, CalibrationSource(calibrationSource)
				{
				}

				void SetCalibrationSource(IAccelerationCalibration* calibrationSource)
				{
					CalibrationSource = calibrationSource;
				}

				bool GetData(Inertia::Model::timestamped_acceleration_t& out_data) override
				{
					if (AccelerationSource == nullptr
						|| !AccelerationSource->GetData(out_data))
					{
						return false;
					}

					if (CalibrationSource == nullptr)
					{
						return true;
					}

					const auto calibration = CalibrationSource->GetAccelerationCalibration();
					const auto rotated = RotateVector(Inertia::Model::vector32_t{
						.x = static_cast<int32_t>(out_data.x),
						.y = static_cast<int32_t>(out_data.y),
						.z = static_cast<int32_t>(out_data.z)
					}, calibration.Orientation);

					out_data.x = static_cast<int16_t>(IntegerSignal::LimitValue<int32_t, INT16_MIN, INT16_MAX>(rotated.x + calibration.Offset.x));
					out_data.y = static_cast<int16_t>(IntegerSignal::LimitValue<int32_t, INT16_MIN, INT16_MAX>(rotated.y + calibration.Offset.y));
					out_data.z = static_cast<int16_t>(IntegerSignal::LimitValue<int32_t, INT16_MIN, INT16_MAX>(rotated.z + calibration.Offset.z));
					return true;
				}
			};
		}
	}
}
#endif