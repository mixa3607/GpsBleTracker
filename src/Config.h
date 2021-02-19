#pragma once

#include <HardwareSerial.h>
#include <Wire.h>
#include <Misc/ButtonTypes.h>

//gps
//#define GPS_TASK_MOCK 1 //if set then use trash gps reports insted real gps module
HardwareSerial *const GpsSerial = &Serial2; //tx2=17; rx2=16
const int GpsSerialBaudrate = 230400;       //9600 default
const int GpsFixQueueLen = 5;               //messages in gps queue, when full begin overwrite last

//display
const uint8_t DisplayAddress = 0x3C; //3C => 128x32; 3D => 128x64
const uint8_t DisplayWidth = 128;
const uint8_t DisplayHeight = 32;
const uint8_t DisplayRotate = 2;    //90deg * 2
TwoWire *const DisplayWire = &Wire; //sda=21; scl=22
const int DisplayMsgQueueLen = 5;   //messages in render queue, when full begin overwrite last

//message broker
//nothing...

//ble
const char *const BleManufcturer = "Mixa3607";
const char *const BleDeviceName  = "BleGps";
//gps service
const char *const BleGpsServiceUuid    = "1ac88a96-6a36-4c1e-958f-7f34a7000000";
const char *const BleGpsFixCharactUuid = "1ac88a96-6a36-4c1e-958f-7f34a7000001";
//auth service
const char *const BleAuthServiceUuid        = "1ac88a96-6a36-4c1e-958f-7f34a7000100";
const char *const BleAuthLoginCharactUuid   = "1ac88a96-6a36-4c1e-958f-7f34a7000101";
const char *const BleAuthIsLoginCharactUuid = "1ac88a96-6a36-4c1e-958f-7f34a7000102";
const char *const BleAuthPassword = "123"; //ble password
const int BleMsgQueueLen = 50; //ble messages queue, when full begin overwrite last

//buttons
const Btns::Button ButtonsInfo[] = {
        {Btns::BtnCenter, 25, INPUT_PULLUP, LOW},
        {Btns::Btn2, 25, INPUT_PULLUP, LOW}};