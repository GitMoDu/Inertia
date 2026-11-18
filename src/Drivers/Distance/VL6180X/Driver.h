#ifndef _INERTIA_DRIVERS_DISTANCE_VL6180X_DRIVER_h
#define _INERTIA_DRIVERS_DISTANCE_VL6180X_DRIVER_h

#include <Arduino.h>
#include <Wire.h>
#include "Model.h"
#include "Initialization.h"
#include "../../../Components/Distance/Model.h"
#include "../../../Components/Core/DataSource/Model.h"
#include "../../../Components/Core/Lifecycle/Model.h"
#include "../../../Components/Log/Model.h"
#include "../../HardwareInterface/I2c/I2cInterface.h"

namespace Inertia
{
	namespace Drivers
	{
		namespace Distance
		{
			namespace Vl6180x
			{
				/// <summary>
				/// VL6180X standard (1 mm/unit, no upscaling) single-shot range driver.
				/// Implements ST AN4545 Rev 1 sections 1, 2 and 9 (SR03 settings).
				/// Protocol/status cross-reference: ST's stm32duino/VL6180X,
				/// src/vl6180x_def.h and src/vl6180x_class.cpp.
				/// https://www.st.com/resource/en/application_note/an4545-vl6180x-basic-ranging-application-note-stmicroelectronics.pdf
				/// The caller initializes/owns the bus, its transaction timeout, CE pins and scheduling.
				/// No bus begin/end, delay or busy-poll loops. I2C calls themselves are synchronous.
				/// Serialize access to a shared bus; this driver does not provide a bus lock.
				/// Transport/reset/timeout errors stop sampling until explicit Start recovery.
				/// Sensor-invalid results leave the last valid sample and filter unchanged;
				/// the sample still expires normally if no valid acquisition replaces it.
				/// Acquisition rejections are diagnostic-only. VCSEL/PLL statuses 1..5 log Errors
				/// once per status until five consecutive valid shots or restart rearm reporting.
				/// A single valid shot still restores measurement availability immediately.
				/// FilterFactor optionally applies a per-instance low-pass filter to each new valid
				/// acquisition (alpha = 1 / 2^FilterFactor). Zero preserves unfiltered output.
				/// First samples after start or stale gaps seed the filter directly.
				/// </summary>
				template<typename WireType = TwoWire, uint8_t FilterFactor = 0>
				class Driver : public Inertia::Components::Lifecycle::IPeriodicDriver,
					public Inertia::Components::DataSource::IDataSource<Inertia::Model::timestamped_quality_range16_t>,
					public Inertia::Components::DataSource::IDataSource<Inertia::Components::Distance::timestamped_signal_range16_t>
				{
					static_assert(FilterFactor <= 16, "Range filter factor must be between 0 and 16.");

				private:
					WireType& WireInstance;
					uint8_t Address;
					const uint32_t ClockHz;
					const uint32_t MeasurementTimeoutMillis;
					const uint32_t MeasurementPeriodMillis;
					const uint8_t MaxConvergenceMillis;
					const uint8_t AveragingSamplePeriod;
					const uint16_t CrosstalkCompensationRate;
					const int8_t PartToPartOffsetMm;
					const bool UseCustomOffset;
					Vl6180xDiagnostics Diagnostics{};
					bool DiagnosticsAvailable = false;
					int8_t FactoryOffset = 0;
					bool FactoryOffsetAvailable = false;
					Inertia::Components::Distance::timestamped_signal_range16_t Sample{};
					IntegerSignal::Filters::LowPassU16<FilterFactor> DistanceFilter{};
					uint32_t SampleMillis = 0;
					uint32_t LastStartMillis = 0;
					uint32_t WaitSinceMillis = 0;
					bool Running = false;
					bool Pending = false;
					bool Available = false;
					ErrorEnum LastError = ErrorEnum::None;
					uint8_t RangeStatus = static_cast<uint8_t>(RangeStatusEnum::NoReading);
					uint8_t ReportedHardwareFaults = 0;
					uint8_t ConsecutiveValidReadings = 0;
					static constexpr uint8_t ValidReadingRecoveryThreshold = 5;

				public:
					using DataTypes = Inertia::Components::DataSource::Variadic::VariadicDataTypeList<
						Inertia::Model::timestamped_quality_range16_t,
						Inertia::Components::Distance::timestamped_signal_range16_t>;

					Inertia::Model::ILogListener* LogListener = nullptr;
					uint8_t InstanceId = 0;

					/// <summary>
					/// Address is the current 7-bit address, not an automatic reassignment target.
					/// Clock must be 1..400000 Hz; reapplied before every I2C transaction through
					/// Inertia's platform-aware SetClockIfSupported (other drivers must do likewise).
					/// On unsupported platforms the caller must configure the bus clock explicitly.
					/// Period is the minimum time between shot starts; Step may run more frequently.
					/// Timeout bounds readiness/completion waits, not a blocking Wire call.
					/// AN4545 section 6: max convergence is 2..63 ms; a longer budget helps weak returns.
					/// Averaging period is register 0x010A (0..255); higher values reduce noise but
					/// increase measurement time. These are not emitter-power or ALS-gain controls.
					/// Set the host timeout above the hardware measurement budget plus readout time.
					/// Crosstalk compensation is unsigned 9.7 Mcps (128 = 1 Mcps), default zero.
					/// The signed mm offset replaces the hardware offset only when useCustomOffset
					/// is true. Otherwise the offset register is left untouched, including on restart.
					/// </summary>
					Driver(WireType& wire, const uint8_t address = DEFAULT_ADDRESS,
						const uint32_t clockHz = 400000, const uint32_t measurementTimeoutMillis = 100,
						const uint32_t measurementPeriodMillis = 50,
						const uint8_t maxConvergenceMillis = 49, const uint8_t averagingSamplePeriod = 0x30,
						const uint16_t crosstalkCompensationRate = 0, const int8_t partToPartOffsetMm = 0,
						const bool useCustomOffset = false)
						: WireInstance(wire), Address(address), ClockHz(clockHz),
						MeasurementTimeoutMillis(measurementTimeoutMillis),
						MeasurementPeriodMillis(measurementPeriodMillis),
						MaxConvergenceMillis(maxConvergenceMillis), AveragingSamplePeriod(averagingSamplePeriod),
						CrosstalkCompensationRate(crosstalkCompensationRate), PartToPartOffsetMm(partToPartOffsetMm),
						UseCustomOffset(useCustomOffset)
					{}

					bool Start() final
					{
						if (Running)
						{
							return true;
						}
						Available = false;
						Pending = false;
						DiagnosticsAvailable = false;
						RangeStatus = static_cast<uint8_t>(RangeStatusEnum::NoReading);
						ReportedHardwareFaults = 0;
						ConsecutiveValidReadings = 0;
						if (!IsAddressValid(Address) || ClockHz == 0 || ClockHz > 400000
							|| MeasurementTimeoutMillis == 0 || MeasurementTimeoutMillis > 0x7FFFFFFFUL
							|| MaxConvergenceMillis < 2 || MaxConvergenceMillis > 63
							|| MeasurementPeriodMillis > 0x7FFFFFFFUL - MeasurementTimeoutMillis)
						{
							return Fail(ErrorEnum::InvalidConfiguration);
						}
						uint8_t value = 0;
						if (!Read(Register::ModelId, value)) return Fail(ErrorEnum::I2c);
						if (value != MODEL_ID) return Fail(ErrorEnum::WrongModel);
						if (!Read(Register::FreshOutOfReset, value)) return Fail(ErrorEnum::I2c);
						if (value > 1) return Fail(ErrorEnum::NotReady);
						if (!Read(Register::RangeStatus, value)) return Fail(ErrorEnum::I2c);
						if ((value & RANGE_READY_MASK) == 0) return Fail(ErrorEnum::NotReady);

						for (const auto& setting : PrivateInitialization)
						{
							if (!Write(setting.Register, setting.Value)) return Fail(ErrorEnum::I2c);
						}
						if (!Write(Register::ModeGpio1, 0x10)
							|| !Write(Register::AveragingSamplePeriod, AveragingSamplePeriod)
							|| !Write(Register::MaxConvergenceTime, MaxConvergenceMillis)
							|| !Write(Register::VhvRepeatRate, 0xFF)
							|| !Write(Register::RangeVhvRecalibrate, 0x01)
							|| !Write(Register::InterruptConfigGpio, RANGE_INTERRUPT_READY)
							|| !Write(Register::InterruptClear, CLEAR_ALL_INTERRUPTS)) return Fail(ErrorEnum::I2c);
						int8_t factoryOffset = 0;
						if (!ReadFactoryOffset(factoryOffset)) return false;
						if (!Write(Register::CrosstalkRateHigh, static_cast<uint8_t>(CrosstalkCompensationRate >> 8))
							|| !Write(Register::CrosstalkRateLow, static_cast<uint8_t>(CrosstalkCompensationRate))) return Fail(ErrorEnum::I2c);
						if (UseCustomOffset && !Write(Register::PartToPartOffset, static_cast<uint8_t>(PartToPartOffsetMm))) return Fail(ErrorEnum::I2c);
						if (!Write(Register::FreshOutOfReset, 0x00)) return Fail(ErrorEnum::I2c);
						LastError = ErrorEnum::None;
						Running = true;
						WaitSinceMillis = millis();
						LastStartMillis = WaitSinceMillis - MeasurementPeriodMillis;
						return true;
					}

					/// <summary>
					/// Stops software sampling and invalidates the cache, without touching the shared
					/// bus or CE. An already-started finite single shot may finish in hardware.
					/// Start can be retried if that shot still holds the device busy.
					/// </summary>
					void Stop() final
					{
						Running = false;
						Pending = false;
						Available = false;
						DiagnosticsAvailable = false;
					}

					void Step() final
					{
						if (!Running) return;
						const uint32_t now = millis();
						uint8_t value = 0;
						if (!Read(Register::FreshOutOfReset, value)) { Fail(ErrorEnum::I2c); return; }
						if (value != 0) { Fail(ErrorEnum::UnexpectedReset); return; }
						if (!Pending)
						{
							if (static_cast<uint32_t>(now - LastStartMillis) < MeasurementPeriodMillis) return;
							if (!Read(Register::RangeStatus, value)) { Fail(ErrorEnum::I2c); return; }
							if ((value & RANGE_READY_MASK) == 0)
							{
								if (static_cast<uint32_t>(now - WaitSinceMillis) >= MeasurementTimeoutMillis)
									Fail(ErrorEnum::Timeout);
								return;
							}
							if (!Write(Register::RangeStart, START_SINGLE_SHOT)) { Fail(ErrorEnum::I2c); return; }
							Pending = true;
							LastStartMillis = millis();
							return;
						}
						if (static_cast<uint32_t>(now - LastStartMillis) >= MeasurementTimeoutMillis)
						{
							Fail(ErrorEnum::Timeout);
							return;
						}
						if (!Read(Register::InterruptStatusGpio, value)) { Fail(ErrorEnum::I2c); return; }
						if ((value & INTERRUPT_ERROR_MASK) != 0) { Fail(ErrorEnum::Interrupt); return; }
						if ((value & RANGE_INTERRUPT_MASK) != RANGE_INTERRUPT_READY)
						{
							return;
						}
						if (!Read(Register::RangeStatus, value)) { Fail(ErrorEnum::I2c); return; }
						RangeStatus = value >> RANGE_STATUS_SHIFT;
						Vl6180xDiagnostics diagnostics{};
						diagnostics.RangeStatus = RangeStatus;
						uint8_t rateHigh = 0, rateLow = 0;
						if (!Read(Register::RawRange, diagnostics.RawRangeMm) || !Read(Register::RangeValue, diagnostics.RangeValMm)
							|| !Read(Register::ReturnRateHigh, rateHigh) || !Read(Register::ReturnRateLow, rateLow)) { Fail(ErrorEnum::I2c); return; }
						diagnostics.ReturnRate = static_cast<uint16_t>((static_cast<uint16_t>(rateHigh) << 8) | rateLow);
						if (!Write(Register::InterruptClear, CLEAR_ALL_INTERRUPTS)) { Fail(ErrorEnum::I2c); return; }
						diagnostics.TimestampMicros = micros();
						Diagnostics = diagnostics;
						DiagnosticsAvailable = true;
						const uint8_t distance = diagnostics.RangeValMm;
						Pending = false;
						WaitSinceMillis = millis();
						if (RangeStatus != static_cast<uint8_t>(RangeStatusEnum::Valid))
						{
							ReportInvalidReading();
							return;
						}
						if (ConsecutiveValidReadings < ValidReadingRecoveryThreshold)
							++ConsecutiveValidReadings;
						if (ConsecutiveValidReadings >= ValidReadingRecoveryThreshold)
							ReportedHardwareFaults = 0;
						const uint16_t filterInput = distance;
						const uint32_t acquiredMillis = millis();
						if (!Available || static_cast<uint32_t>(acquiredMillis - SampleMillis)
							>= MeasurementTimeoutMillis + MeasurementPeriodMillis)
						{
							DistanceFilter.Clear(filterInput);
						}
						else
						{
							DistanceFilter.Set(filterInput);
							DistanceFilter.Step();
						}
						Sample.distance = distance;
						Sample.timestamp = diagnostics.TimestampMicros;
						Sample.quality = 255;
						Sample.ReturnRate = diagnostics.ReturnRate;
						SampleMillis = acquiredMillis;
						Available = true;
						LastError = ErrorEnum::None;
					}

					/// <summary>
					/// Non-consuming latest sample, timestamped in microseconds at host acquisition.
					/// Quality 255 means status-valid, not a measured confidence score. False leaves
					/// output unchanged before first valid result, after a stopping error/Stop, or
					/// at age >= timeout + period. Nonzero hardware statuses skip the acquisition.
					/// Pending and sensor-invalid measurements preserve the last unexpired valid
					/// result, including its timestamp; invalid results never refresh sample age.
					/// Distance is filtered when FilterFactor is nonzero; getters never advance it.
					/// </summary>
					bool GetData(Inertia::Model::timestamped_quality_range16_t& data) final
					{
						if (!GetRawData(data)) return false;
						data.distance = DistanceFilter.Get();
						return true;
					}

					/// <summary>Unfiltered sensor distance with the same validity and acquisition metadata as GetData.</summary>
					bool GetRawData(Inertia::Model::timestamped_quality_range16_t& data)
					{
						if (!IsSampleAvailable()) return false;
						data = Sample;
						return true;
					}

					/// <summary>Unfiltered valid range/rate from one shot; return rate is unsigned 9.7 Mcps.</summary>
					bool GetData(Inertia::Components::Distance::timestamped_signal_range16_t& data) final
					{
						if (!IsSampleAvailable()) return false;
						data = Sample;
						return true;
					}

					/// <summary>
					/// Latest completed shot, including rejected shots. Cached, non-consuming and
					/// bus-free; retained until the next completed shot or Stop/restart/fault.
					/// Check RangeStatus and TimestampMicros; this is not a valid-distance source.
					/// False leaves output unchanged when no snapshot is available.
					/// </summary>
					bool GetDiagnostics(Vl6180xDiagnostics& out) const
					{
						if (!DiagnosticsAvailable) return false;
						out = Diagnostics;
						return true;
					}

					/// <summary>
					/// Reads 0x0024 once, before this instance applies any override; cached thereafter,
					/// including across Stop/Start. It is the factory offset only if no earlier writer
					/// changed that register since power-up. Does not reload NVM or restore an offset.
					/// Read failures stop the driver and log I2c; output is unchanged on failure.
					/// </summary>
					bool ReadFactoryOffset(int8_t& offset)
					{
						if (!FactoryOffsetAvailable)
						{
							if (!IsAddressValid(Address) || ClockHz == 0 || ClockHz > 400000)
								return Fail(ErrorEnum::InvalidConfiguration);
							uint8_t value = 0;
							if (!Read(Register::PartToPartOffset, value)) return Fail(ErrorEnum::I2c);
							FactoryOffset = static_cast<int8_t>(value < 128 ? value : static_cast<int16_t>(value) - 256);
							FactoryOffsetAvailable = true;
						}
						offset = FactoryOffset;
						return true;
					}

					/// <summary>
					/// Reassigns a stopped unit's volatile 7-bit address via register 0x0212.
					/// On one shared bus hold ALL units in CE/GPIO0 shutdown, release ONE, wait for
					/// boot, construct at 0x29 and SetAddress to a free nonreserved address before
					/// releasing the next unit. Each unit needs independent CE; no automatic scan.
					/// Power/CE reset restores 0x29: reconstruct the instance at 0x29 and repeat.
					/// Failure may leave hardware at either address; use CE reset to recover safely.
					/// Separate buses can each use 0x29. The caller prevents address collisions.
					/// </summary>
					bool SetAddress(const uint8_t address)
					{
						if (Running) return Fail(ErrorEnum::AddressChangeWhileRunning);
						if (!IsAddressValid(address) || !IsAddressValid(Address) || ClockHz == 0 || ClockHz > 400000)
							return Fail(ErrorEnum::InvalidConfiguration);
						if (address == Address) return true;
						if (!Write(Register::I2cAddress, address)) return Fail(ErrorEnum::I2c);
						Address = address;
						uint8_t model = 0;
						if (!Read(Register::ModelId, model)) return Fail(ErrorEnum::I2c);
						if (model != MODEL_ID) return Fail(ErrorEnum::WrongModel);
						LastError = ErrorEnum::None;
						return true;
					}

					uint8_t GetAddress() const { return Address; }
					ErrorEnum GetLastError() const { return LastError; }
					/// <returns>RESULT_RANGE_STATUS bits 7:4, or 0xFF before any completed result.</returns>
					uint8_t GetRangeStatus() const { return RangeStatus; }
					bool IsRunning() const { return Running; }

				private:
					static bool IsAddressValid(const uint8_t address) { return address >= 0x08 && address <= 0x77; }

					bool IsSampleAvailable() const
					{
						return Running && Available && static_cast<uint32_t>(millis() - SampleMillis)
							< MeasurementTimeoutMillis + MeasurementPeriodMillis;
					}

					void ReportInvalidReading()
					{
						ConsecutiveValidReadings = 0;
						if (RangeStatus >= static_cast<uint8_t>(RangeStatusEnum::VcselContinuityTest)
							&& RangeStatus <= static_cast<uint8_t>(RangeStatusEnum::Pll2Lock))
						{
							const uint8_t faultBit = static_cast<uint8_t>(1U << (RangeStatus - 1));
							if ((ReportedHardwareFaults & faultBit) == 0 && LogListener != nullptr)
							{
								LogListener->OnLog(Inertia::Model::LogEntryStruct{
									LOG_TAG, InstanceId, Inertia::Model::LogTypeEnum::Error,
									static_cast<uint8_t>(ErrorEnum::SensorStatus), RangeStatus });
								ReportedHardwareFaults |= faultBit;
							}
						}
						LastError = ErrorEnum::SensorStatus;
					}

					void SetError(const ErrorEnum error, const uint8_t value = 0)
					{
						if (LastError != error && LogListener != nullptr)
						{
							LogListener->OnLog(Inertia::Model::LogEntryStruct{
								LOG_TAG, InstanceId, Inertia::Model::LogTypeEnum::Error, static_cast<uint8_t>(error), value });
						}
						LastError = error;
					}

					bool Fail(const ErrorEnum error)
					{
						Stop();
						SetError(error);
						return false;
					}

					void SetClock()
					{
						Inertia::Drivers::HardwareInterface::I2c::Drivers::SetClockIfSupported(WireInstance, ClockHz);
					}

					bool Write(const uint16_t reg, const uint8_t value)
					{
						SetClock();
						WireInstance.beginTransmission(Address);
						const bool queued = WireInstance.write(static_cast<uint8_t>(reg >> 8)) == 1
							&& WireInstance.write(static_cast<uint8_t>(reg)) == 1
							&& WireInstance.write(value) == 1;
						const uint8_t result = WireInstance.endTransmission();
						return queued && result == 0;
					}

					bool Read(const uint16_t reg, uint8_t& value)
					{
						SetClock();
						WireInstance.beginTransmission(Address);
						const bool queued = WireInstance.write(static_cast<uint8_t>(reg >> 8)) == 1
							&& WireInstance.write(static_cast<uint8_t>(reg)) == 1;
						const uint8_t result = WireInstance.endTransmission(!queued);
						if (!queued || result != 0) return false;
						SetClock();
						const size_t count = WireInstance.requestFrom(Address, static_cast<uint8_t>(1));
						if (count != 1 || WireInstance.available() != 1)
						{
							// Bound draining even if a broken backend keeps available() asserted.
							for (uint8_t i = 0; i < 32 && WireInstance.available(); ++i) WireInstance.read();
							return false;
						}
						const int read = WireInstance.read();
						if (read < 0) return false;
						value = static_cast<uint8_t>(read);
						return true;
					}
				};
			}
		}
	}
}

#endif