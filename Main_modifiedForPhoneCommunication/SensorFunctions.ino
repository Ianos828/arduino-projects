#include <Wire.h>
#include "SparkFun_AS7265X.h"
#include "SparkFun_VL53L1X.h"
#include "Adafruit_SGP40.h"
#include <esp32-hal-ledc.h>
#include <string.h>
#include <WiFi.h> //Hariz
#include <ArduinoJson.h>

// Forward declarations for functions defined in WifiFunctions.ino Hariz
extern void reconnectWifi();
extern void sendDataViaWifi(const char* serverUrl, String payload);

void initAllSensors();
void initDgs();
void initTriad();
void initLidar();
void initSgp40();
void initOpt101();
String formatJson(
  float fluorescenceInMillivolts,
  float* nirReadings,
  int distanceInMilimetres,
  String ethyleneInPartsPerBillion,
  uint16_t airQualityValue,
  int mq3Reading,
  int mq4Reading,
  int mq5Reading
);
String getSensorsJson();
float getOpt101Reading();
float* getTriadReading();
int getLidarReading();
String getDgsReading();
uint16_t getSgp40Reading();
int getMq3Reading();
int getMq4Reading();
int getMq5Reading();

AS7265X triad;
SFEVL53L1X lidar;
Adafruit_SGP40 sgp;

// Pin Definitions
const int SENSOR_PIN = A3;    // A0 is GPIO2 on Nano ESP32
const int LED_PIN    = 13;    // D12 pin

const int DGS_TX_PIN = 8;
const int DGS_RX_PIN = 7;

const int MQ3_PIN = 6;
const int MQ4_PIN = 5;
const int MQ5_PIN = 4;

// PWM "Lock-in" Configuration
const int LED_FREQ    = 1000;   // 1 kHz pulse frequency
const int LED_RES     = 8;      // 8-bit resolution (0-255 duty cycle)
const int LED_DUTY    = 128;    // 50% duty cycle (128 out of 255)

// Sampling Configuration
// The time window to find one peak/trough pair.
const int SAMPLE_WINDOW_MS = 50;

// Averaging Configuration
const int READING_COUNT = 20;   // Number of readings to average

const int LOW_BAUD_RATE = 9600;
const int HIGH_BAUD_RATE = 115200;
const int DGS_STRING_PPB_STARTING_INDEX = 14;

extern const char* GOOGLE_SERVER_URL;
extern const char* LOCAL_PC_URL;

void initAllSensors() {
  Serial.begin(HIGH_BAUD_RATE);

  initDgs();
  initTriad();
  initLidar();
  initSgp40();
  initOpt101();
}

void initDgs() {
  Serial1.begin(LOW_BAUD_RATE, SERIAL_8N1, DGS_RX_PIN, DGS_TX_PIN);  //Start Serial1 for DGS comm RX: 7, TX: 8
  delay(1000);

  Serial.println("Ethylene Sensor initialised!");
}

void initTriad() {
  if (!triad.begin()) {
    Serial.println("Triad not detected.");
  } else {
    // Correct constant for reading all 18 channels
    triad.setMeasurementMode(AS7265X_MEASUREMENT_MODE_6CHAN_CONTINUOUS); 
    triad.setIntegrationCycles(99); 
    triad.setGain(AS7265X_GAIN_37X);
    Serial.println("Triad ready in 18-channel mode.");
  }
}

void initLidar() {
  // --- LiDAR (VL53L1X) ---
  if (!lidar.begin()) {
    Serial.println("LiDAR (VL53L1X) not detected (I2C Address: 0x29).");
  } else {
    lidar.setDistanceModeShort();        // better for close range
    lidar.setTimingBudgetInMs(33);       // 20–50 ms typical
    lidar.setIntermeasurementPeriod(40); // must be >= timing budget
    lidar.startRanging();                // continuous mode
    Serial.println("LiDAR (VL53L1X) ready.");
  }
}

void initSgp40() {
  while (!Serial) { delay(10); } // Wait for serial console to open!

  if (!sgp.begin()){
    Serial.println("Air Quality Sensor (SGP40) not found (I2C Address: 0x59).");
  } else {
    Serial.println("Air Quality Sensor (SGP40) initialised!");
  }
}

void initOpt101() {
  delay(1000);
  analogReadResolution(12); // Sets ADC to 0-4095 range
  
  // Corrected: Pass both the pin and the attenuation level
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db); 

  // NEW ESP32 3.x API for PWM
  ledcAttach(LED_PIN, LED_FREQ, LED_RES); 
  ledcWrite(LED_PIN, LED_DUTY); 

  Wire.begin(11, 12);
  Wire.setClock(100000);
  Serial.println("OPT101 Initialised!");
}

/*
* Returns a JSON string based on the input values
*
* @return a string in JSON format
*/
String formatJson(
  float fluorescenceInMillivolts,
  float* nirReadings,
  int distanceInMilimetres,
  String ethyleneInPartsPerBillion,
  uint16_t airQualityValue,
  int mq3Reading,
  int mq4Reading,
  int mq5Reading
) {
  StaticJsonDocument<512> doc;

  doc["fluorescence"] = fluorescenceInMillivolts;

  doc["nir_680"] = nirReadings[0];
  doc["nir_705"] = nirReadings[1];
  doc["nir_730"] = nirReadings[2];
  doc["nir_760"] = nirReadings[3];
  doc["nir_810"] = nirReadings[4];
  doc["nir_860"] = nirReadings[5];
  doc["nir_940"] = nirReadings[6];

  doc["distance_mm"] = distanceInMilimetres;
  doc["ethylene_ppb"] = ethyleneInPartsPerBillion;
  doc["air_quality"] = airQualityValue;

  doc["mq3"] = mq3Reading;
  doc["mq4"] = mq4Reading;
  doc["mq5"] = mq5Reading;

  String payload;
  serializeJson(doc, payload);

  return payload;
}

