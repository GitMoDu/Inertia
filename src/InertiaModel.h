#ifndef _INERTIA_MODEL_INCLUDE_h
#define _INERTIA_MODEL_INCLUDE_h

// Framework model include.
#include "Framework/Model.h"

// Log model and buffer task.
#include "Components/Log/Model.h"
#include "Components/Log/BufferTask.h"

// Task profiler model and sampling profiler task.
#include "Components/TaskProfiler/Model.h"
#include "Components/TaskProfiler/SamplingProfiler.h"

// Data Source model and observable.
#include "Components/Core/DataSource/Model.h"
#include "Components/Core/DataSource/MultiObservable.h"

// Lifecycle model and generic driver task.
#include "Components/Core/Lifecycle/Model.h"
#include "Components/Core/Lifecycle/VariadicDriverTask.h"

// Hardware interface models.
#include "Components/HardwareInterface/Serial/Model.h"
#include "Components/HardwareInterface/I2c/Model.h"
#include "Components/HardwareInterface/Spi/Model.h"

// Surface model include.
#include "Components/Surface/Model.h"
#include "Components/Surface/ValuePack.h"

// Timestamp source include.
#include "Components/Timestamp/Model.h"
#include "Components/Timestamp/MillisTimestampSource.h"

// Storage allocation model and allocator.
#include "Components/Storage/Allocation/Model.h"
#include "Components/Storage/Allocation/Allocator.h"

// FRAM storage model.
#include "Components/Storage/Fram/Model.h"

// LittleFS storage model.
#include "Components/Storage/LittleFs/Model.h"

// Boot counter model.
#include "Components/BootCounter/Model.h"

// UartInterface model.
#include "Components/UartInterface/Model.h"

// Link model.
#include "Components/Link/Model.h"

// AHRS model.
#include "Components/Ahrs/Model.h"

// Control Theory models.
#include "Components/Control/Pid/Model.h"

// PowerTrain actuator model.
#include "Components/PowerTrain/Model.h"

// Servo model.
#include "Components/PowerTrain/Servo/Model.h"

// PWM model
#include "Components/PowerTrain/Pwm/Model.h"


// Servo actuator model, default calibration, and calibrated driver.
#include "Components/PowerTrain/ServoActuator/Model.h"
#include "Components/PowerTrain/ServoActuator/DefaultCalibration.h"
#include "Components/PowerTrain/ServoActuator/CalibratedDriver.h"

// PWM actuator model.
#include "Components/PowerTrain/PwmActuator/Model.h"

// Xio AHRS model and template driver.
#include "Components/Ahrs/Xio/Model.h"
#include "Components/Ahrs/Xio/TemplateDriver.h"

// Reefwing AHRS model and template driver.
#include "Components/Ahrs/Reefwing/Model.h"
#include "Components/Ahrs/Reefwing/TemplateDriver.h"

// Storage model for MB85RC FRAM variants.
#include "Drivers/Storage/Fram/Mb85Rc/Model.h"

// MPU6050 IMU sensor model.
#include "Drivers/Imu/MPU6050/Model.h"

// MTF-01/02 Optical Flow and Range sensor model.
#include "Drivers/OpticalFlow/MTF0X/Model.h"

// Kinematic model and components.
#include "Components/Kinematic/Model.h"
#include "Components/Kinematic/PeriodicCalibratedFlow.h"
#include "Components/Kinematic/VelocityEstimator.h"
#include "Components/Kinematic/PeriodicAngleAwareAltitudeEstimator.h"


#endif
