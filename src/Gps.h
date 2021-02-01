#pragma once
#include <Logger.h>
#include <freertos/queue.h>
#include <TinyGPS++.h>

struct GpsGeoInfo
{
  GpsGeoInfo(){};
  GpsGeoInfo(uint16_t id, uint32_t date, uint32_t time,
             int32_t hdop, int32_t satellites,
             double longtitude, double latitude)
  {
    this->id = id;
    this->date = date;
    this->time = time;
    this->hdop = hdop;
    this->satellites = satellites;
    this->longtitude = longtitude;
    this->latitude = latitude;
  };
  uint16_t id;

  uint32_t date;
  uint32_t time;

  int32_t hdop;

  int32_t satellites;

  double longtitude;
  double latitude;
};

struct GpsTaskParams
{
  Logger *logger;
  xQueueHandle gpsGeoInfoQueue;
  HardwareSerial *gpsSerial;
  int32_t gpsSerialBaudrate = 9600;
};

void QueueSendOverflow(QueueHandle_t handle, GpsGeoInfo *value)
{
  if (xQueueSend(handle, value, 0) == errQUEUE_FULL)
  {
    GpsGeoInfo buf;
    xQueueReceive(handle, &buf, 0);
    QueueSendOverflow(handle, value);
  }
}

void GpsUpdate(QueueHandle_t queue, TinyGPSPlus *gps, Logger *logger, uint16_t updateId)
{
  if (gps->hdop.isUpdated() || gps->time.isUpdated() || gps->date.isUpdated() || gps->location.isUpdated() || gps->satellites.isUpdated())
  {
    logger->Log("Detect update in gps");
    auto geoInfo = GpsGeoInfo(updateId, gps->date.value(), gps->time.value(), gps->hdop.value(), gps->satellites.value(),
                              gps->location.lng(), gps->location.lat());
    
    uint8_t sec = (geoInfo.time / 100) % 100;
    uint8_t min = (geoInfo.time / 10000) % 100;
    uint8_t hour = (geoInfo.time / 1000000);
    uint16_t year = (geoInfo.date % 100) + 2000;
    uint8_t month = (geoInfo.date / 100) % 100;
    uint8_t day = (geoInfo.date / 10000);
    logger->Log("Id: " + String(geoInfo.id) +", Sa: " + String(geoInfo.satellites) + ", HDOP: " + String(geoInfo.hdop));
    logger->Log("Lat: " + String(geoInfo.latitude, 10) + ", Lng: " + String(geoInfo.longtitude, 10));
    logger->Log("DT: " + String(hour) + ":" + String(min) + ":" + String(sec) + " " + String(day) + "." + String(month) + "." + String(year));

    QueueSendOverflow(queue, &geoInfo);
  }
  else
  {
    //logger->Log("Nothing updated in gps");
  }
}

void GpsTask(void *parameters)
{
  auto *params = static_cast<GpsTaskParams *>(parameters);
  auto *logger = params->logger;
  auto geoQueue = params->gpsGeoInfoQueue;
  auto *gpsSerial = params->gpsSerial;

  logger->Log("Begin task");
  auto *gps = new TinyGPSPlus();
  gpsSerial->begin(params->gpsSerialBaudrate);
  logger->Log("Uart and gps initialized on baudrate " + String(params->gpsSerialBaudrate));

  for (uint16_t i = 0; true; i++)
  {
    while (gpsSerial->available() == 0)
      vTaskDelay(10);

    while (gpsSerial->available() > 0)
      if (gps->encode(gpsSerial->read()))
      {
        //displayInfo(gps);
        GpsUpdate(geoQueue, gps, logger, i);
      }
  }

  vTaskDelete(nullptr);
}




std::pair<TaskHandle_t, QueueHandle_t> CreateGpsTask(Stream *const logOut, const int queueLen = 100)
{
  const int8_t prior = 5;
  const char *const taskName = "Gps";
  auto *const logger = new Logger(logOut, taskName);
  auto geoInfoSize = sizeof(GpsGeoInfo);
  logger->Log("Initialize geo info queue with len " + String(queueLen) + "and elem size " + String(geoInfoSize));

  QueueHandle_t gpsQueue = xQueueCreate(queueLen, geoInfoSize);
  if (!gpsQueue)
  {
    logger->Log("Error initialize geo queue");
    return std::pair<TaskHandle_t, QueueHandle_t>(nullptr, nullptr);
  }
  logger->Log("Initialize geo queue ok");

  auto *params = new GpsTaskParams();
  params->logger = logger;
  params->gpsGeoInfoQueue = gpsQueue;
  params->gpsSerial = &Serial2; //tx2=17; rx2=16
  params->gpsSerialBaudrate = 230400;

  TaskHandle_t taskHandler = nullptr;
  if (xTaskCreate(GpsTask, taskName, 2000, params, prior, &taskHandler) == pdTRUE)
  {
    logger->Log("Task created");
    return std::pair<TaskHandle_t, QueueHandle_t>(taskHandler, gpsQueue);
  }
  vQueueDelete(gpsQueue);
  logger->Log("Task creation failure");
  return std::pair<TaskHandle_t, QueueHandle_t>(nullptr, nullptr);
}