#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mqtt_client.h"

#define NUM_SLOTS 4
#define OCCUPIED_THRESHOLD_CM 50.0f

#define WIFI_SSID "Wokwi-GUEST"

#define MQTT_BROKER_URI "mqtt://broker.emqx.io:1883"
#define MQTT_TOPIC "shivaraj/smartparking/status"

static const char *WIFI_TAG = "WIFI";
static const char *MQTT_TAG = "MQTT";

typedef struct
{
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
    float distance_cm;
    bool occupied;
} parking_slot_t;

static parking_slot_t slots[NUM_SLOTS] =
{
    {GPIO_NUM_5,  GPIO_NUM_18, 0, false},
    {GPIO_NUM_19, GPIO_NUM_21, 0, false},
    {GPIO_NUM_22, GPIO_NUM_23, 0, false},
    {GPIO_NUM_25, GPIO_NUM_26, 0, false}
};

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void ultrasonic_init(gpio_num_t trig, gpio_num_t echo)
{
    gpio_config_t trig_config = {
        .pin_bit_mask = (1ULL << trig),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&trig_config);

    gpio_config_t echo_config = {
        .pin_bit_mask = (1ULL << echo),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&echo_config);
    gpio_set_level(trig, 0);
}

static float ultrasonic_read(gpio_num_t trig, gpio_num_t echo)
{
    gpio_set_level(trig, 0);
    esp_rom_delay_us(2);

    gpio_set_level(trig, 1);
    esp_rom_delay_us(10);
    gpio_set_level(trig, 0);

    int64_t timeout = esp_timer_get_time();

    while (gpio_get_level(echo) == 0)
    {
        if (esp_timer_get_time() - timeout > 30000)
        {
            return -1.0f;
        }
    }

    int64_t start = esp_timer_get_time();

    while (gpio_get_level(echo) == 1)
    {
        if (esp_timer_get_time() - start > 30000)
        {
            return -1.0f;
        }
    }

    int64_t end = esp_timer_get_time();
    int64_t pulse_width = end - start;

    return (pulse_width * 0.0343f) / 2.0f;
}

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "Connected to MQTT broker");
            mqtt_connected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(MQTT_TAG, "Disconnected from MQTT broker");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "Message published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(MQTT_TAG, "MQTT error");
            break;

        default:
            break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));

    ESP_LOGI(MQTT_TAG, "MQTT client started");
}

static void publish_parking_status(int available_slots)
{
    if (!mqtt_connected)
    {
        ESP_LOGW(MQTT_TAG, "MQTT not connected - skipping publish");
        return;
    }

    char payload[256];

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"slot1\":%d,"
        "\"slot2\":%d,"
        "\"slot3\":%d,"
        "\"slot4\":%d,"
        "\"available\":%d,"
        "\"total\":%d"
        "}",
        slots[0].occupied ? 1 : 0,
        slots[1].occupied ? 1 : 0,
        slots[2].occupied ? 1 : 0,
        slots[3].occupied ? 1 : 0,
        available_slots,
        NUM_SLOTS
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC,
        payload,
        0,
        1,
        0
    );

    ESP_LOGI(MQTT_TAG, "Published: %s", payload);
    ESP_LOGI(MQTT_TAG, "msg_id=%d", msg_id);
}

static void parking_task(void *arg)
{
    while (1)
    {
        int available_slots = 0;

        printf("\n-----------------------------\n");
        printf("SMART PARKING STATUS\n");
        printf("-----------------------------\n");

        for (int i = 0; i < NUM_SLOTS; i++)
        {
            slots[i].distance_cm =
                ultrasonic_read(
                    slots[i].trig_pin,
                    slots[i].echo_pin
                );

            if (slots[i].distance_cm < 0)
            {
                printf("Slot %d : SENSOR ERROR\n", i + 1);
                continue;
            }

            slots[i].occupied =
                slots[i].distance_cm < OCCUPIED_THRESHOLD_CM;

            printf(
                "Slot %d | %.2f cm | %s\n",
                i + 1,
                slots[i].distance_cm,
                slots[i].occupied ? "OCCUPIED" : "FREE"
            );

            if (!slots[i].occupied)
            {
                available_slots++;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        printf("-----------------------------\n");
        printf(
            "Available Slots: %d / %d\n",
            available_slots,
            NUM_SLOTS
        );

        publish_parking_status(available_slots);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(WIFI_TAG, "WiFi started. Connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(WIFI_TAG, "WiFi disconnected. Reconnecting...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(WIFI_TAG, "Connected!");
        ESP_LOGI(
            WIFI_TAG,
            "IP Address: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .threshold.authmode = WIFI_AUTH_OPEN
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "WiFi initialization complete");
}

void app_main(void)
{
    printf("\n");
    printf("IoT Smart Parking System\n");
    printf("========================\n");

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    for (int i = 0; i < NUM_SLOTS; i++)
    {
        ultrasonic_init(
            slots[i].trig_pin,
            slots[i].echo_pin
        );
    }

    wifi_init();
    mqtt_init();

    xTaskCreate(
        parking_task,
        "parking_task",
        4096,
        NULL,
        5,
        NULL
    );
}
