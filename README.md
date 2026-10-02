# IoT Smart Parking System using ESP32 and ESP-IDF

ESP32-based IoT smart parking system using ESP-IDF, FreeRTOS, ultrasonic sensors, Wi-Fi, and MQTT for real-time parking occupancy monitoring and cloud data transmission.

## Features

- Four parking-slot monitoring using HC-SR04 ultrasonic sensors
- Native ESP-IDF firmware
- FreeRTOS parking monitoring task
- Wi-Fi connectivity using Wokwi-GUEST
- MQTT communication using the EMQX public broker
- Real-time parking occupancy updates
- Available-slot calculation
- Cloud data monitoring using MQTTX
- Wokwi simulation

## Architecture

HC-SR04 Sensors  → ESP32 + ESP-IDF  → Parking Occupancy Logic  → Wi-Fi  → MQTT Broker  → MQTTX Subscriber


## Example Payload

```json
{
  "slot1": 1,
  "slot2": 0,
  "slot3": 1,
  "slot4": 0,
  "available": 2,
  "total": 4
}
```

- `1` = occupied
- `0` = free

## Pin Mapping

| Slot | TRIG | ECHO |
|---|---:|---:|
| 1 | GPIO 5 | GPIO 18 |
| 2 | GPIO 19 | GPIO 21 |
| 3 | GPIO 22 | GPIO 23 |
| 4 | GPIO 25 | GPIO 26 |

## Technologies

- ESP32
- ESP-IDF
- FreeRTOS
- HC-SR04
- Wi-Fi
- MQTT
- EMQX
- MQTTX
- Wokwi

## Run

1. Build the ESP-IDF project.
2. Start the Wokwi simulation.
3. Connect MQTTX to the EMQX public broker.
4. Subscribe to `shivaraj/smartparking/status`.
5. Change HC-SR04 distances in Wokwi to simulate vehicles entering or leaving slots.
