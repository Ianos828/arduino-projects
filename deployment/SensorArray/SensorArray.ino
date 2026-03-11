#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>

extern void initWifi();
extern void initAllSensors();
extern void reconnectWifi();

extern WebServer server;

#define COMMUNICATE_WITH_PHONE

/*
* Intialises the ESP32 to connect to the WiFi network and initialise all sensors.
*/
void setup() {
  initWifi();
  
  //initialise all sensors
  initAllSensors();
}

/*
* Main loop of the program that responds to a HTTP GET request from the companion app.
*/
void loop() {
  #ifdef COMMUNICATE_WITH_PHONE
    server.handleClient();
  #endif

  if (WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
}

