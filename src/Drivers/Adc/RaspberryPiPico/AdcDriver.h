#ifndef _INERTIA_DRIVERS_ADC_RASPBERRY_PI_PICO_ADC_DRIVER_h
#define _INERTIA_DRIVERS_ADC_RASPBERRY_PI_PICO_ADC_DRIVER_h

#if defined(ARDUINO) && defined(ARDUINO_ARCH_RP2040)

#include "../../../Components/Adc/Model.h"

#include <hardware/adc.h>

namespace Inertia
{
	namespace Drivers
	{
		namespace Adc
		{
			namespace RaspberryPiPico
			{
				// Number of bits of resolution for the ADC on the Raspberry Pi Pico.
				static constexpr uint16_t ADC_BITS = 12;

				// Maximum raw ADC value based on the number of bits (4095 for 12-bit ADC).
				static constexpr uint16_t ADC_MAX = (uint16_t(1) << ADC_BITS) - 1;

				/// <summary>
				/// Driver for reading analog values from an ADC pin on RaspberryPiPico 1/2. 
				/// Implements lifecycle management and provides scaled analog readings through the IDataSource interface.
				/// </summary>
				/// <typeparam name="AdcPin">The GPIO pin number to use for ADC input. Must be within the valid ADC pin range.</typeparam>
				template<uint8_t AdcPin
				>
				class AdcDriver
					: public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Inertia::Components::DataSource::IDataSource<Inertia::Components::Adc::analog_value_t>
				{
				private:
					// Time in microseconds to wait after starting an ADC read before the result is ready.
					static constexpr uint8_t SETTLE_TIME_US = 5;



					static constexpr uint8_t PIN_ADC0 = ADC_BASE_PIN;
					static constexpr uint8_t PIN_ADC_MAX = PIN_ADC0 + NUM_ADC_CHANNELS - 1;

					// Pre-calculated scale factor to convert raw ADC readings to the full uint16_t range, using fixed-point scaling for efficiency.
					static constexpr auto AdcScaleFactor = IntegerSignal::FixedPoint::FactorScale::Scale16::GetFactor<uint16_t>(Inertia::Components::Adc::ANALOG_VALUE_MAX, ADC_MAX);

				private:
					bool Started = false;

				public:
					AdcDriver() : Inertia::Components::Lifecycle::ILifecycleDriver()
						, Inertia::Components::DataSource::IDataSource<Inertia::Components::Adc::analog_value_t>()
					{}

					bool Start() override
					{
						if (!IsSupportedAdcPin())
						{
							return false;
						}

						if (Started)
						{
							return true;
						}

						adc_init();
						adc_gpio_init(AdcPin);
						Started = true;
						return true;
					}

					void Stop() override
					{
						Started = false;
					}

					bool GetData(Inertia::Components::Adc::analog_value_t& value) override
					{
						if (!Started)
						{
							return false;
						}

						adc_select_input(GetAdcInput());
						adc_read();
						delayMicroseconds(SETTLE_TIME_US);
						value = IntegerSignal::FixedPoint::FactorScale::Scale(AdcScaleFactor, adc_read());

						return true;
					}

				private:
					static constexpr bool IsSupportedAdcPin()
					{
						return AdcPin >= PIN_ADC0 && AdcPin <= PIN_ADC_MAX;
					}

					static constexpr uint8_t GetAdcInput()
					{
						return static_cast<uint8_t>(AdcPin - PIN_ADC0);
					}
				};

				class InternalTemperatureDriver
					: public Inertia::Components::Lifecycle::ILifecycleDriver
					, public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_temperature_t>
				{
				private:
					static constexpr int32_t TemperatureKelvinOffsetMilliKelvin = 273150;

					static constexpr uint8_t InternalTemperatureAdcInput = NUM_ADC_CHANNELS - 1;
					static constexpr uint8_t SETTLE_TIME_US = 10;
					static constexpr int32_t TemperatureBaseMilliKelvin = 300150;
					static constexpr int32_t TemperatureBaseMicroVolts = 706000;
					static constexpr int32_t TemperatureSlopeNanoVoltsPerMilliKelvin = 17210;
					static constexpr int32_t ReferenceMicroVolts = 3300000;

					bool Started = false;

				public:
					InternalTemperatureDriver()
						: Inertia::Components::Lifecycle::ILifecycleDriver()
						, Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_temperature_t>()
					{}

					bool Start() override
					{
						if (Started)
						{
							return true;
						}

						adc_init();
						adc_set_temp_sensor_enabled(true);
						Started = true;
						return true;
					}

					void Stop() override
					{
						adc_set_temp_sensor_enabled(false);
						Started = false;
					}

					bool GetData(Inertia::Model::timestamped_temperature_t& value) override
					{
						if (!Started)
						{
							return false;
						}

						adc_select_input(InternalTemperatureAdcInput);
						adc_read();
						delayMicroseconds(SETTLE_TIME_US);

						const uint16_t rawValue = adc_read();
						const int32_t voltageMicroVolts = (static_cast<int32_t>(rawValue) * ReferenceMicroVolts) / ADC_MAX;
						const int32_t deltaMicroVolts = TemperatureBaseMicroVolts - voltageMicroVolts;
						const int32_t temperatureMilliKelvin = TemperatureBaseMilliKelvin
							+ ((deltaMicroVolts * 1000) / TemperatureSlopeNanoVoltsPerMilliKelvin);

						value.temperature = static_cast<Inertia::Model::temperature_t>(IntegerSignal::MaxValue<int32_t>(temperatureMilliKelvin, 0) / 10);
						value.timestamp = micros();

						return true;
					}
				};
			}
		}
	}
}
#endif
#endif
