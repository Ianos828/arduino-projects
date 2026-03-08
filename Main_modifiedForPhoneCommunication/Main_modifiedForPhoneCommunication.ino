#include <Arduino.h> //hariz
#include <WebServer.h>
#include <ESPmDNS.h>

extern void initWifi();
extern void initAllSensors();
extern void reconnectWifi();
extern String getSensorsJson();

// External variables defined in WifiFunctions.ino hariz
extern const char* GOOGLE_SERVER_URL;
extern const char* LOCAL_PC_URL;
extern WebServer server;

//timing constants CHANGE THIS to 1h or smth!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! hi regan
const int SAMPLING_DELAY = 1800000;

//comment out lines according to data destinations if required
#define SEND_DATA_TO_SHEETS
// #define SEND_DATA_TO_CSV
// #define COMMUNICATE_WITH_PHONE

void setup() {
  initWifi();
  
  //initialise all sensors
  initAllSensors();
}

void loop() {
  #ifdef COMMUNICATE_WITH_PHONE
    server.handleClient();
  #endif

  //ideally all the code below this gets nuked if we dont need data logging
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

