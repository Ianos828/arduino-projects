#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Forward Declarations
void handleTrigger();
void handleLidarScan();
void initWifi();
void reconnectWifi();
void connectedToApHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);
void gotIpHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);
void wifiDisconnectedHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);

//for WiFi connection, modify accordingly
const char* SSID = "UNIFI-AP";
const char* PASSWORD = "P@ssw0rd";
const char* HOST_NAME = "esp32_combined";

WebServer server(80);

extern String getSensorsJson();
extern String getLidarScanJson();

/*
* This method runs when the ESP32 receives a HTTP GET request from the phone.
* 
* The JSON data from the sensors is uploaded to the ESP32's own server for the phone app to read.
*/
void handleTrigger() {
  String json = getSensorsJson();

  server.send(200, "application/json", json);
}
void handleLidarScan() {
  String json = getLidarScanJson();
  server.send(200, "application/json", json);
}

/*
* Connects the ESP32 to the WiFi network and sets up a webserver.
*/
void initWifi() {
  //sets WiFi mode to STATION mode
  WiFi.mode(WIFI_STA);

  //interrupts for WiFi connection
  WiFi.onEvent(connectedToApHandler, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(gotIpHandler, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(wifiDisconnectedHandler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(SSID, PASSWORD);
  Serial.println("\nConnecting to WiFi Network ..");

  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter < 20) {
    delay(500);
    Serial.print(".");
    timeoutCounter++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Start mDNS
  if (MDNS.begin(HOST_NAME)) {
      Serial.println("mDNS started");
      Serial.print("Access at: http://");
      Serial.print(HOST_NAME);
      Serial.println(".local/trigger");
  }

  // API endpoint
  server.on("/trigger", HTTP_GET, handleTrigger);
  server.on("/lidar-scan", HTTP_GET, handleLidarScan);

  server.begin();

  Serial.println("HTTP server started");
}

/*
* If WiFi gets disconnected, try to reconnect 5 times before giving up.
*/
void reconnectWifi() {
  // AGGRESSIVE RECONNECT
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 5) {
    Serial.println("WiFi disconnected! Attempting reconnect...");
    WiFi.disconnect();
    WiFi.begin(SSID, PASSWORD);
    
    // Wait up to 5 seconds for connection
    unsigned long startWait = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startWait < 5000) {
      delay(500);
      Serial.print(".");
    }
    retryCount++;
  }
}

/*
* Handler for when WiFi gets connected.
*/
void connectedToApHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.println("Connected To The WiFi Network");
}

/*
* Handler when ESP32 is assigned an IP.
*/
void gotIpHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

/*
* Handler when WiFi gets disconnected.
*/
void wifiDisconnectedHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.println("Disconnected From WiFi Network");
  // Attempt Re-Connection
  WiFi.begin(SSID, PASSWORD);
}
