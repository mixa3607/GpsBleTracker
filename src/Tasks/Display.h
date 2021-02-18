#pragma once

#include <Tasks/Gps.h>

namespace Display
{
    enum DisplayMessageType : char
    {
        Unknown = 0,
        GpsInfo = 1,
        EnableScreen = 20,
        DisableScreen = 21,
    };

    struct DisplayMessage
    {
        DisplayMessageType type;
        union
        {
            Gps::GpsGeoInfo geoInfo;
        } value = {};
    };

    bool WriteMessage(DisplayMessage *const msgPtr, const uint32_t timeout);
    bool Init();
    TaskHandle_t CreateTask();
} // namespace Display