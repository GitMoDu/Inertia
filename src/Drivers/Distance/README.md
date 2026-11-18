# Distance drivers

Device families keep their register maps, initialization sequences, hardware
statuses and hardware calibration support in their own namespaces. Drivers expose
the standard timestamped range interface; optional paired signal/range sources
support software compensation without joining independent diagnostic caches.

- `Vl6180x`: standard 1 mm ranging, model ID 0xB4. Shielding is a mounting and
  calibration concern, not a driver variant. Verify the chip identity of modules
  sold as VL6180 before treating them as compatible.
- VL53L0X requires a separate register/protocol implementation; none is provided
  here yet. A matching default I2C address does not imply compatibility.

No speculative common VL register base is used. Shared transport should be
extracted only when another implemented device demonstrates compatible needs.
Software empirical compensation belongs in `Components::Distance`; hardware
status decoding and calibration register programming stay in the device driver.
