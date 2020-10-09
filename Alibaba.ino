#include "BLEDevice.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems

// The remote service we wish to connect to.
static BLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
static String SWITCHBOT = "c0:e5:90:81:37:6f";
static String BEACON_1 = "d0:f0:18:43:f2:82";
static String BEACON_2 = "d0:f0:18:43:f5:9b";

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean inProximity = false;
static unsigned long last_press = 0;
static unsigned long last_connection = 0;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static uint8_t cmdPress[3] = {0x57, 0x01, 0x00};

bool connectToSwitchbot() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->connect(myDevice);
  Serial.println(" - Connected to switchbot");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  connected = true;
  return true;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String addr = advertisedDevice.getAddress().toString().c_str();
      if (inProximity &&
          addr.equalsIgnoreCase(SWITCHBOT)) {
        Serial.print("SWITCHBOT found: ");
        Serial.println(addr);
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

      } else if (addr.equalsIgnoreCase(BEACON_1) || addr.equalsIgnoreCase(BEACON_2)) {
        int delay = (millis() - last_press) / 1000;
        if (delay >= 60 || last_press == 0) {
          inProximity = true;
          Serial.print("BEACON Device found: ");
          Serial.println(addr);
        }
      }
    }
};


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println("Starting Alibaba application...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  Serial.println("doConnect");
  Serial.println(doConnect);
  Serial.print("inProximity ");
  Serial.println(inProximity);
  if (doConnect == true) {
    if (connectToSwitchbot()) {
      Serial.println("We are now connected to the switchbot.");
    } else {
      Serial.println("We have failed to connect to the switchbot; there is nothin more we will do.");
    }
    doConnect = false;
    last_connection = millis();
  }

  int connTime = (millis() - last_connection) / 1000;
  Serial.println("connTime");
  Serial.println(connTime);
  if (connTime >= 180) {
    connected = false;
  }
  if (connected) {
    int delay = (millis() - last_press) / 1000;
    Serial.println("delay");
    Serial.println(delay);
    if ((delay >= 60 || last_press == 0) && inProximity) {
      //trigger switchbot
      Serial.println("trigger switchbot");
      pRemoteCharacteristic->writeValue(cmdPress, sizeof(cmdPress));
      last_press = millis();
      last_connection = millis();
      inProximity = false;
    }
  }
  BLEDevice::getScan()->start(2);
  Serial.println("------------------");
} // End of loop
