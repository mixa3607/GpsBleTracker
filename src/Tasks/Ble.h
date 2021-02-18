#pragma once

#include <Tasks/Gps.h>

namespace Ble
{
    enum BleMessageType : char
    {
        Unknown = 0,
        GpsInfo = 1,
    };

    struct BleMessage
    {
        BleMessageType type;
        union
        {
            Gps::GpsGeoInfo geoInfo;
        } value = {};
    };

    void WriteMessageOverflow(BleMessage *const msgPtr); //, const uint32_t timeout);
    bool Init();
    TaskHandle_t CreateTask();
} // namespace Ble
