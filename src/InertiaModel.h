#ifndef _INERTIA_MODEL_INCLUDE_h
#define _INERTIA_MODEL_INCLUDE_h

// Framework model include.
#include "Framework/Model.h"
#include "Framework/Interface.h"

// Generic driver task include.
#include "Components/Variadic/VariadicDriverTask.h"

// Observable include.
#include "Components/Observable/MultiObservable.h"

// Surface model include.
#include "Components/Surface/Model.h"
#include "Components/Surface/ValuePack.h"

// Timestamp source include.
#include "Components/Timestamp/MillisTimestampSource.h"

// Log model and components.
#include "Components/Log/Model.h"
#include "Components/Log/BufferTask.h"
#include "Components/Log/Repository/LittleFsLogRepository.h"
#include "Components/Log/Repository/SerialOutRepository.h"

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

// Hardware interface models.
#include "Drivers/I2c/Model.h" 
#include "Drivers/Spi/Model.h" 
#include "Drivers/Serial/Model.h" 


#endif
