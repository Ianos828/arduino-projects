#include <Arduino.h>

extern void initWifi();
extern void initAllSensors();
extern String getSensorsJson();
extern void reconnectWifi();
extern void sendDataViaWifi(const char* serverUrl, String payload);

// External variables defined in WifiFunctions.ino hariz
extern const char* GOOGLE_SERVER_URL;
extern const char* LOCAL_PC_URL;

//timing constants CHANGE THIS to 1h or smth!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! hi regan
const int SAMPLING_DELAY = 1800000;

//comment out lines according to data destinations if required
#define SEND_DATA_TO_SHEETS
// #define SEND_DATA_TO_CSV

/*
* Intialises the ESP32 to connect to the WiFi network and initialise all sensors.
*/
void setup() {
  initWifi();
  
  //initialise all sensors
  initAllSensors();
}

/*
* Main loop of the program that periodically sends sensor data to specified locations.
*/
void loop() {
  //get and send all sensor data via WiFi
  String payload = getSensorsJson();
  Serial.println(payload);

  if (WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }

  //send data via WiFi to Google Sheets
  #ifdef SEND_DATA_TO_SHEETS
    sendDataViaWifi(GOOGLE_SERVER_URL, payload);
  #endif

  //send data via WiFi to local PC
  #ifdef SEND_DATA_TO_CSV
    sendDataViaWifi(LOCAL_PC_URL, payload);
  #endif
}

