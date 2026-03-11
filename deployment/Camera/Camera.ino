#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "board_config.h"
#include <time.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char *ssid = "UNIFI-AP";
const char *password = "P@ssw0rd";

const char* HOST_NAME = "esp32_cam_image";

// Flash LED pin
#define FLASH_LED_PIN     4
#define FLASH_BRIGHTNESS  8   // 0–255; 5 is very dim — consider 25–50 for testing

//sets up a webserver at port 80
WebServer server(80);

//forward declarations
void initialiseWebServer();
void handleTrigger();

#define COMMUNICATE_WITH_PHONE

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

  //set up webserver for phone app
  initialiseWebServer();
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
  server.on("/image", HTTP_GET, handleImage);

  server.begin();

  Serial.println("HTTP server started");
}

//when a HTTP GET request is received from the app
void handleImage() {
  //turn on the LED and wait 2s
  ledcWrite(FLASH_LED_PIN, FLASH_BRIGHTNESS);
  delay(2000);

  //get the image
  camera_fb_t * fb = esp_camera_fb_get();

  //turn off the LED
  ledcWrite(FLASH_LED_PIN, 0);

  //if capture fails
  if (!fb) {
      server.send(500, "text/plain", "Camera capture failed");
      return;
  }

  //else send the image via WiFi to mDNS server
  server.send_P(
      200,
      "image/jpeg",
      (const char*)fb->buf,
      fb->len
  );

  //releases the buffer
  esp_camera_fb_return(fb);
}

void loop() {
  //only listen for app requests when the constant is defined 
  #ifdef COMMUNICATE_WITH_PHONE
    server.handleClient();
  #endif
}


