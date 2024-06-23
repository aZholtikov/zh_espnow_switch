#pragma once

#include "stdio.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "zh_ds18b20.h"
#include "zh_dht.h"
#include "zh_config.h"

#ifdef CONFIG_NETWORK_TYPE_DIRECT
#include "zh_espnow.h"
#define zh_send_message(a, b, c) zh_espnow_send(a, b, c)
#define ZH_EVENT ZH_ESPNOW
#else
#include "zh_network.h"
#define zh_send_message(a, b, c) zh_network_send(a, b, c)
#define ZH_EVENT ZH_NETWORK
#endif

#ifdef CONFIG_IDF_TARGET_ESP8266
#define ZH_CHIP_TYPE HACHT_ESP8266
#elif CONFIG_IDF_TARGET_ESP32
#define ZH_CHIP_TYPE HACHT_ESP32
#elif CONFIG_IDF_TARGET_ESP32S2
#define ZH_CHIP_TYPE HACHT_ESP32S2
#elif CONFIG_IDF_TARGET_ESP32S3
#define ZH_CHIP_TYPE HACHT_ESP32S3
#elif CONFIG_IDF_TARGET_ESP32C2
#define ZH_CHIP_TYPE HACHT_ESP32C2
#elif CONFIG_IDF_TARGET_ESP32C3
#define ZH_CHIP_TYPE HACHT_ESP32C3
#elif CONFIG_IDF_TARGET_ESP32C6
#define ZH_CHIP_TYPE HACHT_ESP32C6
#endif

#ifdef CONFIG_IDF_TARGET_ESP8266
#define ZH_CPU_FREQUENCY CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ;
#define get_app_description() esp_ota_get_app_description()
#else
#define ZH_CPU_FREQUENCY CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#define get_app_description() esp_app_get_description()
#endif

#define ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY 10 // Frequency of sending a switch keep alive message to the gateway (in seconds).
#define ZH_SWITCH_ATTRIBUTES_MESSAGE_FREQUENCY 60 // Frequency of sending a switch attributes message to the gateway (in seconds).
#define ZH_SENSOR_STATUS_MESSAGE_FREQUENCY 300    // // Frequency of sending a sensor status message to the gateway (in seconds).
#define ZH_SENSOR_ATTRIBUTES_MESSAGE_FREQUENCY 60 // Frequency of sending a sensor attributes message to the gateway (in seconds).

#define ZH_GPIO_TASK_PRIORITY 3    // Prioritize the task of GPIO processing.
#define ZH_GPIO_STACK_SIZE 2048    // The stack size of the task of GPIO processing.
#define ZH_MESSAGE_TASK_PRIORITY 2 // Prioritize the task of sending messages to the gateway.
#define ZH_MESSAGE_STACK_SIZE 2048 // The stack size of the task of sending messages to the gateway.

