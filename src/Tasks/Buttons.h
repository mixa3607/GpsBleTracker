#pragma once

#include <FreeRTOS.h>

#include <Misc/ButtonTypes.h>

namespace Btns
{
    bool Init();
    TaskHandle_t CreateTask();

    bool ReadMessage(ButtonMessage *const msgPtr, const uint32_t timeout);
} // namespace Btns
