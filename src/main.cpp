#include <Arduino.h>
#include <gpsSnapshot.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <freertos/queue.h>
#include <freertos/task.h>
#include <SSD1306.h>

enum ScreenMessageType : uint8_t
{
  Text = 1,
  GpsSnapshot = 2,
  ChangeScreen = 3
};

enum ScreenIdType : uint8_t
{
  GpsMain = 0,
  SdCard = 1,
};

struct Config
{
  uint16_t GpsQueueLen;
};

struct DisplayMessage
{
  ScreenMessageType MessageType;
  ScreenIdType ScreenId;
  void *Value;
};

struct DisplayTaskParams
{
  Stream *Logger;
  xQueueHandle MessagesQueue;
  SSD1306 *Display;
};

SSD1306 *InitDisplay()
{
  const uint8_t addr = 0x3C;
  const uint8_t sdaPin = 21;
  const uint8_t sclPin = 22;
  const OLEDDISPLAY_GEOMETRY geometry = GEOMETRY_128_32;

  auto display = new SSD1306Wire(addr, sdaPin, sclPin, geometry);
  display->init();
  display->setFont(ArialMT_Plain_16);
  display->drawString(0, 0, "OK");
  display->display();

  return display;
}
void displayTask(void *parameter)
{
  auto params = (DisplayTaskParams *)parameter;
  auto msgQueue = params->MessagesQueue;
  auto display = params->Display;
  auto logger = params->Logger;

  logger->println("Begin display task");

  DisplayMessage *msg;
  auto i = 0;
  while (true)
  {
    i++;
    display->clear();
    vTaskDelay(100);
    display->drawString(0, 0, String(i));
    display->display();
    vTaskDelay(300);
  }

  //int *v;
  while (true)
  {
    if (xQueueReceive(msgQueue, &msg, UINT32_MAX) == pdFALSE)
      //if (xQueueReceive(msgQueue, &v, UINT32_MAX) == pdFALSE)
      continue;

    //logger->println(*v);
    //continue;
    if (msg->MessageType == Text)
    {
      auto text = (char *)msg->Value;
      logger->println(text);

      //display->println(text);
      //display->display();
    }
  }

  vTaskDelete(nullptr);
}

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

  DisplayTaskParams *displTaskParams = new DisplayTaskParams();
  displTaskParams->Logger = logger;
  displTaskParams->MessagesQueue = xQueueCreate(10, sizeof(DisplayMessage *));
  displTaskParams->Display = InitDisplay();
  xTaskCreate(displayTask, "Display task", 2000, displTaskParams, tskIDLE_PRIORITY, nullptr);

  auto msg = new DisplayMessage;
  msg->MessageType = Text;
  msg->ScreenId = SdCard;
  msg->Value = (void *)"aaaa";

  xQueueSend(displTaskParams->MessagesQueue, &msg, INT32_MAX);
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