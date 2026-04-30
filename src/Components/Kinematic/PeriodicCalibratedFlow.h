#ifndef _INERTIA_COMPONENTS_KINEMATIC_PERIODIC_CALIBRATED_FLOW_h
#define _INERTIA_COMPONENTS_KINEMATIC_PERIODIC_CALIBRATED_FLOW_h

#include "Model.h"

#include "../../Components/Core/Lifecycle/Model.h"
#include "../../Components/Timestamp/Model.h"

#include <IntegerTrigonometry16.h>

namespace Inertia
{
	namespace Components
	{
		namespace Kinematic
		{
			/// <summary>
			/// A periodic driver that processes and calibrates flow translation data with rotation, flipping, and filtering capabilities.
			/// </summary>
			/// <typeparam name="LowPassFilterFactor">The low-pass filter factor for smoothing flow data. Defaults to 2.</typeparam>
			template<uint8_t LowPassFilterFactor = 2>
			class PeriodicCalibratedFlow
				: public Inertia::Components::Lifecycle::IPeriodicDriver
				, public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_flow_translation_t>
			{
			private:
				static constexpr uint32_t SOURCE_TIMEOUT = 500000;

				IntegerSignal::Filters::LowPassU16<LowPassFilterFactor> FlowFilter{};

			private:
				Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_flow_translation_t>* FlowTranslationSource;

			private:
				IntegerSignal::Trigonometry::angle_t RotationAngle = 0;
				bool FlipX = false;
				bool FlipY = false;

			public:
				Inertia::Model::ILogListener* LogListener = nullptr;
				uint8_t InstanceId = 0;

			private:
				Inertia::Model::timestamped_quality_flow_translation_t LastFlow{};
				Inertia::Model::timestamped_quality_flow_translation_t FilteredFlow{};
				bool HasFlow = false;

			public:
				PeriodicCalibratedFlow(Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_flow_translation_t>* flow_translation_source)
					: Inertia::Components::Lifecycle::IPeriodicDriver()
					, FlowTranslationSource(flow_translation_source)
				{}

				void SetRotation(const IntegerSignal::Trigonometry::angle_t rotationAngle)
				{
					RotationAngle = rotationAngle;
				}

				void SetFlip(const bool flipX, const bool flipY)
				{
					FlipX = flipX;
					FlipY = flipY;
				}

				bool Start() override
				{
					LastFlow = {};
					FilteredFlow = {};
					HasFlow = false;

					return true;
				}

				void Stop() override
				{
					LastFlow = {};
					FilteredFlow = {};
					HasFlow = false;
				}

				bool GetData(Inertia::Model::timestamped_quality_flow_translation_t& out_data) override
				{
					//out_data = LastFlow;
					out_data = FilteredFlow;
					return HasFlow;
				}

			public:
				void Step() override
				{
					Inertia::Model::timestamped_quality_flow_translation_t rawFlow{};

					if (FlowTranslationSource != nullptr
						&& FlowTranslationSource->GetData(rawFlow))
					{
						const uint32_t sampleAge = micros() - rawFlow.timestamp;
						if (sampleAge > (SOURCE_TIMEOUT))
						{
							// Data is too old, skip processing and mark flow as unavailable.
							HasFlow = false;
						}
						else
						{
							int32_t flowX = rawFlow.x;
							int32_t flowY = rawFlow.y;

							if (FlipX)
							{
								flowX = -flowX;
							}

							if (FlipY)
							{
								flowY = -flowY;
							}

							const auto cosine = IntegerSignal::Trigonometry::Cosine16(RotationAngle);
							const auto sine = IntegerSignal::Trigonometry::Sine16(RotationAngle);

							LastFlow.x =
								IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(cosine, flowX)
								- IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(sine, flowY);
							LastFlow.y =
								IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(sine, flowX)
								+ IntegerSignal::FixedPoint::ScalarFraction::Fraction<int32_t>(cosine, flowY);
							LastFlow.timestamp = rawFlow.timestamp;
							LastFlow.quality = rawFlow.quality;

							FilteredFlow = LastFlow;
							HasFlow = true;
						}
					}
					else
					{
						HasFlow = false;
					}


				}
			};
		}
	}
}
#endif