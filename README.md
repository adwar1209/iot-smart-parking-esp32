# iot-smart-parking-esp32
ESP32-based IoT smart parking system using ESP-IDF, FreeRTOS, ultrasonic sensors, Wi-Fi, and MQTT for real-time parking occupancy monitoring and cloud data transmission.

An IoT-based smart parking system developed using ESP32 and ESP-IDF.

## Features

- Four parking-slot monitoring using HC-SR04 ultrasonic sensors
- ESP-IDF based firmware
- FreeRTOS parking monitoring task
- Wi-Fi connectivity
- MQTT communication
- Real-time parking occupancy updates
- Available-slot calculation
- Cloud data monitoring using MQTTX
- Simulated using Wokwi

## System Architecture

HC-SR04 Sensors
        ->
ESP32 + ESP-IDF
        ->
Parking Occupancy Logic
        ->
Wi-Fi
        ->
MQTT Broker
        ->
MQTTX Subscriber
