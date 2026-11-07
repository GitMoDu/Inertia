#ifndef _INERTIA_COMPONENTS_KINEMATIC_CALIBRATED_ANGULAR_VELOCITY_h
#define _INERTIA_COMPONENTS_KINEMATIC_CALIBRATED_ANGULAR_VELOCITY_h

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
            class CalibratedAngularVelocity
                : public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>
            {
            private:
                Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* AngularVelocitySource;
                IAngularVelocityCalibration* CalibrationSource;

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
                explicit CalibratedAngularVelocity(
                    Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>* angularVelocitySource,
                    IAngularVelocityCalibration* calibrationSource = nullptr)
                    : Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_angular_velocity_t>()
                    , AngularVelocitySource(angularVelocitySource)
                    , CalibrationSource(calibrationSource)
                {
                }

                void SetCalibrationSource(IAngularVelocityCalibration* calibrationSource)
                {
                    CalibrationSource = calibrationSource;
                }

                bool GetData(Inertia::Model::timestamped_angular_velocity_t& out_data) override
                {
                    if (AngularVelocitySource == nullptr
                        || !AngularVelocitySource->GetData(out_data))
                    {
                        return false;
                    }

                    if (CalibrationSource == nullptr)
                    {
                        return true;
                    }

                    const auto calibration = CalibrationSource->GetAngularVelocityCalibration();
                    auto value = RotateVector(Inertia::Model::vector32_t{
                        .x = out_data.x,
                        .y = out_data.y,
                        .z = out_data.z
                    }, calibration.BaseOrientation);

                    value.x += calibration.Offset.x;
                    value.y += calibration.Offset.y;
                    value.z += calibration.Offset.z;
                    value = RotateVector(value, calibration.Orientation);

                    out_data.x = value.x;
                    out_data.y = value.y;
                    out_data.z = value.z;
                    return true;
                }
            };
        }
    }
}

#endif
