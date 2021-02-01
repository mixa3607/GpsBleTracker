#pragma once
#include <Adafruit_SSD1306.h>
#include <Logger.h>
#include <Gps.h>

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

struct DisplayMessage
{
  ScreenMessageType MessageType;
  ScreenIdType ScreenId;
  void *Value;
};

struct DisplayTaskParams
{
  Logger *logger;
  xQueueHandle MessagesQueue;
  xQueueHandle gpsGeoQueue;
};

Adafruit_SSD1306 *CreateDisplay(Logger *logger)
{
  auto a = F("a");
  const uint8_t addr = 0x3C;
  //const uint8_t sdaPin = 21;
  //const uint8_t sclPin = 22;
  const uint8_t width = 128;
  const uint8_t height = 32;

  logger->Log("Init display on addr 0x3c and core " + String(xPortGetCoreID()));
  auto *display = new Adafruit_SSD1306(width, height, &Wire, -1); //sda=25; scl=26
  if (!display->begin(SSD1306_SWITCHCAPVCC, addr))
  {
    logger->Log("Display begin failed");
    delete display;
    return nullptr;
  }

  //display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setTextSize(3);
  display->println("OK");
  display->display();
  display->setTextSize(1);

  logger->Log("Display initialized");
  return display;
}

void DisplayTask(void *pvParameters)
{
  auto *params = static_cast<DisplayTaskParams *>(pvParameters);
  auto *logger = params->logger;
  auto msgQueue = params->MessagesQueue;

  logger->Log("Begin display task");
  auto *const display = CreateDisplay(logger);

  if (!display)
  {
    logger->Log("Display is null. Kill task");
    vTaskDelete(nullptr);
    return;
  }

  DisplayMessage *msg;
  while (true)
  {
    if (xQueueReceive(msgQueue, &msg, UINT32_MAX) == pdFALSE)
      continue;
    logger->Log("Receive new message with screen id: " + String(msg->ScreenId) + " and type: " + String(msg->MessageType));

    if (msg->MessageType == Text)
    {
      auto text = (char *)msg->Value;
      logger->Log("Receive text: " + String(text));

      display->clearDisplay();
      display->setCursor(0, 0);
      display->println(text);
      display->display();
    }
    else
    {
      logger->Log("Receive unknown message");
    }
  }

  vTaskDelete(nullptr);
}

std::pair<TaskHandle_t, QueueHandle_t> CreateDisplayTask(Stream *const logOut, const QueueHandle_t gpsGeoQueue)
{
  const int8_t coreId = 1;
  const int8_t prior = 5;
  const char *const taskName = "Display";
  auto *const logger = new Logger(logOut, taskName);
  logger->Log("Initialize msg queue");

  const int msgQueueLen = 50;
  QueueHandle_t msgQueue = xQueueCreate(msgQueueLen, sizeof(DisplayMessage *));
  if (!msgQueue)
  {
    logger->Log("Error initialize msg queue");
    return std::pair<TaskHandle_t, QueueHandle_t>(nullptr, nullptr);
  }
  logger->Log("Initialize msg queue ok");

  auto *params = new DisplayTaskParams();
  params->logger = logger;
  params->MessagesQueue = msgQueue;
  params->gpsGeoQueue = gpsGeoQueue;

  TaskHandle_t taskHandler = nullptr;
  if (xTaskCreatePinnedToCore(DisplayTask, taskName, 2000, params, prior, &taskHandler, coreId) == pdTRUE)
  {
    logger->Log("Task created");
    return std::pair<TaskHandle_t, QueueHandle_t>(taskHandler, msgQueue);
  }
  vQueueDelete(msgQueue);
  logger->Log("Task creation failure");
  return std::pair<TaskHandle_t, QueueHandle_t>(nullptr, nullptr);
}