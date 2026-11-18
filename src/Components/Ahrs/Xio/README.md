# Xio Fusion AHRS

## Configuration

`TemplateDriver` keeps the existing source/range/gain constructor. The other
constructor takes acceleration, gyroscope and optional magnetometer sources,
then `FusionAhrsSettings` and optional `FusionBiasSettings`, both copied by value.
Settings apply at `Start()`; restarting clears Fusion state and learned bias.

Only NWU convention is supported. `Start()` returns false for unsupported sources,
non-NWU convention, nonfinite/negative settings or overflowing recovery timers.
A gyroscope source is required; acceleration and magnetometer enable correction.
Configure nominal AHRS sample rate to match scheduling: recovery timeouts are
sample-count based. Actual integration periods come from gyro timestamps.
Bias sample rate is derived from each accepted period (minimum 1 Hz), overriding
`FusionBiasSettings.sampleRate`; stationary threshold and period are caller-owned.

Native settings also provide explicit diagnostic experiments: zero gyroscope
range disables overrange recovery, and zero rejection angles disable their
rejection tests. Gain controls feedback; Fusion still has its startup phase.
These settings change estimation, not just output. Test with actuators disabled.
Vehicle mounting trims and residual input calibration belong to the application.

## Diagnostics

`GetDiagnostics(diagnostics_t&)` returns the last completed update: exact source
inputs (including their own timestamps), fused Euler angles in degrees, learned
gyro bias in degrees/second, Fusion flags/internal states and the gyro integration
period in microseconds. Inputs are before Fusion bias correction. Unused sources
are zero. Bias is the estimate after the update, not necessarily the offset used
for that update's gyro correction.

The getter does not read sources or update Fusion. Derived diagnostics are
computed only on request, without a second persistent diagnostic buffer.
Missing sources and duplicate timestamps leave the previous snapshot unchanged;
availability is not a freshness guarantee. Inspect timestamps. Timestamp zero and
32-bit wrap are valid. Calls must be serialized with `Step()` and lifecycle calls.

Before the first completed update, after stop/restart, or after a failed start,
the getter returns false without modifying its output. `Step()` cannot revive a
stopped driver. There is no serial formatting, printing or debug transport in Xio.

Board tests (no physical IMU required), from the repository root:
`Examples\Testing\UnitTesting\Ahrs\XioTemplateDriverTest`.
