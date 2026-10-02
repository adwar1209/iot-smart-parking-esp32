# IoT Smart Parking System using ESP32

This project is a small IoT-based parking monitor built with an ESP32, ESP-IDF, FreeRTOS, ultrasonic sensors, Wi-Fi, and MQTT.

The idea is simple: each parking slot has an ultrasonic sensor. The ESP32 checks the distance reported by each sensor, decides whether the slot is free or occupied, counts how many spaces are available, and sends the latest parking status to the cloud using MQTT.

I built and tested the whole setup in Wokwi, so the project can be demonstrated without needing the physical hardware.

## What the project does

- Monitors four parking slots using HC-SR04 ultrasonic sensors
- Detects whether each slot is free or occupied
- Counts the number of available parking spaces
- Runs the monitoring logic as a FreeRTOS task
- Connects the ESP32 to Wi-Fi
- Publishes parking data using MQTT
- Uses the EMQX public broker for testing
- Lets the parking status be viewed live in MQTTX
- Runs completely in Wokwi simulation

## How it works

The data flow is:

```text
HC-SR04 Sensors
      |
      v
ESP32
      |
      v
Parking Occupancy Logic
      |
      v
Wi-Fi
      |
      v
MQTT
      |
      v
EMQX Broker
      |
      v
MQTTX Subscriber
```

Each sensor measures the distance to the nearest object.

If the measured distance is below the configured threshold, the slot is treated as occupied. Otherwise, it is treated as free.

The ESP32 repeats this process for all four slots and then publishes the updated result over MQTT.

## MQTT topic

```text
shivaraj/smartparking/status
```

## Example MQTT message

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

In the payload:

- `1` means the parking slot is occupied
- `0` means the parking slot is free

## Pin connections

| Parking Slot | TRIG | ECHO |
|---|---:|---:|
| Slot 1 | GPIO 5 | GPIO 18 |
| Slot 2 | GPIO 19 | GPIO 21 |
| Slot 3 | GPIO 22 | GPIO 23 |
| Slot 4 | GPIO 25 | GPIO 26 |

## Tools and technologies

- ESP32
- ESP-IDF
- FreeRTOS
- HC-SR04 ultrasonic sensors
- Wi-Fi
- MQTT
- EMQX
- MQTTX
- Wokwi

## Running the project

1. Build the project using ESP-IDF.
2. Start the Wokwi simulation.
3. Wait for the ESP32 to connect to `Wokwi-GUEST`.
4. Open MQTTX and connect to the EMQX public broker.
5. Subscribe to:

   ```text
   shivaraj/smartparking/status
   ```

6. Change the HC-SR04 distances in Wokwi to simulate cars entering or leaving parking spaces.
7. Watch the MQTT payload update in real time.

## Why I built this

I wanted to build a project that combines embedded firmware with a real IoT communication flow instead of stopping at local sensor readings.

This project helped me work with GPIO, timing, FreeRTOS tasks, Wi-Fi, MQTT, structured sensor data, and cloud communication using ESP-IDF.

## Future improvements

Some possible next steps are:

- Add sensor filtering and hysteresis
- Publish data only when a slot changes state
- Add entry and exit tracking
- Add alerts when the parking area is full
- Add a dedicated cloud backend or database
- Test the same firmware on physical ESP32 hardware
