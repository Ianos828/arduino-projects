# Arduino
This repo contains three Arduino projects, namely:
1. `SensorArray`
2. `ForceSensor`
3. `Camera`
   
split into `logger` and `deployment` versions. This repository was used to measure the freshness of fruits, by taking a measurement of a fruit's attributes.

## SensorArray
This project communicates with an ESP32 Arduino Nano equipped with:
1. DGS2 CH4 sensor from SPEC Sensors
2. SGP40 Air Quality Sensor from Adafruit
3. MQ3, MQ4 and MQ5 sensors for Arduino
4. OPT101P
5. SparkFun Triad Spectroscopy Sensor - AS7265x (Qwiic)
6. SparkFun Distance Sensor Breakout - 4 Meter, VL53L1X (Qwiic)

to measure the Voltile Organic Compounds (VOCs), fluorescence, NIR and LiDAR readings of the fruit.

## ForceSensor
This project communicates with an ESP32 Arduino Nano equipped with:
1. Sensitronic ThruMode Force Sensor Resistor (FSR) Matrix Array

to measure the weight of the fruit.

## Camera
This project communicates with an ESP32 Arduino Nano equipped with:
1. ESP32-CAM with OV2640 Camera with Development Board

to take a photo of the fruit.

# Logger
The `logger` variants of the project are used to send data from the three ESP32s to a local python websocket from `LocalDataLogger.py` and via `Google API`.
They can be configured to send data periodically.

# Deployment
The `deployment` variants of the project are used to communicate with the companion app [FruitApp](https://github.com/Ianos828/FruitApp).
