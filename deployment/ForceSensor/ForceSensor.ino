#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// =============================
// Wi-Fi Credentials
// =============================
const char* ssid     = "UNIFI-AP";
const char* password = "P@ssw0rd";

const char* HOST_NAME = "force_sensor";

// =============================
// Matrix Pins
// =============================
#define BAUD_RATE                 115200
#define ROW_COUNT                 16
#define COLUMN_COUNT              16
#define PIN_ADC_INPUT             34

#define PIN_MUX_CHANNEL_0         26
#define PIN_MUX_CHANNEL_1         32  
#define PIN_MUX_CHANNEL_2         33

const int columnPins[COLUMN_COUNT] = {2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,27};

// =============================
// Globals
// =============================
WebServer webServer(80);
int fsrValues[ROW_COUNT][COLUMN_COUNT];

#define COMMUNICATE_WITH_PHONE //for use with phone app

// =============================
// Function Declarations
// =============================
void setRow(int row_number);
void enableColumn(int column_number);
void disableAllColumns();
void readMatrix();
void initialiseWebServer();
void handleTrigger();

// =============================
// Setup
// =============================
void setup() {
  Serial.begin(BAUD_RATE);
  
  // Setup ADC pin
  pinMode(PIN_ADC_INPUT, INPUT);
  
  // Setup multiplexer control pins
  pinMode(PIN_MUX_CHANNEL_0, OUTPUT);
  pinMode(PIN_MUX_CHANNEL_1, OUTPUT);
  pinMode(PIN_MUX_CHANNEL_2, OUTPUT);
  
  // Setup all column pins as outputs
  for (int i = 0; i < COLUMN_COUNT; i++) {
    pinMode(columnPins[i], OUTPUT);
    digitalWrite(columnPins[i], LOW);  // Start with all columns off
  }

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  initialiseWebServer();
}

// =============================
// Main Loop
// =============================
void loop() {
  #ifdef COMMUNICATE_WITH_PHONE
    webServer.handleClient();
  #endif
}

//Below code is for a separate webserver for communication with phone
void initialiseWebServer() {
  // Start mDNS
  if (MDNS.begin(HOST_NAME)) {
      Serial.println("mDNS started");
      Serial.print("Access at: http://");
      Serial.print(HOST_NAME);
      Serial.println(".local/trigger");
  }

  // API endpoint
  webServer.on("/trigger", HTTP_GET, handleTrigger);

  webServer.begin();

  Serial.println("HTTP server started");
}

/**
* This method runs when the ESP32 receives a HTTP GET request from the phone.
* 
* The JSON data from the sensors is uploaded to the ESP32's own server for the phone app to read.
*/
void handleTrigger() {
  readMatrix();
  
  int top1 = 0, top2 = 0, top3 = 0;

  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COLUMN_COUNT; c++) {
      int val = fsrValues[r][c];

      if (val > top1) {
        top3 = top2;
        top2 = top1;
        top1 = val;
      } else if (val > top2) {
        top3 = top2;
        top2 = val;
      } else if (val > top3) {
        top3 = val;
      }
    }
  }

  // Calculate average of top 3
  float Avg = (top1 + top2 + top3) / 3.0;

  // Create JSON 
  String jsonPayload = "{\"matrix\":" + String(Avg, 2) + "}";

  webServer.send(200, "application/json", jsonPayload);
}

// =============================
// Read Matrix
// =============================
void readMatrix() {
  for (int row = 0; row < ROW_COUNT; row++) {
    setRow(row);
    
    for (int col = 0; col < COLUMN_COUNT; col++) {
      enableColumn(col);
      delayMicroseconds(50);  // Let signal settle
      
      fsrValues[row][col] = analogRead(PIN_ADC_INPUT);
      
      disableAllColumns();
    }
  }
}

// =============================
// Helper Functions
// =============================
void setRow(int row_number) {
  int channel = row_number % 8;
  
  digitalWrite(PIN_MUX_CHANNEL_0, channel & 0x01);
  digitalWrite(PIN_MUX_CHANNEL_1, (channel & 0x02) >> 1);
  digitalWrite(PIN_MUX_CHANNEL_2, (channel & 0x04) >> 2);
}

void enableColumn(int column_number) {
  if (column_number >= 0 && column_number < COLUMN_COUNT) {
    digitalWrite(columnPins[column_number], HIGH);
  }
}

void disableAllColumns() {
  for (int i = 0; i < COLUMN_COUNT; i++) {
    digitalWrite(columnPins[i], LOW);
  }
}
