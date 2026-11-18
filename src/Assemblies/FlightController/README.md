# Flight controllers

## Initial gains

`QuadController` requires a `quad_pid_gains_t` by value as its fourth constructor
argument. `QuadAltitudeHoldController` requires it as its fifth argument.
Controllers own their initial gain copy; applications may replace or edit public
`QuadGains` between controller steps. Supply gains for pitch, roll, and yaw;
there are no built-in operational quad gains.

`QuadAltitudeHoldController` also requires an `altitude_hold_pid_gains_t` by value
as its sixth argument. It owns this copy and supports runtime changes through
`SetAltitudeGain()` / `GetAltitudeGain()`. Only altitude `Kp` is implemented;
`Ki` and `Kd` are unused. No built-in altitude gains are supplied.
