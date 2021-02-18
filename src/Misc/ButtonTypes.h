#pragma once

#include <stdint.h>
#include <esp32-hal-gpio.h>

namespace Btns
{
    enum BtnType : uint8_t
    {
        BtnUnknown = 0,
        BtnCenter = 1,
        Btn1 = 1,
        Btn2 = 2,
        Btn3 = 3,
        Btn4 = 4,
        Btn5 = 5,
    };

    enum BtnEventType : uint8_t
    {
        UnknownClick = 0,
        ShortClick = 1,
        LongClick = 2,
    };

    struct ButtonMessage
    {
        BtnType type;
        BtnEventType eventType;
    };

    struct Button
    {
        Button(){};
        Button(BtnType type, uint8_t pin, uint8_t mode = INPUT_PULLUP, bool enable = LOW,
               uint16_t debounceMcs = 2000, uint16_t shortClickMs = 100,
               uint16_t longClickMs = 500) : type(type), pin(pin), mode(mode),
                                             enable(enable), debounceMcs(debounceMcs),
                                             shortClickMs(shortClickMs), longClickMs(longClickMs)

        {
        }
        
        BtnType type;
        uint8_t pin;
        uint8_t mode;
        bool enable;

        uint16_t debounceMcs = 1 * 1000; //2ms
        uint16_t shortClickMs = 50;
        uint16_t longClickMs = 500;

        uint64_t lastUpdateMcs = 0;
        bool lastState = 0;
    };
} // namespace Btns