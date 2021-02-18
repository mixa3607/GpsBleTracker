#pragma once

#include <stdint.h>
#include <FreeRTOS.h>

#define GPS_TASK_MOCK 1

namespace Gps
{
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

  bool Init();
  TaskHandle_t CreateTask();
  bool ReadMessage(GpsGeoInfo *const msgPtr, const uint32_t timeout);
}