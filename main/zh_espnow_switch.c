#include "stdio.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "zh_espnow.h"
#include "zh_ds18b20.h"
#include "zh_config.h"

#define ZH_GPIO_TASK_PRIORITY 3
#define ZH_GPIO_STACK_SIZE 2048
#define ZH_MESSAGE_TASK_PRIORITY 2
#define ZH_MESSAGE_STACK_SIZE 2048

static uint8_t s_relay_pin = NOT_USED;
static uint8_t s_relay_on_level = LOW;
static uint8_t s_led_pin = NOT_USED;
static uint8_t s_led_on_level = LOW;
static uint8_t s_int_button_pin = NOT_USED;
static uint8_t s_int_button_on_level = LOW;
static uint8_t s_ext_button_pin = NOT_USED;
static uint8_t s_ext_button_on_level = LOW;
static uint8_t s_ds18b20_pin = NOT_USED;

static uint8_t s_relay_status = OFF;
static bool s_gpio_processing = false;

static uint8_t s_gateway_mac[6] = {0};
static bool s_gateway_is_available = false;

static TaskHandle_t s_switch_attributes_message_task = {0};
static TaskHandle_t s_switch_config_message_task = {0};
static TaskHandle_t s_switch_keep_alive_message_task = {0};
static TaskHandle_t s_switch_status_message_task = {0};
static TaskHandle_t s_ds18b20_attributes_message_task = {0};
static TaskHandle_t s_ds18b20_config_message_task = {0};
static TaskHandle_t s_ds18b20_status_message_task = {0};
static SemaphoreHandle_t s_button_pushing_semaphore = NULL;

static esp_timer_handle_t s_gateway_availability_check_timer = {0};

static const esp_partition_t *s_update_partition = NULL;
static esp_ota_handle_t s_update_handle = 0;
static uint16_t s_ota_message_part_number = 0;

static void s_zh_load_config(void);
static void s_zh_save_config(void);
static void s_zh_load_status(void);
static void s_zh_save_status(void);

static void s_zh_gpio_init(void);
static void s_zh_gpio_set_level(void);
static void s_zh_gpio_isr_handler(void *arg);
static void s_zh_gpio_processing_task(void *pvParameter);

static void s_zh_send_switch_attributes_message_task(void *pvParameter);
static void s_zh_send_switch_config_message_task(void *pvParameter);
static void s_zh_send_switch_keep_alive_message_task(void *pvParameter);
static void s_zh_send_switch_status_message_task(void *pvParameter);
static void s_zh_send_switch_status_message(void);

static void s_zh_send_ds18b20_attributes_message_task(void *pvParameter);
static void s_zh_send_ds18b20_config_message_task(void *pvParameter);
static void s_zh_send_ds18b20_status_message_task(void *pvParameter);

static void s_zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void s_zh_set_gateway_offline_status(void);

void app_main(void)
{
#if CONFIG_RELAY_USING
    s_relay_pin = CONFIG_RELAY_PIN;
#if CONFIG_RELAY_ON_LEVEL_HIGH
    s_relay_on_level = HIGH;
#endif
#endif
#if CONFIG_LED_USING
    s_led_pin = CONFIG_LED_PIN;
#if CONFIG_LED_ON_LEVEL_HIGH
    s_led_on_level = HIGH;
#endif
#endif
#if CONFIG_INT_BUTTON_USING
    s_int_button_pin = CONFIG_INT_BUTTON_PIN;
#if CONFIG_INT_BUTTON_ON_LEVEL_HIGH
    s_int_button_on_level = HIGH;
#endif
#endif
#if CONFIG_EXT_BUTTON_USING
    s_ext_button_pin = CONFIG_EXT_BUTTON_PIN;
#if CONFIG_EXT_BUTTON_ON_LEVEL_HIGH
    s_ext_button_on_level = HIGH;
#endif
#endif
#if CONFIG_SENSOR_USING
    s_ds18b20_pin = CONFIG_SENSOR_PIN;
#endif
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    s_zh_load_config();
    s_zh_load_status();
    s_zh_gpio_init();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    zh_espnow_init_config_t zh_espnow_init_config = ZH_ESPNOW_INIT_CONFIG_DEFAULT();
    zh_espnow_init(&zh_espnow_init_config);
    esp_event_handler_register(ZH_ESPNOW, ESP_EVENT_ANY_ID, &s_zh_espnow_event_handler, NULL);
    esp_timer_create_args_t gateway_availability_check_timer_args = {
        .callback = (void *)s_zh_set_gateway_offline_status};
    esp_timer_create(&gateway_availability_check_timer_args, &s_gateway_availability_check_timer);
}

