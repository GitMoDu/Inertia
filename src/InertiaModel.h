#ifndef _INERTIA_MODEL_INCLUDE_h
#define _INERTIA_MODEL_INCLUDE_h

// Framework model include.
#include "Framework/Model.h"
#include "Framework/Interface.h"

// Task profiler model and timer drivers.
#include "Components/TaskProfiler/Model.h"
#include "Components/TaskProfiler/SamplingProfiler.h"
#include "Components/TaskProfiler/SamplingTimers/RpPicoSamplingTimer.h"
#include "Components/TaskProfiler/SamplingTimers/StmSamplingTimer.h"
#include "Components/TaskProfiler/SamplingTimers/AvrSamplingTimer.h"

// Generic driver task include.
#include "Components/Variadic/VariadicDriverTask.h"

// Observable include.
#include "Components/Observable/MultiObservable.h"

// Surface model include.
#include "Components/Surface/Model.h"
#include "Components/Surface/ValuePack.h"

// Timestamp source include.
#include "Components/Timestamp/MillisTimestampSource.h"

// Storage model and components.
#include "Components/Storage/Allocation/Model.h"
#include "Components/Storage/Allocation/Allocator.h"
#include "Components/Storage/Fram/Model.h"
#include "Components/Storage/Fram/CircularStore.h"
#include "Components/Storage/Fram/StructStore.h"
#include "Components/Storage/LittleFs/CircularStore.h"
#include "Components/Storage/LittleFs/StructStore.h"

// Storage model and drivers for LittleFS on SPIFFS or built in flash.
#include "Components/Storage/LittleFs/FileSystem.h"

// Log model and components.
#include "Components/Log/Model.h"
#include "Components/Log/BufferTask.h"
#include "Components/Log/Repository/LittleFsLogRepository.h"
#include "Components/Log/Repository/SerialOutRepository.h"
#include "Components/Log/Repository/FramLogRepository.h"

// Boot counter model and components.
#include "Components/BootCounter/Model.h"
#include "Components/BootCounter/Repository/NoBootCounterRepository.h"
#include "Components/BootCounter/Repository/LittleFsBootCounterRepository.h"

// UartInterface model.
#include "Components/UartInterface/Model.h"

// AHRS model.
#include "Components/Ahrs/Model.h"

// Reefwing AHRS model and template driver.
#include "Components/Ahrs/Reefwing/Model.h"
#include "Components/Ahrs/Reefwing/TemplateDriver.h"



#endif
