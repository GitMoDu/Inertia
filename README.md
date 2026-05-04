# Inertia

Inertia is a modular embedded C/C++ library for motion-oriented, robotics-oriented, and device-control systems.

It provides reusable models, drivers, components, and assemblies for building embedded applications with sensor integration, control logic, storage, communication, logging, and task instrumentation.

## Status

The library is stable enough to document and build against, although some APIs and module boundaries may continue to evolve over time.

## What Inertia Is For

Inertia is intended to serve as a reusable embedded foundation for projects that need:

- Motion And Orientation Processing
- Sensor Acquisition
- Serial, I2C, And SPI Integration
- Storage And Persistent Repositories
- Logging And Profiling
- Control And Actuation
- Reusable Robotics- Or Vehicle-Oriented Building Blocks

## Repository Layout

- `src/InertiaModel.h` — Main include for shared models, components, and assemblies, with no hardware dependencies.
- `src/InertiaDrivers.h` — Main include for concrete hardware and platform drivers.
- `src/InertiaTaskInstrumentation.h` — Task instrumentation hooks and integration helpers.
- `src/Components` — Reusable application-level and domain-level building blocks.
- `src/Drivers` — Hardware-specific and platform-specific drivers.
- `src/Assemblies` — Higher-level systems composed from components and drivers.
- `Examples` — Example, testing, and profiling-oriented projects.

## Included Feature Areas

### Motion And Orientation
- AHRS-Related Components
- Kinematic Estimators
- IMU Driver Integrations
- Orientation And Motion Support Utilities

### Communication And Hardware Interfaces
- UART Support
- Serial, I2C, And SPI Interface Abstractions
- Link And Transport-Oriented Models

### Storage And Persistence
- FRAM Support
- LittleFS-Based Storage Support
- Repositories For Logs, Boot Counters, PWM, And Servo Calibration Data

### Control And Actuation
- PID And Control-Related Models
- PowerTrain Abstractions
- Servo And PWM Actuator Models
- Calibrated Driver Helpers

### Logging And Observability
- Log Models And Repositories
- Task Profiling
- Sampling Profiler Support
- Task Instrumentation Hooks

## Examples

The repository includes example material under `Examples/`, including:

- `TaskProfilerExample` — Example for task profiling.
- `Testing` — Integration and unit test examples.

These examples are useful as integration references and validation material, especially around profiling and driver interaction.

## Usage

For broad access to logic-only shared models and components:

```cpp
#include <InertiaModel.h>
```

For hardware-dependent and platform driver access:

```cpp
#include <InertiaDrivers.h>
```
