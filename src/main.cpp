#include <Arduino.h>


#include <Display.h>
#include <Gps.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct Config
{
  uint16_t GpsQueueLen;
};

void setup(void)
{
  auto *logOut = &Serial;
  logOut->begin(921600);
  logOut->println("Start setup");

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  auto gpsTaskResult = CreateGpsTask(logOut);
  auto queue = CreateDisplayTask(logOut, gpsTaskResult.second).second;
  vTaskDelay(1000);
  auto *const msg = new DisplayMessage();
  msg->MessageType = Text;
  msg->ScreenId = SdCard;
  msg->Value = (void *)"aaaa";
  xQueueSend(queue, &msg, INT32_MAX);
}

void loop()
{
}