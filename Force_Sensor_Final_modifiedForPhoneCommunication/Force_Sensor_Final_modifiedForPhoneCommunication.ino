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

// =============================
// Google Sheets Endpoint
// =============================
// Make sure this is your real deployed web app URL with /exec
const char* GOOGLE_SERVER_URL = "https://script.google.com/macros/s/AKfycbwI3v-q9joOscSdw_lYDmvAsofaRes9HZnBwhH90rWYbiDjLet5iu27EyQRo9D2XEt2/exec";
const char* HOST_NAME = "force_sensor"

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
WiFiServer server(80);
WebServer webServer(80);
int fsrValues[ROW_COUNT][COLUMN_COUNT];

#define COMMUNICATE_WITH_PHONE //for use with phone app

// For periodic uploads (currently 2 seconds for testing)
unsigned long lastUploadTime = 0;
const unsigned long UPLOAD_INTERVAL = 1800000;  // change to 3600000UL for 1 hour

// =============================
// Function Declarations
// =============================
void setRow(int row_number);
void enableColumn(int column_number);
void disableAllColumns();
void readMatrix();
void sendWebPage(WiFiClient client);
void sendMatrixToGoogleSheets();
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

  server.begin();

  initialiseWebServer();
}

// =============================
// Main Loop
// =============================
void loop() {
  #ifdef COMMUNICATE_WITH_PHONE
    webServer.handleClient();
  #endif

  // Always update matrix for live display
  readMatrix();

  // Periodic upload to Google Sheets
  unsigned long currentTime = millis();
  if (currentTime - lastUploadTime >= UPLOAD_INTERVAL) {
    sendMatrixToGoogleSheets();
    lastUploadTime = currentTime;
  }

  // Serve live web page if client connects
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String req = client.readStringUntil('\r');
        client.flush();
        sendWebPage(client);
        break;
      }
    }
    client.stop();
  }

  delay(500);  // controls live page refresh frequency
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
  //DUPLICATED CODE FROM UR sendMatrixToGoogleSheets() METHOD
  // Build comma-separated values
  String payload = "";
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COLUMN_COUNT; c++) {
      payload += String(fsrValues[r][c]);
      if (!(r == ROW_COUNT-1 && c == COLUMN_COUNT-1)) {
        payload += ",";
      }
    }
  }

  // Create JSON
  String jsonPayload = "{\"matrix\":\"" + payload + "\"}";

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
// Google Sheets Upload
// =============================
void sendMatrixToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping upload.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate verification (common fix for script.google.com)

  HTTPClient http;

  Serial.print("[HTTPS] Connecting to Google Apps Script... ");

  if (http.begin(client, GOOGLE_SERVER_URL)) {
    
    http.addHeader("Content-Type", "application/json");

    // Build comma-separated values
    String payload = "";
    for (int r = 0; r < ROW_COUNT; r++) {
      for (int c = 0; c < COLUMN_COUNT; c++) {
        payload += String(fsrValues[r][c]);
        if (!(r == ROW_COUNT-1 && c == COLUMN_COUNT-1)) {
          payload += ",";
        }
      }
    }

    // Create JSON
    String jsonPayload = "{\"matrix\":\"" + payload + "\"}";

    // Optional: print first part for debugging (remove later if you want)
    if (jsonPayload.length() > 200) {
      Serial.println("Sending JSON (first 200 chars): " + jsonPayload.substring(0, 200) + "...");
    } else {
      Serial.println("Sending JSON: " + jsonPayload);
    }

    // Send POST request
    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0) {
      Serial.printf("HTTP response code: %d\n", httpCode);
      
      if (httpCode == 200 || httpCode == 302) {
        String response = http.getString();
        Serial.println("Response: " + response);
      } else {
        Serial.println("Unexpected HTTP code");
      }
    } else {
      Serial.printf("HTTP request failed: %s (error code %d)\n", 
                    http.errorToString(httpCode).c_str(), httpCode);
    }

    http.end();
  } else {
    Serial.println("Failed to connect / begin HTTPS");
  }
}

// =============================
// Web Page Output
// =============================
void sendWebPage(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta http-equiv='refresh' content='2'>");  // auto-refresh every 2s
  client.println("<style>body{font-family:Arial;} table{border-collapse:collapse;} td{border:1px solid #ccc;padding:4px;text-align:center;}</style>");
  client.println("</head><body>");
  client.println("<h2>ESP32 FSR Matrix Readings</h2>");
  client.println("<table>");
  for (int i = 0; i < ROW_COUNT; i++) {
    client.println("<tr>");
    for (int j = 0; j < COLUMN_COUNT; j++) {
      client.print("<td>");
      client.print(fsrValues[i][j]);
      client.print("</td>");
    }
    client.println("</tr>");
  }
  client.println("</table>");
  client.println("<p>Page auto-refreshes every 2 seconds.</p>");
  client.println("</body></html>");
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