static void s_zh_load_config(void)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    uint8_t config_is_present = 0;
    if (nvs_get_u8(nvs_handle, "present", &config_is_present) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_handle, "present", 0xFE);
        nvs_close(nvs_handle);
        s_zh_save_config();
        return;
    }
    nvs_get_u8(nvs_handle, "relay_pin", &s_relay_pin);
    nvs_get_u8(nvs_handle, "relay_lvl", &s_relay_on_level);
    nvs_get_u8(nvs_handle, "led_pin", &s_led_pin);
    nvs_get_u8(nvs_handle, "led_lvl", &s_led_on_level);
    nvs_get_u8(nvs_handle, "int_btn_pin", &s_int_button_pin);
    nvs_get_u8(nvs_handle, "int_btn_lvl", &s_int_button_on_level);
    nvs_get_u8(nvs_handle, "ext_btn_pin", &s_ext_button_pin);
    nvs_get_u8(nvs_handle, "ext_btn_lvl", &s_ext_button_on_level);
    nvs_get_u8(nvs_handle, "ds18b20_pin", &s_ds18b20_pin);
    nvs_close(nvs_handle);
}

static void s_zh_save_config(void)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "relay_pin", s_relay_pin);
    nvs_set_u8(nvs_handle, "relay_lvl", s_relay_on_level);
    nvs_set_u8(nvs_handle, "led_pin", s_led_pin);
    nvs_set_u8(nvs_handle, "led_lvl", s_led_on_level);
    nvs_set_u8(nvs_handle, "int_btn_pin", s_int_button_pin);
    nvs_set_u8(nvs_handle, "int_btn_lvl", s_int_button_on_level);
    nvs_set_u8(nvs_handle, "ext_btn_pin", s_ext_button_pin);
    nvs_set_u8(nvs_handle, "ext_btn_lvl", s_ext_button_on_level);
    nvs_set_u8(nvs_handle, "ds18b20_pin", s_ds18b20_pin);
    nvs_close(nvs_handle);
}

static void s_zh_load_status(void)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("status", NVS_READWRITE, &nvs_handle);
    uint8_t status_is_present = 0;
    if (nvs_get_u8(nvs_handle, "present", &status_is_present) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_handle, "present", 0xFE);
        nvs_close(nvs_handle);
        s_zh_save_status();
        return;
    }
    nvs_get_u8(nvs_handle, "relay_state", &s_relay_status);
    nvs_close(nvs_handle);
}

static void s_zh_save_status(void)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("status", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "relay_state", s_relay_status);
    nvs_close(nvs_handle);
}

