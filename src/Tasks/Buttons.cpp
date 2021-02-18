#include <Tasks/Buttons.h>
#include <Config.h>

#include <FreeRTOS.h>

#include <pins_arduino.h>
#include <esp32-hal-gpio.h>

namespace Btns
{
    const char *const TAG = "Btns";

    //buttons
    const uint8_t ButtonsCount = sizeof(ButtonsInfo) / sizeof(*ButtonsInfo);
    Btns::Button Buttons[ButtonsCount] = {};

    const int btnMsgQueueLen = 5;
    QueueHandle_t btnMsgQueue = nullptr;

    bool ReadMessage(ButtonMessage *const msgPtr, const uint32_t timeout)
    {
        return xQueueReceive(btnMsgQueue, msgPtr, timeout);
    }

    bool WriteMessage(ButtonMessage *const msgPtr, const uint32_t timeout)
    {
        return xQueueSend(btnMsgQueue, msgPtr, timeout);
    }

    bool IRAM_ATTR WriteMessageFromISR(ButtonMessage *const msgPtr)
    {
        return xQueueSendFromISR(btnMsgQueue, msgPtr, nullptr);
    }

    struct event
    {
        uint8_t pin;
        uint8_t state;
        uint64_t dur;
    };

    QueueHandle_t testQueue = nullptr;
    void IRAM_ATTR ButtonISR(void *param)
    {
        Button *btn = static_cast<Button *>(param);

        uint64_t currentMcs = micros();

        uint64_t stateDurationMcs = currentMcs - btn->lastUpdateMcs;
        bool currentState = digitalRead(btn->pin);
        bool lastState = btn->lastState;
        char str[] = "string";

        btn->lastUpdateMcs = currentMcs;
        if (btn->lastState == currentState)
        {
            return;
        }

        btn->lastState = currentState;
        if (stateDurationMcs > btn->debounceMcs)
        {

            event ev = {btn->pin, btn->lastState, stateDurationMcs};
            xQueueSendFromISR(testQueue, &ev, nullptr);
            if (currentState == btn->enable)
            {
                int64_t clickDurationMs = currentMcs - btn->lastUpdateMcs / 1000;
                if (clickDurationMs > btn->longClickMs)
                {
                    //long
                }
                else if (clickDurationMs > btn->shortClickMs)
                {
                    //short
                }
                else
                {
                    //shortest
                }
            }
            else if (btn->lastState != btn->enable && currentState == btn->enable)
            {
                /* code */
            }
        }
    }

    void ButtonsTask(void *parameters)
    {
        ESP_LOGV(TAG, "start task");
        testQueue = xQueueCreate(50, sizeof(event));

        while (true)
        {
            event tev;
            if (xQueueReceive(testQueue, &tev, portMAX_DELAY) == pdTRUE)
            {
                ESP_LOGV(TAG, "pin %d val %d dur %llu", tev.pin, tev.state, tev.dur);
            }
        }

        vTaskDelete(nullptr);
    }

    bool Init()
    {
        ESP_LOGI(TAG, "Init...");

        ESP_LOGI(TAG, "Init pins...");
        for (size_t i = 0; i < ButtonsCount; i++)
        {
            Buttons[i] = ButtonsInfo[i];
            Button *btn = &Buttons[i];
            pinMode(btn->pin, INPUT_PULLUP);
            attachInterruptArg(digitalPinToInterrupt(btn->pin), ButtonISR, btn, CHANGE);
            ESP_LOGI(TAG, "Pin %d attached", btn->pin);
        }

        ESP_LOGI(TAG, "Create msg queue...");
        btnMsgQueue = xQueueCreate(btnMsgQueueLen, sizeof(ButtonMessage));
        if (btnMsgQueue == nullptr)
        {
            ESP_LOGE(TAG, "Queue allocation failure");
            return false;
        }

        ESP_LOGI(TAG, "Initialized");
        return true;
    }

    TaskHandle_t CreateTask()
    {
        ESP_LOGI(TAG, "create task");
        xTaskCreate(ButtonsTask, "no", 2048, nullptr, 5, nullptr);
        return nullptr;
    }

} // namespace Btns
