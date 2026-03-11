#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Forward Declarations
void initWifi();
void sendDataViaWifi(const char* serverUrl, String payload);
void reconnectWifi();
void connectedToApHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);
void gotIpHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);
void wifiDisconnectedHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo);

//for WiFi connection, modify accordingly
const char* SSID = "UNIFI-AP";
const char* PASSWORD = "P@ssw0rd";
const char* GOOGLE_SERVER_URL = "https://script.google.com/macros/s/AKfycbzr3yFizSsGRuR5JW_b-NV4U0gP653yVst_NULgUE_q2XwP9R1jWwkB5Oh-yC61BzUVQQ/exec";
const char* LOCAL_PC_URL = "http://192.168.68.55:5000/data";  //http://<PC-IP>:5000/data [get ur pc ip via cmd ipconfig /all and find the ipv4 value]

extern String getSensorsJson();

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
}

void sendDataViaWifi(const char* serverUrl, String payload) {
  HTTPClient http;
  WiFiClientSecure secureClient; 
  bool connectionEstablished = false;

  if (strcmp(serverUrl, LOCAL_PC_URL) == 0) {
    connectionEstablished = http.begin(serverUrl); 
  } else {
    secureClient.setInsecure();
    connectionEstablished = http.begin(secureClient, serverUrl);
  }

  if (connectionEstablished) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close"); 
    http.setTimeout(10000); 

    // Perform the POST silently
    int httpCode = http.POST(payload);

    // Optional: Only print if there is a CRITICAL error (code < 0)
    if (httpCode < 0) {
      Serial.printf("[Error] %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

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

//interrupts
void connectedToApHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.println("Connected To The WiFi Network");
}

void gotIpHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void wifiDisconnectedHandler(WiFiEvent_t wifiEvent, WiFiEventInfo_t wifiInfo) {
  Serial.println("Disconnected From WiFi Network");
  // Attempt Re-Connection
  WiFi.begin(SSID, PASSWORD);
}