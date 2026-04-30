#ifndef _INERTIA_DRIVERS_INCLUDE_h
#define _INERTIA_DRIVERS_INCLUDE_h

// Framework model include.
#include "Framework/Model.h"
#include "Framework/Interface.h"

// Task profiler timer drivers. Platform-specific timers that implement the ISamplingTimer interface.
#include "Drivers/TaskProfiler/SamplingTimers/RpPicoSamplingTimer.h"
#include "Drivers/TaskProfiler/SamplingTimers/StmSamplingTimer.h"
#include "Drivers/TaskProfiler/SamplingTimers/AvrSamplingTimer.h"

// Hardware interface driver base and helpers.
#include "Drivers/HardwareInterface/Serial/SerialInterface.h"
#include "Drivers/HardwareInterface/I2c/I2cInterface.h"
#include "Drivers/HardwareInterface/Spi/SpiInterface.h"

// Storage driver for LittleFS on SPIFFS or built in flash. Ignored if platform does not support LittleFS.
#include "Drivers/Storage/LittleFs/FileSystem.h"

// Log repository drivers.
#include "Drivers/Log/Repository/LittleFsLogRepository.h"
#include "Drivers/Log/Repository/SerialOutRepository.h"
#include "Drivers/Log/Repository/FramLogRepository.h"

// Boot counter drivers.
#include "Drivers/BootCounter/Repository/NoBootCounterRepository.h"
#include "Drivers/BootCounter/Repository/LittleFsBootCounterRepository.h"

// VG6328A UART module.
#include "Drivers/Uart/VG6328A/DeviceDriver.h" 
#include "Drivers/Uart/VG6328A/StreamDriver.h"
#include "Drivers/Uart/VG6328A/WatchDogTask.h"

// LSM6DS3 IMU sensor.
#include "Drivers/Imu/LSM6DS3.h"

// QMI8658 IMU sensor.
#include "Drivers/Imu/QMI8658.h"

// MPU6050 IMU sensor driver.
#include "Drivers/Imu/MPU6050/DeviceDriver.h"
#include "Drivers/Imu/MPU6050/TemplateDriver.h"

// MTF-01/02 Optical Flow and Range sensor drivers.
#include "Drivers/OpticalFlow/MTF0X/DeviceDriver.h"
#include "Drivers/OpticalFlow/MTF0X/TaskDriver.h"

// Storage drivers for MB85RC FRAM variants.
#include "Drivers/Storage/Fram/Mb85Rc/Mb85Rc256v.h"

#endif
