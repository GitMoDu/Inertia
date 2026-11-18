# Kinematic components

## PeriodicAngleAwareAltitudeEstimator

Assumes a body-down range beam and locally level ground. `SetSensorOffset()`
specifies the sensor optical origin relative to the clearance reference in
millimetres: x forward, y left, z up. The default zero offset preserves the
original sensor-origin geometry. With right-handed ZYX body-to-world Euler
angles, reference clearance is:

```text
h = (range - z) * cos(pitch) * cos(roll)
	+ x * sin(pitch) - y * cos(pitch) * sin(roll)
```

Yaw does not affect vertical projection. Distance limits clamp this compensated
clearance before base-distance subtraction; do not subtract mounting height a
second time through the base distance. Both raw and filtered outputs carry the
accepted range sample's timestamp and quality. Filter factor zero bypasses
smoothing. Sensor mounting dimensions remain application configuration.
Each 16-bit range/offset component is fraction-scaled before summation in 32 bits,
avoiding 64-bit intermediates. Component-wise integer rounding can differ from
a single final rounding by a few millimetres.

Board regression sketch (no physical sensor required), from the repository root:
`Examples\Testing\UnitTesting\Kinematic\AngleAwareAltitudeEstimatorTest`.
