#include <Config.h>
#include <Tasks/Display.h>
#include <Tasks/Gps.h>
#include <Tasks/Ble.h>

#include <FreeRTOS.h>

namespace Broker
{
    const char *const TAG = "Broker";

    void MessageBrokerTask(void *parameters)
    {
        ESP_LOGI(TAG, "Begin task");

        while (true)
        {
            Gps::GpsGeoInfo gpsFix;
            if (Gps::ReadMessage(&gpsFix, 0))
            {
                Display::DisplayMessage displMsg;
                displMsg.type = Display::GpsInfo;
                displMsg.value.geoInfo = gpsFix;

                Ble::BleMessage bleMsg;
                bleMsg.type = Ble::GpsInfo;
                bleMsg.value.geoInfo = gpsFix;

                if (Display::WriteMessage(&displMsg, 0) == errQUEUE_FULL)
                {
                    ESP_LOGW(TAG, "Display queue is full. Skip one geo fix");
                }
                Ble::WriteMessageOverflow(&bleMsg);
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    bool Init()
    {
        ESP_LOGI(TAG, "Init...");

        ESP_LOGI(TAG, "Initialized");
        return true;
    }

    TaskHandle_t CreateTask()
    {
        const int8_t prior = 5;
        const uint32_t stackSizeBytes = 1024 * 4;

        TaskHandle_t taskHandler = nullptr;
        if (xTaskCreate(MessageBrokerTask, TAG, stackSizeBytes, nullptr, prior, &taskHandler) == pdTRUE)
        {
            ESP_LOGI(TAG, "Task created");
            return taskHandler;
        }
        ESP_LOGE(TAG, "Task creation failure");
        return nullptr;
    }

} // namespace Broker