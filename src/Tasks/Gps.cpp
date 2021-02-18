#include <Tasks/Gps.h>
#include <Config.h>
#include <FreeRTOS.h>
#include <freertos/queue.h>
#include <TinyGPS++.h>

namespace Gps
{
  const char *const TAG = "Gps";

  QueueHandle_t gpsFixQueue = nullptr;
  TinyGPSPlus gps;

  void LogGpsFix(const GpsGeoInfo *const fix)
  {
    uint8_t sec = (fix->time / 100) % 100;
    uint8_t min = (fix->time / 10000) % 100;
    uint8_t hour = (fix->time / 1000000);
    uint16_t year = (fix->date % 100) + 2000;
    uint8_t month = (fix->date / 100) % 100;
    uint8_t day = (fix->date / 10000);
    double hdop = static_cast<double>(fix->hdop) / 100;
    millis();
    ESP_LOGV(TAG, "Id: %d, Sa: %d, HDOP: %e", fix->id, fix->satellites, hdop);
    ESP_LOGV(TAG, "Lat: %e, Lng: %e", fix->latitude, fix->longtitude);
    ESP_LOGV(TAG, "DT: %d:%d:%d %d.%d.%d", hour, min, sec, day, month, year);
  }

  bool Init()
  {
    ESP_LOGI(TAG, "Init...");

    const uint32_t geoInfoSize = sizeof(GpsGeoInfo);
    ESP_LOGI(TAG, "Initialize geo info queue with len %d and elem size %d on core %d", GpsFixQueueLen, geoInfoSize, xPortGetCoreID());
    gpsFixQueue = xQueueCreate(GpsFixQueueLen, geoInfoSize);
    if (!gpsFixQueue)
    {
      ESP_LOGE(TAG, "Error initialize geo queue");
      return false;
    }
    ESP_LOGI(TAG, "Initialize geo queue ok");

    gps = TinyGPSPlus();
    GpsSerial->begin(GpsSerialBaudrate);
    ESP_LOGI(TAG, "Uart and gps initialized on baudrate %d", GpsSerialBaudrate);

    return true;
  }

  bool ReadMessage(GpsGeoInfo *const msgPtr, const uint32_t timeout)
  {
    return xQueueReceive(gpsFixQueue, msgPtr, timeout);
  }

  void WriteMessageOverflow(const GpsGeoInfo *const value)
  {
    LogGpsFix(value);
    if (xQueueSend(gpsFixQueue, value, 0) == errQUEUE_FULL)
    {
      ESP_LOGV(TAG, "Geo queue is overflow. Try rewrite");
      GpsGeoInfo buf;
      xQueueReceive(gpsFixQueue, &buf, 0);
      if (xQueueSend(gpsFixQueue, value, 0) == errQUEUE_FULL)
      {
        ESP_LOGW(TAG, "Failure rewrite value in geo queue");
      }
    }
  }

  bool GpsIsUpdated()
  {
    //GGA is end of epoch
    return gps.satellites.isUpdated();

    //will be triggered to RMC and GGA
    //return gps.hdop.isUpdated() || gps.time.isUpdated() || gps.date.isUpdated() || gps.location.isUpdated() || gps.satellites.isUpdated()
  }

  bool FixGps(uint16_t updateId)
  {
    if (GpsIsUpdated())
    {
      ESP_LOGV(TAG, "Detect update in gps");
      const GpsGeoInfo geoInfo = GpsGeoInfo(updateId,
                                            gps.date.isValid() ? gps.date.value() : 0,
                                            gps.time.isValid() ? gps.time.value() : 0,
                                            gps.hdop.value(),
                                            gps.satellites.value(),
                                            gps.location.lng(),
                                            gps.location.lat());

      WriteMessageOverflow(&geoInfo);
      return true;
    }

    return false;
  }

#ifdef GPS_TASK_MOCK
  void GpsTask(void *parameters)
  {
    ESP_LOGI(TAG, "Begin task on core %d", xPortGetCoreID());
    uint16_t gpsFixId = 0;
    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
      const GpsGeoInfo geoInfo = GpsGeoInfo(gpsFixId++, gpsFixId++, gpsFixId++,
                                            gpsFixId++, gpsFixId++, gpsFixId++, gpsFixId++);
      WriteMessageOverflow(&geoInfo);
    }

    vTaskDelete(nullptr);
  }
#else
  void GpsTask(void *parameters)
  {
    ESP_LOGI(TAG, "Begin task on core %d", xPortGetCoreID());
    uint16_t gpsFixId = 0;
    while (true)
    {
      while (gpsSerial->available() == 0)
        vTaskDelay(pdMS_TO_TICKS(1));

      while (gpsSerial->available() > 0)
      {
        if (gps.encode(gpsSerial->read()))
        {
          gpsFixId += FixGps(gpsFixId);
        }
      }
    }

    vTaskDelete(nullptr);
  }
#endif

  TaskHandle_t CreateTask()
  {
    const int8_t prior = 5;
    const uint32_t stackSizeBytes = 1024 * 3;

    TaskHandle_t taskHandler = nullptr;
    if (xTaskCreate(GpsTask, TAG, stackSizeBytes, nullptr, prior, &taskHandler) == pdPASS)
    {
      ESP_LOGI(TAG, "Task created");
      return taskHandler;
    }
    ESP_LOGE(TAG, "Task creation failure");
    return nullptr;
  }
} // namespace Gps