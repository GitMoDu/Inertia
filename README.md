# Inertia Framework

Embedded framework for robotic vehicles (quad-copters, rc cars, planes, etc...).

### Layered Architecture
| Layer | Responsibility |
|-------|----------------|
| Model | Fundamental data types, units, integer angle conversion, abstract interfaces (`ILifecycleDriver`, `IPeriodicDriver`, `IDataSource<T>`, driver task contracts) |
| Drivers | Concrete hardware or algorithmic data acquisition (IMU, optical flow, AHRS providers) implementing Model interfaces |
| Composition | Variadic driver task wrappers aggregating multiple `IDataSource<T>` views and scheduling periodic `Step()` calls |
| Scenes / Consumer Logic | Higher‑level modules consuming data |

### Core Data Types (Model)
All timestamped types include a `uint32_t timestamp` in microseconds (caller is responsible for consistency). Metrics use integer scaling for speed and portability.
- `vector16_t` / `vector32_t` base numeric 3D vectors
- `timestamped_vector16_t` / `timestamped_vector32_t` time‑stamped variants
- `timestamped_acceleration_t`: milli‑G units (1000 = 1 G)
- `timestamped_angular_velocity_t`: `angle_t` units per second (0.05° precision)
- `timestamped_magnet_t`: arbitrary units for magnetic flux density
- `temperature_t` / `timestamped_temperature_t`: centi‑Kelvin
- `range16_t` / `range32_t` and timestamped range structs: millimeters
- `flow_translation_t` / `timestamped_flow_translation_t`: planar displacement + quality
- `quaternion_t`, `euler_angle_t`, and timestamped orientation variants

### Angle System
`angle_t` spans `[0, ANGLE_RANGE)` mapping linearly onto `[0°, 360°)`. Helper:

angle_t a = Inertia::Model::GetIntegerAngle(headingDegrees);

Rounding and wrap normalization are handled internally.

### Driver Interfaces
- `ILifecycleDriver`: (`Start()`, `Stop()`). Basic driver lifecyle.
- `IPeriodicDriver`: (`Step()`) extends `ILifecycleDriver`. Periodic update driver with lifecycle.
- `IDataSource<T>`: (`GetData(T& out)` data retrieval interface, returns true when available.


### Variadic Driver Task Pattern
A variadic task orchestrates periodic `Step()` calls and exposes multiple data sources:
- Schedules `Step()` via `TaskScheduler`
- Allows upcast to each `IDataSource<T>` matching declared payload types
- Can be specialized using a published `TypeList` (partial specialization expands it)

### Publishing Type Packs
Drivers publish supported payload sets with:

using DataTypes = Variadic::TypeList<DataType1, DataType2>;

This enables drop‑in task declarations without repeating each type.

### Driver Contract
1. Define raw storage members for each output sample (timestamped struct)
2. Implement `Start()` to configure hardware; return false on failure
3. Implement `Step()` to populate cached samples atomically and set availability flag
4. Implement one `GetData()` overload per exported type
5. Publish `DataTypes` list for variadic task integration

### Adapting Existing Drivers (Type Remapping)
Create an adapter inheriting `IPeriodicDriver` + new `IDataSource<NewType>` specializations. Convert source structs inside `GetData()` and publish a new `TypeList`. Example pattern:

template<class Base>
class RemapAdapter : public IPeriodicDriver,
                     public IDataSource<MyAccel>, public IDataSource<MyGyro> { /* ... */ };

Then use `VariadicDriverTask<RemapAdapter<Base>, RemapAdapter<Base>::DataTypes>`.

### Scheduling Integration
`DriverStepTask` and `VariadicDriverTask` internally derive from `TS::Task` (OO callbacks enabled). Period is specified in milliseconds.

### Error Handling & Availability
`GetData()` returns `false` when no valid sample exists. Callers should guard reads and may choose to fall back or skip processing on failure.

### Performance Considerations
- Integer scaling avoids float math in steady‑state reads.
- Multiple inheritance for data sources has no runtime cost (compile‑time layout).

### License & Attribution
Hardware drivers may depend on vendor libraries (e.g., SparkFun LSM6DS3). Maintain upstream attribution in driver wrappers. Core Inertia code is intended for inclusion in embedded projects with minimal dependencies.

### Future Extensions
- Additional fusion drivers (magnetometer + accelerometer + gyro)
- Optional filtering stages with fixed‑point math

  
