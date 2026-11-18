# Distance components

## Paired samples

`timestamped_signal_range16_t` extends the normal timestamped/quality range with
`ReturnRate`. Distance and rate must describe the same accepted acquisition.
Rate is an unsigned source-native count, not a universal physical unit. Each
source documents its own scale. Do not copy calibration coefficients between
different scales, sensors or mountings.

## PeriodicCalibratedRange

A hardware-independent software data-source adapter. It accepts any
`IDataSource<timestamped_signal_range16_t>`; no particular sensor driver is
required. It implements one specific empirical model: a capped, additive distance
correction based on return rate. It is not a general-purpose calibration model;
use it only where measurements support that relationship.

Its constructor requires a paired source, copied `return_rate_calibration_t`, and
maximum sample gap in microseconds. The application starts and steps the source
first, then this component. It does not own or start/stop its source.

For rates above `Threshold`, add the nearest integer to
`(rate - Threshold) / RatePerMillimetre`, capped at `MaxCorrectionMillimetres`.
The final range saturates at UINT16_MAX. No measured coefficients are built in.
A zero correction limit bypasses compensation. `Start()` explicitly returns
false for a zero divisor or a zero/greater-than-INT32_MAX maximum sample gap.

Correction precedes the optional per-acquisition low-pass filter (template
factor, alpha = 1 / 2^factor). The paired source must be unfiltered. Duplicate
timestamps never advance the filter. First samples, unavailable-source recovery
and gaps at least the configured maximum reseed it. Timestamp wrap is supported;
source sessions must be accompanied by component restart to reset filter history.

Output preserves the source timestamp and quality. Getters do not advance the
filter. They check source availability and matching timestamp, so an expired or
unprocessed source sample cannot be presented as a current corrected result.
False leaves output unchanged. Source getters must be cached and non-consuming;
serialize source and component calls. Rejected shots must not replace the paired
sample, even if they update a separate diagnostic snapshot.

Board tests: `Examples\Testing\UnitTesting\Distance\CalibratedRangeTest` from the
repository root, with sensor-independent boundary and lifecycle checks.
