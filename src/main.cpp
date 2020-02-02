#include <Arduino.h>
#include <EasyButton.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Change this to true, if you want push-to-talk, otherwise you get push-to-mute
#define PUSH_TO_TALK false
// Change this if you have a different Port to connect your button to
#define BUTTON_PIN 18
// Change this for custom device name
#define DEVICE_NAME "RaeusperESP32"
// Change this for different channel (default:1)
#define CHANNEL_NR 0x01

bool deviceConnected = false;
BLECharacteristic *pCharacteristic;
uint8_t midiPacket[] = {0x80, 0x80, 0xB0, CHANNEL_NR, 127};

EasyButton button_down(BUTTON_PIN);
EasyButton button_up(BUTTON_PIN, 35, true, false);

void processPressDown()
{
  Serial.printf("Button Down\n");
  midiPacket[4] = (PUSH_TO_TALK ? 0 : 127);
  pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes
  pCharacteristic->notify();
  Serial.printf("ON\n");
  delay(50);
}

void processPressUp()
{
  Serial.printf("Button Up\n");
  midiPacket[4] = (PUSH_TO_TALK ? 127 : 0);
  pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
  pCharacteristic->notify();
  Serial.printf("OFF\n");
  delay(50);
}

class MyCallbacks : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    deviceConnected = true;
    Serial.println("Connected!");
  };

  void onDisconnect(BLEServer *pServer) override
  {
    deviceConnected = false;
    Serial.println("Disonnected!");
  }

  void onWrite(BLECharacteristic *pChar) override
  {
    std::string value = pChar->getValue();
    const uint8_t *const data = reinterpret_cast<const uint8_t *const>(value.data());
    size_t len = value.size();
    Serial.print("Write: ");
    for (size_t i = 0; i < len; i++)
      Serial.printf("%02x ", data[i]);
    Serial.println();
  }

  void onRead(BLECharacteristic *pChar) override
  {
    Serial.println("Read");
    pChar->setValue("");
  }
};

void setup()
{
  Serial.begin(115200);

  button_down.begin();
  button_up.begin();
  button_down.onPressed(processPressDown);
  button_up.onPressed(processPressUp);

  static MyCallbacks cb;

  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(&cb);

  BLEService *pService = pServer->createService("03b80e5a-ede8-4b33-a751-6ce34ec4c700");
  pCharacteristic = pService->createCharacteristic("7772e5db-3868-4112-a1a9-f2669d106bf3",
                                                   BLECharacteristic::PROPERTY_READ |
                                                       BLECharacteristic::PROPERTY_NOTIFY |
                                                       BLECharacteristic::PROPERTY_WRITE_NR);

  pCharacteristic->setCallbacks(&cb);

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);

  ESP_LOGD(LOG_TAG, "Advertising started!");
}

void loop()
{
  if (deviceConnected)
  {
    button_down.read();
    button_up.read();
  }
}
