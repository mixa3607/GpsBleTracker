#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <freertos/queue.h>
#include <freertos/task.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Display.h>

struct Config
{
  uint16_t GpsQueueLen;
};

void InitSdCard(xQueueHandle displayMessageQueue, Stream *logger)
{
  const uint8_t sdCardDetectAttempts = 5;
  const uint8_t sdCardCsPin = 5;
  auto msg = new DisplayMessage;
  msg->MessageType = Text;
  msg->ScreenId = SdCard;
  for (uint8_t i = 0; i < sdCardDetectAttempts; i++)
  {
    auto progressMsg = new DisplayMessage(*msg);
    progressMsg->Value = (void *)("Det sd " + String(i)).c_str();
    xQueueOverwrite(displayMessageQueue, progressMsg);
    if (!SD.begin(sdCardCsPin))
    {
      logger->println("Sd card not detected " + String(i));
    }
    else
    {
      progressMsg = new DisplayMessage(*msg);
      progressMsg->Value = (void *)"Sd OK";
      xQueueOverwrite(displayMessageQueue, progressMsg);
      logger->println("Sd detected");
      return;
    }
  }
}

void setup(void)
{
  auto logger = &Serial1;
  logger->begin(115200);
  logger->println("Start setup");

  auto queue = CreateDisplayTask(logger).second;
  vTaskDelay(1000);
  auto *const msg = new DisplayMessage();
  msg->MessageType = Text;
  msg->ScreenId = SdCard;
  msg->Value = (void *)"aaaa";

  xQueueSend(queue, &msg, INT32_MAX);
  //auto v = new int(10);
  //xQueueSend(displTaskParams->MessagesQueue, &v, INT32_MAX);

  //InitSdCard(displTaskParams->MessagesQueue, logger);
  return;
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }
  if (SD.exists("/data.txt"))
  {
    Serial.println("File doens't exist");
  }

  File file = SD.open("/data.txt", FILE_APPEND);
  if (!file)
  {
    Serial.println("Creating file...");
  }
  else
  {
    Serial.println("File already exists");
  }
  file.println("test text");
  file.close();
}

struct gpsSdLoggerTaskParams
{
  const xQueueHandle gpsSnapshotQueue;
  char *const fileName;
  Stream *const logStream;
};

void gpsSdLoggerTask(void *parameter)
{
  auto params = (gpsSdLoggerTaskParams *)parameter;
  auto logger = params->logStream;
}

void loop()
{
}