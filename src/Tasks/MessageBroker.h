#pragma once

#include <FreeRTOS.h>

namespace Broker
{
    bool Init();
    TaskHandle_t CreateTask();
} // namespace Broker