static void s_zh_gpio_init(void)
{
    gpio_config_t config = {0};
    if (s_relay_pin != NOT_USED)
    {
        config.intr_type = GPIO_INTR_DISABLE;
        config.mode = GPIO_MODE_OUTPUT;
        config.pin_bit_mask = (1ULL << s_relay_pin);
        config.pull_down_en = (s_relay_on_level == HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = (s_relay_on_level == HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        gpio_config(&config);
    }
    if (s_led_pin != NOT_USED)
    {
        config.intr_type = GPIO_INTR_DISABLE;
        config.mode = GPIO_MODE_OUTPUT;
        config.pin_bit_mask = (1ULL << s_led_pin);
        config.pull_down_en = (s_led_on_level == HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = (s_led_on_level == HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        gpio_config(&config);
    }
    if (s_int_button_pin != NOT_USED)
    {
        config.intr_type = (s_int_button_on_level == HIGH) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE;
        config.mode = GPIO_MODE_INPUT;
        config.pin_bit_mask = (1ULL << s_int_button_pin);
        config.pull_down_en = (s_int_button_on_level == HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = (s_int_button_on_level == HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        gpio_config(&config);
    }
    if (s_ext_button_pin != NOT_USED)
    {
        config.intr_type = (s_ext_button_on_level == HIGH) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE;
        config.mode = GPIO_MODE_INPUT;
        config.pin_bit_mask = (1ULL << s_ext_button_pin);
        config.pull_down_en = (s_ext_button_on_level == HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = (s_ext_button_on_level == HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        gpio_config(&config);
    }
    if (s_int_button_pin != NOT_USED || s_ext_button_pin != NOT_USED)
    {
        s_button_pushing_semaphore = xSemaphoreCreateBinary();
        xTaskCreate(&s_zh_gpio_processing_task, "s_zh_gpio_processing_task", ZH_GPIO_STACK_SIZE, NULL, ZH_GPIO_TASK_PRIORITY, NULL);
        gpio_install_isr_service(0);
        if (s_int_button_pin != NOT_USED)
        {
            gpio_isr_handler_add(s_int_button_pin, s_zh_gpio_isr_handler, NULL);
        }
        if (s_ext_button_pin != NOT_USED)
        {
            gpio_isr_handler_add(s_ext_button_pin, s_zh_gpio_isr_handler, NULL);
        }
    }
    if (s_ds18b20_pin != NOT_USED)
    {
        zh_ds18b20_init(s_ds18b20_pin);
    }
    s_zh_gpio_set_level();
}

static void s_zh_gpio_set_level(void)
{
    if (s_relay_status == ON)
    {
        gpio_set_level(s_relay_pin, (s_relay_on_level == HIGH) ? HIGH : LOW);
        if (s_led_pin != NOT_USED)
        {
            gpio_set_level(s_led_pin, (s_led_on_level == HIGH) ? HIGH : LOW);
        }
    }
    else
    {
        gpio_set_level(s_relay_pin, (s_relay_on_level == HIGH) ? LOW : HIGH);
        if (s_led_pin != NOT_USED)
        {
            gpio_set_level(s_led_pin, (s_led_on_level == HIGH) ? LOW : HIGH);
        }
    }
    s_zh_save_status();
    if (s_gateway_is_available == true)
    {
        s_zh_send_switch_status_message();
    }
}

static void IRAM_ATTR s_zh_gpio_isr_handler(void *arg)
{
    if (s_gpio_processing == false)
    {
        xSemaphoreGiveFromISR(s_button_pushing_semaphore, NULL);
    }
}

static void s_zh_gpio_processing_task(void *pvParameter)
{
    for (;;)
    {
        xSemaphoreTake(s_button_pushing_semaphore, portMAX_DELAY);
        s_gpio_processing = true;
        s_relay_status = (s_relay_status == ON) ? OFF : ON;
        s_zh_gpio_set_level();
        vTaskDelay(500 / portTICK_PERIOD_MS); // To prevent button contact rattling. Value is selected experimentally.
        s_gpio_processing = false;
    }
    vTaskDelete(NULL);
}

static void s_zh_send_switch_attributes_message_task(void *pvParameter)
{
    const esp_app_desc_t *app_info = esp_ota_get_app_description();
    zh_attributes_message_t attributes_message = {0};
    attributes_message.chip_type = HACHT_ESP8266;
    strcpy(attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    attributes_message.cpu_frequency = CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ;
    attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(attributes_message.app_name, app_info->project_name);
    strcpy(attributes_message.app_version, app_info->version);
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_ATTRIBUTES;
    for (;;)
    {
        attributes_message.heap_size = esp_get_free_heap_size();
        attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        attributes_message.uptime = esp_timer_get_time() / 1000000;
        data.payload_data = (zh_payload_data_t)attributes_message;
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_switch_config_message_task(void *pvParameter)
{
    zh_switch_config_message_t switch_config_message = {0};
    switch_config_message.unique_id = 1;
    switch_config_message.device_class = HASWDC_SWITCH;
    switch_config_message.payload_on = HAONOFT_ON;
    switch_config_message.payload_off = HAONOFT_OFF;
    switch_config_message.enabled_by_default = true;
    switch_config_message.optimistic = false;
    switch_config_message.qos = 2;
    switch_config_message.retain = true;
    zh_config_message_t config_message = {0};
    config_message = (zh_config_message_t)switch_config_message;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data = (zh_payload_data_t)config_message;
    for (;;)
    {
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(300000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_switch_keep_alive_message_task(void *pvParameter)
{
    zh_keep_alive_message_t keep_alive_message = {0};
    keep_alive_message.online_status = ONLINE;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_KEEP_ALIVE;
    data.payload_data = (zh_payload_data_t)keep_alive_message;
    for (;;)
    {
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_switch_status_message_task(void *pvParameter)
{
    zh_switch_status_message_t switch_status_message = {0};
    zh_status_message_t status_message = {0};
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_STATE;
    for (;;)
    {
        switch_status_message.status = (s_relay_status == ON) ? HAONOFT_ON : HAONOFT_OFF;
        status_message = (zh_status_message_t)switch_status_message;
        data.payload_data = (zh_payload_data_t)status_message;
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(300000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_switch_status_message(void)
{
    zh_switch_status_message_t switch_status_message = {0};
    switch_status_message.status = (s_relay_status == ON) ? HAONOFT_ON : HAONOFT_OFF;
    zh_status_message_t status_message = {0};
    status_message = (zh_status_message_t)switch_status_message;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_STATE;
    data.payload_data = (zh_payload_data_t)status_message;
    zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

static void s_zh_send_ds18b20_attributes_message_task(void *pvParameter)
{
    const esp_app_desc_t *app_info = esp_ota_get_app_description();
    zh_attributes_message_t attributes_message = {0};
    attributes_message.chip_type = HACHT_ESP8266;
    attributes_message.sensor_type = HAST_DS18B20;
    strcpy(attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    attributes_message.cpu_frequency = CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ;
    attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(attributes_message.app_name, app_info->project_name);
    strcpy(attributes_message.app_version, app_info->version);
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_ATTRIBUTES;
    for (;;)
    {
        attributes_message.heap_size = esp_get_free_heap_size();
        attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        attributes_message.uptime = esp_timer_get_time() / 1000000;
        data.payload_data = (zh_payload_data_t)attributes_message;
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_ds18b20_config_message_task(void *pvParameter)
{
    zh_sensor_config_message_t sensor_config_message = {0};
    sensor_config_message.unique_id = 2;
    sensor_config_message.sensor_device_class = HASDC_TEMPERATURE;
    char *unit_of_measurement = "°C";
    strcpy(sensor_config_message.unit_of_measurement, unit_of_measurement);
    sensor_config_message.suggested_display_precision = 1;
    sensor_config_message.expire_after = 180;
    sensor_config_message.enabled_by_default = true;
    sensor_config_message.force_update = true;
    sensor_config_message.qos = 2;
    sensor_config_message.retain = true;
    zh_config_message_t config_message = {0};
    config_message = (zh_config_message_t)sensor_config_message;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data = (zh_payload_data_t)config_message;
    for (;;)
    {
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(300000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_send_ds18b20_status_message_task(void *pvParameter)
{
    float temperature = 0;
    zh_sensor_status_message_t sensor_status_message = {0};
    sensor_status_message.sensor_type = HAST_DS18B20;
    zh_status_message_t status_message = {0};
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_STATE;
    for (;;)
    {
        if (zh_ds18b20_read_temp(NULL, &temperature) == ESP_OK)
        {
            sensor_status_message.temperature = temperature;
        }
        status_message = (zh_status_message_t)sensor_status_message;
        data.payload_data = (zh_payload_data_t)status_message;
        zh_espnow_send(s_gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void s_zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const esp_app_desc_t *app_info = esp_ota_get_app_description();
    zh_espnow_data_t data_in = {0};
    zh_espnow_data_t data_out = {0};
    zh_espnow_ota_message_t espnow_ota_message = {0};
    data_out.device_type = ZHDT_SWITCH;
    espnow_ota_message.chip_type = HACHT_ESP8266;
    data_out.payload_data = (zh_payload_data_t)espnow_ota_message;
    switch (event_id)
    {
    case ZH_ESPNOW_ON_RECV_EVENT:;
        zh_espnow_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_ESPNOW_EVENT_HANDLER_EXIT;
        }
        memcpy(&data_in, recv_data->data, recv_data->data_len);
        switch (data_in.device_type)
        {
        case ZHDT_GATEWAY:
            switch (data_in.payload_type)
            {
            case ZHPT_KEEP_ALIVE:
                if (data_in.payload_data.keep_alive_message.online_status == ONLINE)
                {
                    if (s_gateway_is_available == false)
                    {
                        s_gateway_is_available = true;
                        memcpy(s_gateway_mac, recv_data->mac_addr, 6);
                        if (s_relay_pin != NOT_USED)
                        {
                            xTaskCreate(&s_zh_send_switch_config_message_task, "s_zh_send_switch_config_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_switch_config_message_task);
                            xTaskCreate(&s_zh_send_switch_status_message_task, "s_zh_send_switch_status_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_switch_status_message_task);
                            xTaskCreate(&s_zh_send_switch_attributes_message_task, "s_zh_send_switch_attributes_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_switch_attributes_message_task);
                            xTaskCreate(&s_zh_send_switch_keep_alive_message_task, "s_zh_send_switch_keep_alive_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_switch_keep_alive_message_task);
                        }
                        if (s_ds18b20_pin != NOT_USED)
                        {
                            xTaskCreate(&s_zh_send_ds18b20_config_message_task, "s_zh_send_ds18b20_config_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_ds18b20_config_message_task);
                            xTaskCreate(&s_zh_send_ds18b20_status_message_task, "s_zh_send_ds18b20_status_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_ds18b20_status_message_task);
                            xTaskCreate(&s_zh_send_ds18b20_attributes_message_task, "s_zh_send_ds18b20_attributes_message_task", ZH_MESSAGE_STACK_SIZE, NULL, ZH_MESSAGE_TASK_PRIORITY, &s_ds18b20_attributes_message_task);
                        }
                    }
                    if (esp_timer_start_once(s_gateway_availability_check_timer, data_in.payload_data.keep_alive_message.message_frequency * 3000000) == ESP_ERR_INVALID_STATE)
                    {
                        esp_timer_stop(s_gateway_availability_check_timer);
                        esp_timer_start_once(s_gateway_availability_check_timer, data_in.payload_data.keep_alive_message.message_frequency * 3000000);
                    }
                }
                else
                {
                    if (s_gateway_is_available == true)
                    {
                        esp_timer_stop(s_gateway_availability_check_timer);
                        s_zh_set_gateway_offline_status();
                    }
                }
                break;
            case ZHPT_SET:
                s_relay_status = (data_in.payload_data.status_message.switch_status_message.status == HAONOFT_ON) ? ON : OFF;
                s_zh_gpio_set_level();
                break;
            case ZHPT_UPDATE:
                s_update_partition = esp_ota_get_next_update_partition(NULL);
                char *app_name = (char *)calloc(1, strlen(app_info->project_name) + 5 + 1);
                sprintf(app_name, "%s.app%d", app_info->project_name, s_update_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0 + 1);
                strcpy(espnow_ota_message.app_name, app_name);
                free(app_name);
                strcpy(espnow_ota_message.app_version, app_info->version);
                data_out.payload_type = ZHPT_UPDATE;
                data_out.payload_data = (zh_payload_data_t)espnow_ota_message;
                zh_espnow_send(s_gateway_mac, (uint8_t *)&data_out, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_BEGIN:
                esp_ota_begin(s_update_partition, OTA_SIZE_UNKNOWN, &s_update_handle);
                s_ota_message_part_number = 1;
                break;
            case ZHPT_UPDATE_PROGRESS:
                if (s_ota_message_part_number == data_in.payload_data.espnow_ota_message.part)
                {
                    ++s_ota_message_part_number;
                    esp_ota_write(s_update_handle, (const void *)data_in.payload_data.espnow_ota_message.data, data_in.payload_data.espnow_ota_message.data_len);
                }
                data_out.payload_type = ZHPT_UPDATE_PROGRESS;
                zh_espnow_send(s_gateway_mac, (uint8_t *)&data_out, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_ERROR:
                esp_ota_end(s_update_handle);
                break;
            case ZHPT_UPDATE_END:
                if (esp_ota_end(s_update_handle) != ESP_OK)
                {
                    data_out.payload_type = ZHPT_UPDATE_FAIL;
                    zh_espnow_send(s_gateway_mac, (uint8_t *)&data_out, sizeof(zh_espnow_data_t));
                    break;
                }
                esp_ota_set_boot_partition(s_update_partition);
                data_out.payload_type = ZHPT_UPDATE_SUCCESS;
                zh_espnow_send(s_gateway_mac, (uint8_t *)&data_out, sizeof(zh_espnow_data_t));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
                break;
            case ZHPT_RESTART:
                esp_restart();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    ZH_ESPNOW_EVENT_HANDLER_EXIT:
        free(recv_data->data); // Do not delete for prevent of memory leakage!!!
        break;
    case ZH_ESPNOW_ON_SEND_EVENT:
        break;
    default:
        break;
    }
}

static void s_zh_set_gateway_offline_status(void)
{
    s_gateway_is_available = false;
    if (s_relay_pin != NOT_USED)
    {
        vTaskDelete(s_switch_attributes_message_task);
        vTaskDelete(s_switch_keep_alive_message_task);
        vTaskDelete(s_switch_status_message_task);
        vTaskDelete(s_switch_config_message_task);
    }
    if (s_ds18b20_pin != NOT_USED)
    {
        vTaskDelete(s_ds18b20_attributes_message_task);
        vTaskDelete(s_ds18b20_status_message_task);
        vTaskDelete(s_ds18b20_config_message_task);
    }
}