typedef struct // Structure of data exchange between tasks, functions and event handlers.
{
    struct // Storage structure of switch hardware configuration data.
    {
        uint8_t relay_pin;            // Relay GPIO number.
        bool relay_on_level;          // Relay ON level. @note HIGH (true) / LOW (false).
        uint8_t led_pin;              // Led GPIO number (if present).
        bool led_on_level;            // Led ON level (if present). @note HIGH (true) / LOW (false).
        uint8_t int_button_pin;       // Internal button GPIO number (if present).
        bool int_button_on_level;     // Internal button trigger level (if present). @note HIGH (true) / LOW (false).
        uint8_t ext_button_pin;       // External button GPIO number (if present).
        bool ext_button_on_level;     // External button trigger level (if present). @note HIGH (true) / LOW (false).
        uint8_t sensor_pin;           // Sensor GPIO number (if present).
        ha_sensor_type_t sensor_type; // Sensor types (if present). @note Used to identify the sensor type by ESP-NOW gateway and send the appropriate sensor status messages to MQTT.
    } hardware_config;
    struct // Storage structure of switch status data.
    {
        ha_on_off_type_t status; // Status of the zh_espnow_switch. @note Example - ON / OFF. @attention Must be same with set on switch_config_message structure.
    } status;
    volatile bool gpio_processing;               // GPIO processing flag. @note Used to prevent a repeated interrupt from triggering during GPIO processing.
    volatile bool gateway_is_available;          // Gateway availability status flag. @note Used to control the tasks when the gateway connection is established / lost.
    uint8_t gateway_mac[6];                      // Gateway MAC address.
    TaskHandle_t switch_attributes_message_task; // Unique task handle for zh_send_switsh_attributes_message_task().
    TaskHandle_t switch_keep_alive_message_task; // Unique task handle for zh_send_switch_keep_alive_message_task().
    TaskHandle_t sensor_attributes_message_task; // Unique task handle for zh_send_sensor_attributes_message_task().
    TaskHandle_t sensor_status_message_task;     // Unique task handle for zh_send_sensor_status_message_task().
    SemaphoreHandle_t button_pushing_semaphore;  // Unique semaphore handle for GPIO processing tasks.
    const esp_partition_t *update_partition;     // Next update partition.
    esp_ota_handle_t update_handle;              // Unique OTA handle.
    uint16_t ota_message_part_number;            // The sequence number of the firmware upgrade part. @note Used to verify that all parts have been received.
} switch_config_t;

/**
 * @brief Function for loading the switch hardware configuration from NVS memory.
 *
 * @param[out] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_load_config(switch_config_t *switch_config);

/**
 * @brief Function for saving the switch hardware configuration to NVS memory.
 *
 * @param[in] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_save_config(const switch_config_t *switch_config);

/**
 * @brief Function for loading the switch status from NVS memory.
 *
 * @param[out] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_load_status(switch_config_t *switch_config);

/**
 * @brief Function for saving the switch status to NVS memory.
 *
 * @param[out] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_save_status(const switch_config_t *switch_config);

/**
 * @brief Function for GPIO and sensor initialization.
 *
 * @param[in,out] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_gpio_init(switch_config_t *switch_config);

/**
 * @brief Function for changing the relay and led status.
 *
 * @param[in,out] switch_config Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_gpio_set_level(switch_config_t *switch_config);

/**
 * @brief Function for button pushing interrupt.
 *
 * @param[in,out] arg Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_gpio_isr_handler(void *arg);

/**
 * @brief Task for button pushing processing.
 *
 * @param[in,out] pvParameter Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_gpio_processing_task(void *pvParameter);

/**
 * @brief Task for prepare the switch attributes message and sending it to the gateway.
 *
 * @param[in,out] pvParameter Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_switch_attributes_message_task(void *pvParameter);

/**
 * @brief Function for prepare the switch configuration message and sending it to the gateway.
 *
 * @param[in] switch_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_switch_config_message(const switch_config_t *switch_config);

/**
 * @brief Function for prepare the sensor hardware configuration message and sending it to the gateway.
 *
 * @param[in] switch_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_switch_hardware_config_message(const switch_config_t *switch_config);

/**
 * @brief Task for prepare the sensor keep alive message and sending it to the gateway.
 *
 * @param[in] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_switch_keep_alive_message_task(void *pvParameter);

/**
 * @brief Function for prepare the switch status message and sending it to the gateway.
 *
 * @param[in] switch_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_switch_status_message(const switch_config_t *switch_config);

/**
 * @brief Function for prepare the sensor configuration message and sending it to the gateway.
 *
 * @param[in] switch_config Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_sensor_config_message(const switch_config_t *switch_config);

/**
 * @brief Task for prepare the sensor attributes message and sending it to the gateway.
 *
 * @param[in] pvParameter Pointer to structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_sensor_attributes_message_task(void *pvParameter);

/**
 * @brief Task for prepare the sensor status message and sending it to the gateway.
 *
 * @param[in] pvParameter Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_send_sensor_status_message_task(void *pvParameter);

/**
 * @brief Function for ESP-NOW event processing.
 *
 * @param[in,out] arg Pointer to the structure of data exchange between tasks, functions and event handlers.
 */
void zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);