# VL6180X driver

Nonblocking single-shot lifecycle over a caller-owned I2C bus, with optional
source filtering and hardware offset/crosstalk settings. Invalid results never
update the sample/filter; the previous measurement expires normally. Cached
diagnostics retain the latest completed result, including rejections, without
another bus read.

Acquisition statuses 6..15 are diagnostics, not event logs. VCSEL/PLL statuses
1..5 log one Error per distinct status until five consecutive valid acquisitions
or restart. Transport, timeout, reset and setup failures retain Error logging.
No periodic rejection reminders or recovery events are emitted.

## File boundaries

- `Model.h`: pure status/error codes, masks and diagnostic types; no Wire or lifecycle dependency.
- `Registers.h`: named device register addresses.
- `Initialization.h`: ordered mandatory ST AN4545 SR03 register values.
- `Calibration.h`: pure `CalculateCrosstalkCompensation()` calculation in unsigned
  9.7 Mcps; programming the result remains explicit hardware configuration.
- `Driver.h`: I2C acquisition, lifecycle, hardware configuration, cache and logging.

Use `<InertiaModel.h>` for model/component types and `<InertiaDrivers.h>` for the
driver and hardware calibration helper. The caller owns bus setup, timeouts,
CE pins and address isolation. Address assignment is volatile. Model ID must be
0xB4; VL53L0X is not handled by this protocol.

`GetData(timestamped_signal_range16_t&)` returns the unfiltered valid distance
and return rate from one shot, with the same timestamp as that shot's diagnostics.
Its return rate is unsigned 9.7 Mcps (128 = 1 Mcps). Invalid shots update diagnostic
register values but never this accepted pair. The pair expires alongside the
normal sample and is withdrawn on stop or a stopping error. False leaves output
unchanged. Normal `GetData(timestamped_quality_range16_t&)` retains optional
per-shot filtering; `GetRawData()` remains software-unfiltered, hardware-compensated.

Mounting dimensions and empirical coefficients belong to the application.
The old empirical boolean and driver-class calibration helpers have been removed.
Use `Components::Distance::PeriodicCalibratedRange` on the paired source when an
application has a measured fit; it corrects before its own optional filter.
Never derive a fit from invalid shots or reuse a pre-remount vehicle calibration.

Tests run as Arduino sketches on real boards. Paths from the repository root:

- Fake-bus checks: `Examples\Testing\UnitTesting\Distance\Vl6180xDriverTest`.
- Hardware diagnostic: `Examples\Testing\DriverTesting\Vl6180xRange`.
  Connect the sensor to the board's default Wire SDA/SCL pins. The sketch does
  not configure vehicle peripherals or motor outputs.
