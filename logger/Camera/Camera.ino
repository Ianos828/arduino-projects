#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "board_config.h"
#include <time.h>

// Base64 encoding (minimal implementation)
static const char* base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64_encode(const uint8_t* data, size_t length) {
  String encoded = "";
  size_t i = 0;
  uint8_t pad = 0;

  while (i < length) {
    uint32_t octet_a = i < length ? data[i++] : 0;
    uint32_t octet_b = i < length ? data[i++] : 0;
    uint32_t octet_c = i < length ? data[i++] : 0;

    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    encoded += base64_chars[(triple >> 3 * 6) & 0x3F];
    encoded += base64_chars[(triple >> 2 * 6) & 0x3F];
    encoded += base64_chars[(triple >> 1 * 6) & 0x3F];
    encoded += base64_chars[(triple >> 0 * 6) & 0x3F];
  }

  pad = length % 3;
  if (pad > 0) {
    for (size_t j = 0; j < (3 - pad); j++) {
      encoded[encoded.length() - (3 - j)] = '=';
    }
  }

  return encoded;
}

const char *ssid = "UNIFI-AP";
const char *password = "P@ssw0rd";

// Replace with your actual deployed Google Apps Script URL
const char* GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbyRjrJyLJHzfCRWS1pw5IrI_50RQVp2L_TOjvKZi3AHw1YUUFvjCPpRtTsqjiNSgs5qKQ/exec";

// Flash LED pin
#define FLASH_LED_PIN     4
#define FLASH_BRIGHTNESS  8   // 0–255; 5 is very dim — consider 25–50 for testing

// Timing
unsigned long previousMillis = 0;
const unsigned long interval = 1800000UL;  // 30 minutes
// For testing: const unsigned long interval = 10000UL;  // 10 seconds

// NTP settings (Singapore = UTC+8)
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;       // UTC+8 = 8 * 3600
const int   daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32-CAM Periodic Photo Uploader");

  // Setup flash LED
  ledcAttach(FLASH_LED_PIN, 5000, 8);
  ledcWrite(FLASH_LED_PIN, 0);

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_XGA;
  config.jpeg_quality = 8;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (psramFound()) {
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Camera sensor not found!");
    return;
  }

  // Original sensor tweaks
  s->set_reg(s, 0x11, 0xFF, 0x01);
  delay(100);
  s->set_reg(s, 0x15, 0xFF, 0x20);
  delay(100);
  s->set_reg(s, 0x0C, 0xFF, 0x00);
  delay(100);
  s->set_reg(s, 0x0C, 0xFF, 0x08);
  delay(100);
  s->set_reg(s, 0x15, 0xFF, 0x22);
  delay(100);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  delay(3000);  // Give time to sync

  captureAndUploadPhoto();
  previousMillis = millis();
}

void captureAndUploadPhoto() {

  Serial.println("Starting capture & upload cycle...");

  const int max_attempts = 2;
  int attempt = 0;
  bool uploaded = false;

  while (!uploaded && attempt < max_attempts) {
    attempt++;

    // Reconnect WiFi if needed
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      int reconnect_attempts = 0;
      while (WiFi.status() != WL_CONNECTED && reconnect_attempts < 20) {
        delay(500);
        Serial.print(".");
        reconnect_attempts++;
      }
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to reconnect WiFi");
      } else {
        Serial.println("WiFi reconnected");
      }
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("No WiFi connection — skipping attempt");
      if (attempt < max_attempts) delay(10000);
      continue;
    }

    ledcWrite(FLASH_LED_PIN, FLASH_BRIGHTNESS);
    delay(1000);

    Serial.printf("Capturing photo (attempt %d/%d)... ", attempt, max_attempts);
    camera_fb_t *fb = esp_camera_fb_get();

    ledcWrite(FLASH_LED_PIN, 0);

    if (!fb) {
      Serial.println("Camera capture failed");
      if (attempt < max_attempts) {
        Serial.println("Retrying in 10 seconds...");
        delay(10000);
      }
      continue;
    }

    Serial.printf("Captured: %u bytes\n", fb->len);

    // Base64 encode and prepare payload
    String base64Image = base64_encode(fb->buf, fb->len);
    String jsonPayload = "{\"image\":\"" + base64Image + "\"}";

    // Upload
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.printf("Uploading (attempt %d)... ", attempt);

    if (http.begin(client, GOOGLE_SCRIPT_URL)) {
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.POST(jsonPayload);

      if (httpCode > 0) {
        Serial.printf("HTTP %d\n", httpCode);
        if (httpCode >= 200 && httpCode < 300) {
          uploaded = true;
          String response = http.getString();
          Serial.println("Success - Response: " + response);
        } else {
          Serial.println("Server rejected with code " + String(httpCode));
        }
      } else {
        Serial.println("HTTP failed: " + http.errorToString(httpCode));
      }
      http.end();
    } else {
      Serial.println("Failed to begin HTTP connection");
    }

    esp_camera_fb_return(fb);

    if (!uploaded && attempt < max_attempts) {
      Serial.println("Retry in 10 seconds...");
      delay(10000);
    }
  }

  if (!uploaded) {
    Serial.println("Failed after " + String(max_attempts) + " attempts");
  } else {
    Serial.println("Upload successful");
  }

  // Always update timer
  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    captureAndUploadPhoto();
  }
  delay(100);  // Watchdog safety
}


