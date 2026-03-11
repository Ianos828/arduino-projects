# Arduino
This repo contains three Arduino projects, namely:
1. `SensorArray`
2. `ForceSensor`
3. `Camera`
   
split into `logger` and `deployment` versions. This repository was used to measure the freshness of fruits, by taking a measurement of a fruit's attributes.

## SensorArray
This project communicates with an ESP32 Arduino Nano equipped with:
1. [DGS2 CH4 sensor from SPEC Sensors](https://www.spec-sensors.com/product/dgs2/)
2. [SGP40 Air Quality Sensor from Adafruit](https://learn.adafruit.com/adafruit-sgp40)
3. [MQ3](https://sg.element14.com/seeed-studio/101020006/gas-sensor-module-arduino-board/dp/4007654), [MQ4](https://sg.element14.com/dfrobot/sen0129/analogue-ch4-gas-sensor-arduino/dp/3517900) and [MQ5](https://sg.element14.com/seeed-studio/101020056/sensor-module-arduino-raspberry/dp/4007685) sensors for Arduino
4. [OPT101P](https://www.digikey.sg/en/products/detail/texas-instruments/OPT101P/251177)
5. [SparkFun Triad Spectroscopy Sensor - AS7265x (Qwiic)](https://www.sparkfun.com/sparkfun-triad-spectroscopy-sensor-as7265x-qwiic.html)
6. [SparkFun Distance Sensor Breakout - 4 Meter, VL53L1X (Qwiic)](https://www.sparkfun.com/sparkfun-distance-sensor-breakout-4-meter-vl53l1x-qwiic.html)

to measure the Voltile Organic Compounds (VOCs), fluorescence, NIR and LiDAR readings of the fruit.

## ForceSensor
This project communicates with an ESP32 Arduino Nano equipped with:
1. [Sensitronic ThruMode Force Sensor Resistor (FSR) Matrix Array]()

to measure the weight of the fruit.

## Camera
This project communicates with an ESP32 Arduino Nano equipped with:
1. [ESP32-CAM with OV2640 Camera with Development Board]()

to take a photo of the fruit.

# Logger
The `logger` variants of the project are used to send data from the three ESP32s to a local python websocket from `LocalDataLogger.py` and via `Google API`.
They can be configured to send data periodically.

# Deployment
The `deployment` variants of the project are used to communicate with the companion app [FruitApp](https://github.com/Ianos828/FruitApp).