/*
* Gets a reading from the sensors and returns a String in JSON format.
*
* @return a String in JSON format
*/
String getSensorsJson() {
  float fluorescenceInMillivolts = getOpt101Reading();

  float* nirReadings = getTriadReading();

  int distanceInMilimetres = getLidarReading();

  String ethyleneInPartsPerBillion = getDgsReading();

  uint16_t airQualityValue = getSgp40Reading();

  int mq3Reading = getMq3Reading();

  int mq4Reading = getMq4Reading();

  int mq5Reading = getMq5Reading();

  return formatJson(
    fluorescenceInMillivolts,
    nirReadings,
    distanceInMilimetres,
    ethyleneInPartsPerBillion,
    airQualityValue,
    mq3Reading,
    mq4Reading,
    mq5Reading
  );
}

float getOpt101Reading() {
  // Set up for a new batch of readings
  // Serial.print("Taking 20 OPT101 readings...");

  // Use 'long' for the sum to prevent overflow
  long totalFluorescenceIntensity = 0;

  // Run the loop 20 times to get 20 readings
  for (int i = 0; i < READING_COUNT; i++) {

    // We will read the sensor voltage in millivolts (mV).
    int voltagePeak   = 0;
    int voltageTrough = 4095; // Max possible reading in mV

    // Get the start time for our sampling windowS
    unsigned long startTime = millis();

    // Rapidly sample the ADC for the duration of the window
    while (millis() - startTime < SAMPLE_WINDOW_MS) {
      int currentVoltage = analogReadMilliVolts(SENSOR_PIN);

      // Check for a new peak
      if (currentVoltage > voltagePeak) {
        voltagePeak = currentVoltage;
      }

      // Check for a new trough
      if (currentVoltage < voltageTrough) {
        voltageTrough = currentVoltage;
      }
    }

    // Calculate the final Fluorescence Intensity (FI) for this one sample
    int fluorescenceIntensity = voltagePeak - voltageTrough;

    // Add this reading to our total sum
    totalFluorescenceIntensity += fluorescenceIntensity;

    // Print a dot to show progress
    // Serial.print(".");

    // Brief delay between individual samples
    delay(10);
  }

  // Calculate and print the average

  // Use floating-point math for an accurate average
  float averageFluorescenceIntensity = (float)totalFluorescenceIntensity / (float)READING_COUNT;

  // Serial.println(); // Go to the next line
  // Serial.print("Average FI (of 20): ");
  // Serial.println(averageFluorescenceIntensity);
  // Serial.println(); // Add a blank line for readability
  return averageFluorescenceIntensity;
}

float* getTriadReading() {
  // Start measurement
  triad.takeMeasurementsWithBulb();

  // Wait for the data to be ready (Critical for 99 integration cycles)
  unsigned long timeout = millis();
  while (!triad.dataAvailable() && (millis() - timeout < 2000)) {
    delay(10);
  }

  static float nirReadings[7]; 
  nirReadings[0] = triad.getCalibratedR(); // 680 nm 
  nirReadings[1] = triad.getCalibratedD(); // 705 nm
  nirReadings[2] = triad.getCalibratedS(); // 730 nm
  nirReadings[3] = triad.getCalibratedT(); // 760 nm
  nirReadings[4] = triad.getCalibratedU(); // 810 nm
  nirReadings[5] = triad.getCalibratedV(); // 860 nm
  nirReadings[6] = triad.getCalibratedW(); // 940 nm

  return nirReadings;
}

int getLidarReading() {
  // ---------- LiDAR ----------
  lidar.startRanging();                       // Trigger a single measurement
  int distance;
  uint32_t start = millis();
  while (!lidar.checkForDataReady()) {        // Wait for data ready
    if (millis() - start > 300) {             // Timeout after 300 ms
      Serial.println("Distance: timeout waiting for data.");
      break;
    }
    delay(5);
  }

  if (lidar.checkForDataReady()) {
    distance = lidar.getDistance();       // in mm
    lidar.clearInterrupt();                   // Clear data ready flag
    lidar.stopRanging();                      // Stop measurement
    // Serial.println("---- LiDAR Output Reading ----");
    // Serial.print("Distance (mm): ");
    // Serial.println(distance);
    // Serial.println();
  } else {
    lidar.stopRanging();
    distance = -1;
  }
  return distance;
}

String getDgsReading() {
  Serial1.write('\r');
  // While data is available from DGS/DGS2 serial
  while (!Serial1.available());
  String DgsOutputString = Serial1.readString();
  DgsOutputString.trim();

  int indexOfSecondComma = DgsOutputString.indexOf(",", DGS_STRING_PPB_STARTING_INDEX);
  return DgsOutputString.substring(DGS_STRING_PPB_STARTING_INDEX, indexOfSecondComma);
}

uint16_t getSgp40Reading() {
  return sgp.measureRaw();
}

int getMq3Reading() {
  return analogRead(MQ3_PIN);
}

int getMq4Reading() {
  return analogRead(MQ4_PIN);
}

int getMq5Reading() {
  return analogRead(MQ5_PIN);
}