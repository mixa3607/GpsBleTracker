#include <Arduino.h>

#include <Tasks/Gps.h>
#include <Tasks/Display.h>
#include <Tasks/Ble.h>
#include <Tasks/MessageBroker.h>
#include <Tasks/Buttons.h>

const char *const TAG = "Main";
void setup(void)
{
    Serial.begin(921600);
    ESP_LOGE(TAG, "err");
    ESP_LOGW(TAG, "warn");
    ESP_LOGI(TAG, "info");
    ESP_LOGD(TAG, "debug");
    ESP_LOGV(TAG, "verbose");

    ESP_LOGI(TAG, "Start setup");

    //Btns::Init();
    //Btns::CreateTask();

    Ble::Init();
    Gps::Init();
    Display::Init();
    Broker::Init();

    Display::CreateTask();
    Gps::CreateTask();
    Broker::CreateTask();

    ESP_LOGI(TAG, "Finished");
}

void loop()
{
    vTaskDelete(nullptr);
}
