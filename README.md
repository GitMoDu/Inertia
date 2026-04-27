# Inertia

Inertia is an embedded C/C++ library for motion-oriented and robotics-oriented systems.

At the moment, this repository is in an active restructuring phase. Core concepts are already present, but the public layout, namespaces, module boundaries, and some APIs are still settling.

## Current Status

This README is intentionally temporary.

What is in the repository today is a mixture of:
- reusable data/model types
- framework/task utilities
- hardware-facing drivers
- motion/orientation-related components
- experiments and examples

Some parts are more stable than others, and some areas may still be renamed, moved, split, or removed.

## Repository Layout

Current top-level structure includes:

- `src/Framework` — scheduling/task-related framework code
- `src/Drivers` — device/sensor driver code
- `src/Components` — higher-level components such as AHRS-related functionality
- `src/InertiaModel.h` — central model/data declarations
- `src/InertiaDrivers.h` — driver-facing aggregation header
- `src/InertiaTaskInstrumentation.h` — task instrumentation hooks/utilities
- `Examples/` — example and testing sketches/projects

This structure should be treated as provisional.

## What Inertia Is Trying To Be

Inertia is evolving toward a modular foundation for embedded motion systems, including things like:

- sensor data acquisition
- orientation / inertial processing
- driver composition
- task scheduling and instrumentation
- reusable interfaces for robotics and vehicle-oriented projects

The exact packaging of those responsibilities is still being refined.

## Stability Notice

Please assume the following may change without much notice while the library structure settles:

- namespaces
- header names
- folder layout
- public interfaces
- component boundaries
- example organization

If you use the library early, expect some churn.

## Examples

There are examples in `Examples/`, including testing/profiling-oriented material.

They are useful as implementation references, but they should not yet be treated as final API documentation.
