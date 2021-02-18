#include <Config.h>
#include <Tasks/Display.h>
#include <Tasks/Gps.h>

#include <FreeRTOS.h>

#include <Adafruit_SSD1306.h>

namespace Display
{
    const char *const TAG = "Display";

    QueueHandle_t displayMsgQueue = nullptr;
    Adafruit_SSD1306 display;

    bool ReadMessage(DisplayMessage *const msgPtr, const uint32_t timeout)
    {
        return xQueueReceive(displayMsgQueue, msgPtr, timeout);
    }

    bool WriteMessage(DisplayMessage *const msgPtr, const uint32_t timeout)
    {
        return xQueueSend(displayMsgQueue, msgPtr, timeout);
    }

    void WriteGeoInfoPage(Gps::GpsGeoInfo *const gpsInfo)
    {
        uint8_t sec = (gpsInfo->time / 100) % 100;
        uint8_t min = (gpsInfo->time / 10000) % 100;
        uint8_t hour = (gpsInfo->time / 1000000);
        uint16_t year = (gpsInfo->date % 100) + 2000;
        uint8_t month = (gpsInfo->date / 100) % 100;
        uint8_t day = (gpsInfo->date / 10000);
        double hdop = static_cast<double>(gpsInfo->hdop) / 100;

        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("%d:%d:%d %d.%d.%d\n", hour, min, sec, day, month, year);
        display.printf("Lat: %e\n", gpsInfo->latitude);
        display.printf("Lng: %e\n", gpsInfo->longtitude);
        display.printf("Sa:%d, HD:%e %d\n", gpsInfo->satellites, hdop, gpsInfo->id);
        display.display();
    }

    bool InitDisplay()
    {
        ESP_LOGI(TAG, "Init display on core %d", xPortGetCoreID());
        display = Adafruit_SSD1306(DisplayWidth, DisplayHeight, DisplayWire, -1);
        if (!display.begin(SSD1306_SWITCHCAPVCC, DisplayAddress))
        {
            ESP_LOGE(TAG, "Display begin failed");
            return false;
        }

        display.setRotation(DisplayRotate);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(3);
        display.println("OK");
        display.display();
        display.setTextSize(1);

        ESP_LOGI(TAG, "Display initialized");
        return true;
    }

    bool Init()
    {
        ESP_LOGI(TAG, "Init...");

        ESP_LOGI(TAG, "Init display...");
        if (!InitDisplay())
        {
            return false;
        }

        ESP_LOGI(TAG, "Create msg queue...");
        displayMsgQueue = xQueueCreate(DisplayMsgQueueLen, sizeof(DisplayMessage));
        if (displayMsgQueue == nullptr)
        {
            ESP_LOGE(TAG, "Queue allocation failure");
            return false;
        }

        ESP_LOGI(TAG, "Initialized");
        return true;
    }

    void DisplayTask(void *pvParameters)
    {
        ESP_LOGI(TAG, "Begin display task");

        DisplayMessage message;
        while (true)
        {
            if (ReadMessage(&message, INT32_MAX) == pdFALSE)
                continue;
            ESP_LOGD(TAG, "Receive new message");
            switch (message.type)
            {
            case GpsInfo:
                WriteGeoInfoPage(&message.value.geoInfo);
                break;

            default:
                break;
            }
        }

        vTaskDelete(nullptr);
    }

    TaskHandle_t CreateTask()
    {
        const int8_t coreId = 1;
        const int8_t prior = 5;
        const uint32_t stackSizeBytes = 1024 * 3;

        TaskHandle_t taskHandler = nullptr;
        if (xTaskCreatePinnedToCore(DisplayTask, TAG, stackSizeBytes, nullptr, prior, &taskHandler, coreId) == pdTRUE)
        {
            ESP_LOGI(TAG, "Task created");
            return taskHandler;
        }
        ESP_LOGE(TAG, "Task creation failure");
        return nullptr;
    }
} // namespace Display