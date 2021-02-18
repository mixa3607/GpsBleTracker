#include <Config.h>
#include <Tasks/Ble.h>

#include <FreeRTOS.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace Ble
{
    const char *const TAG = "Ble";

    QueueHandle_t bleMsgQueue = nullptr;

    bool ReadMessage(BleMessage *const msgPtr, const uint32_t timeout)
    {
        return xQueueReceive(bleMsgQueue, msgPtr, timeout);
    }

    void WriteMessageOverflow(BleMessage *const value)
    {
        if (xQueueSend(bleMsgQueue, value, pdMS_TO_TICKS(1)) == errQUEUE_FULL)
        {
            ESP_LOGD(TAG, "Ble queue is overflow. Try rewrite");
            BleMessage buf;
            xQueueReceive(bleMsgQueue, &buf, 0);
            xQueueSend(bleMsgQueue, value, 0);
            ESP_LOGV(TAG, "Queue push value");
        }
    }

    esp_bd_addr_t authorizedDbAddr;

    class BleServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
        {
            auto addr = param->connect.remote_bda;
            ESP_LOGI(TAG, "Device connected: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(addr));
        };

        void onDisconnect(BLEServer *pServer)
        {
            ESP_LOGI(TAG, "Device disconnected ");
        }
    };

    class BleGpsFixCharactCallback : public BLECharacteristicCallbacks
    {
        void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
        {
            auto addr = param->read.bda;
            bool isAuth = memcmp(&authorizedDbAddr, addr, ESP_BD_ADDR_LEN) == 0;
            ESP_LOGV(TAG, "Device " ESP_BD_ADDR_STR " try read gps fix data", ESP_BD_ADDR_HEX(addr));
            uint8_t res = 1;
            if (!isAuth)
            {
                pCharacteristic->setValue(&res, 1);
                return;
            }
            BleMessage msg;
            if (ReadMessage(&msg, pdMS_TO_TICKS(1)))
            {
                if (msg.type == Ble::GpsInfo)
                {
                    pCharacteristic->setValue((uint8_t *)&msg.value.geoInfo, sizeof(Gps::GpsGeoInfo));
                    ESP_LOG_BUFFER_HEX_LEVEL(TAG, &msg, sizeof(BleMessage), ESP_LOG_VERBOSE);
                }
            }
            else
            {
                res = 0;
                pCharacteristic->setValue(&res, 1);
            }
        }
    };

    class BleAuthLoginCharactCallback : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
        {
            auto *addr = param->write.bda;
            const char *pwd = pCharacteristic->getValue().c_str();
            ESP_LOGD(TAG, "Try auth from: " ESP_BD_ADDR_STR " with data:", ESP_BD_ADDR_HEX(addr));
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, pwd, strlen(pwd), ESP_LOG_DEBUG);
            if (strcmp(BleAuthPassword, pwd) != 0)
            {
                ESP_LOGW(TAG, "Bad password received from " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(addr));
                return;
            }
            memcpy(&authorizedDbAddr, addr, ESP_BD_ADDR_LEN);
            ESP_LOGD(TAG, "Success auth for mac " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(authorizedDbAddr));
        }
    };

    class BleAuthIsLoginCharactCallback : public BLECharacteristicCallbacks
    {
        void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
        {
            auto *addr = param->read.bda;
            ESP_LOGD(TAG, "Device " ESP_BD_ADDR_STR " read auth status", ESP_BD_ADDR_HEX(addr));

            uint8_t res = memcmp(&authorizedDbAddr, addr, ESP_BD_ADDR_LEN) == 0 ? 0 : 1;
            pCharacteristic->setValue(&res, 1);
        }
    };

    BleServerCallbacks bleServerCallbacks;
    BleGpsFixCharactCallback bleGpsFixChCallbacks;
    BleAuthLoginCharactCallback bleAuthLoginChCallbacks;
    BleAuthIsLoginCharactCallback bleAuthIsLoginChCallbacks;

    bool Init()
    {
        ESP_LOGI(TAG, "Init...");

        ESP_LOGI(TAG, "Create msg queue...");
        bleMsgQueue = xQueueCreate(BleMsgQueueLen, sizeof(BleMessage));
        if (bleMsgQueue == nullptr)
        {
            ESP_LOGE(TAG, "Queue allocation failure");
            return false;
        }
        ESP_LOGI(TAG, "Queue created");

        BLEDevice::init(BleDeviceName);

        //server
        BLEServer *const bleServer = BLEDevice::createServer();
        bleServer->setCallbacks(&bleServerCallbacks);

        //gps service: fix
        BLEService *gpsService = bleServer->createService(BleGpsServiceUuid);
        BLECharacteristic *gpsFixCh = gpsService->createCharacteristic(BleGpsFixCharactUuid, BLECharacteristic::PROPERTY_READ);
        gpsFixCh->setCallbacks(&bleGpsFixChCallbacks);
        gpsService->start();

        //auth service: login, login check
        BLEService *authService = bleServer->createService(BleAuthServiceUuid);
        BLECharacteristic *authLoginCh = authService->createCharacteristic(BleAuthLoginCharactUuid, BLECharacteristic::PROPERTY_WRITE);
        authLoginCh->setCallbacks(&bleAuthLoginChCallbacks);
        BLECharacteristic *authIsLoginCh = authService->createCharacteristic(BleAuthIsLoginCharactUuid, BLECharacteristic::PROPERTY_READ);
        authIsLoginCh->setCallbacks(&bleAuthIsLoginChCallbacks);
        authService->start();

        //start adv
        bleServer->getAdvertising()->start();

        ESP_LOGI(TAG, "Initialized");
        return true;
    }
} // namespace Ble