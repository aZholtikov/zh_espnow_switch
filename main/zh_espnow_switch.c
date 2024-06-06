#include "zh_espnow_switch.h"

switch_config_t switch_main_config = {0};

void app_main(void)
{
    switch_config_t *switch_config = &switch_main_config;
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    zh_load_config(switch_config);
    zh_load_status(switch_config);
    zh_gpio_init(switch_config);
    zh_gpio_set_level(switch_config);
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
    esp_wifi_start();
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    zh_espnow_init_config_t espnow_init_config = ZH_ESPNOW_INIT_CONFIG_DEFAULT();
    zh_espnow_init(&espnow_init_config);
#else
    zh_network_init_config_t network_init_config = ZH_NETWORK_INIT_CONFIG_DEFAULT();
    zh_network_init(&network_init_config);
#endif
#ifdef CONFIG_IDF_TARGET_ESP8266
    esp_event_handler_register(ZH_EVENT, ESP_EVENT_ANY_ID, &zh_espnow_event_handler, switch_config);
#else
    esp_event_handler_instance_register(ZH_EVENT, ESP_EVENT_ANY_ID, &zh_espnow_event_handler, switch_config, NULL);
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = {0};
    esp_ota_get_state_partition(running, &ota_state);
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        vTaskDelay(60000 / portTICK_PERIOD_MS);
        esp_ota_mark_app_valid_cancel_rollback();
    }
#endif
}

void zh_load_config(switch_config_t *switch_config)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    uint8_t config_is_present = 0;
    if (nvs_get_u8(nvs_handle, "present", &config_is_present) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_handle, "present", 0xFE);
        nvs_close(nvs_handle);
#ifdef CONFIG_RELAY_USING
        switch_config->hardware_config.relay_pin = CONFIG_RELAY_PIN;
#else
        switch_config->hardware_config.relay_pin = ZH_NOT_USED;
#endif
#ifdef CONFIG_RELAY_ON_LEVEL_HIGH
        switch_config->hardware_config.relay_on_level = ZH_HIGH;
#else
        switch_config->hardware_config.relay_on_level = ZH_LOW;
#endif
#ifdef CONFIG_LED_USING
        switch_config->hardware_config.led_pin = CONFIG_LED_PIN;
#else
        switch_config->hardware_config.led_pin = ZH_NOT_USED;
#endif
#ifdef CONFIG_LED_ON_LEVEL_HIGH
        switch_config->hardware_config.led_on_level = ZH_HIGH;
#else
        switch_config->hardware_config.led_on_level = ZH_LOW;
#endif
#ifdef CONFIG_INT_BUTTON_USING
        switch_config->hardware_config.int_button_pin = CONFIG_INT_BUTTON_PIN;
#else
        switch_config->hardware_config.int_button_pin = ZH_NOT_USED;
#endif
#ifdef CONFIG_INT_BUTTON_ON_LEVEL_HIGH
        switch_config->hardware_config.int_button_on_level = ZH_HIGH;
#else
        switch_config->hardware_config.int_button_on_level = ZH_LOW;
#endif
#ifdef CONFIG_EXT_BUTTON_USING
        switch_config->hardware_config.ext_button_pin = CONFIG_EXT_BUTTON_PIN;
#else
        switch_config->hardware_config.ext_button_pin = ZH_NOT_USED;
#endif
#ifdef CONFIG_EXT_BUTTON_ON_LEVEL_HIGH
        switch_config->hardware_config.ext_button_on_level = ZH_HIGH;
#else
        switch_config->hardware_config.ext_button_on_level = ZH_LOW;
#endif
#ifdef CONFIG_SENSOR_TYPE_DS18B20
        switch_config->hardware_config.sensor_pin = CONFIG_SENSOR_PIN;
        switch_config->hardware_config.sensor_type = HAST_DS18B20;
#elif CONFIG_SENSOR_TYPE_DHT11
        switch_config->hardware_config.sensor_pin = CONFIG_SENSOR_PIN;
        switch_config->hardware_config.sensor_type = HAST_DHT11;
#elif CONFIG_SENSOR_TYPE_DHT22
        switch_config->hardware_config.sensor_pin = CONFIG_SENSOR_PIN;
        switch_config->hardware_config.sensor_type = HAST_DHT22;
#else
        switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
        switch_config->hardware_config.sensor_type = HAST_NONE;
#endif
        zh_save_config(switch_config);
        return;
    }
    nvs_get_u8(nvs_handle, "relay_pin", &switch_config->hardware_config.relay_pin);
    nvs_get_u8(nvs_handle, "relay_lvl", (uint8_t *)&switch_config->hardware_config.relay_on_level);
    nvs_get_u8(nvs_handle, "led_pin", &switch_config->hardware_config.led_pin);
    nvs_get_u8(nvs_handle, "led_lvl", (uint8_t *)&switch_config->hardware_config.led_on_level);
    nvs_get_u8(nvs_handle, "int_btn_pin", &switch_config->hardware_config.int_button_pin);
    nvs_get_u8(nvs_handle, "int_btn_lvl", (uint8_t *)&switch_config->hardware_config.int_button_on_level);
    nvs_get_u8(nvs_handle, "ext_btn_pin", &switch_config->hardware_config.ext_button_pin);
    nvs_get_u8(nvs_handle, "ext_btn_lvl", (uint8_t *)&switch_config->hardware_config.ext_button_on_level);
    nvs_get_u8(nvs_handle, "sensor_pin", &switch_config->hardware_config.sensor_pin);
    nvs_get_u8(nvs_handle, "sensor_type", (uint8_t *)&switch_config->hardware_config.sensor_type);
    nvs_close(nvs_handle);
}

void zh_save_config(const switch_config_t *switch_config)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("config", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "relay_pin", switch_config->hardware_config.relay_pin);
    nvs_set_u8(nvs_handle, "relay_lvl", switch_config->hardware_config.relay_on_level);
    nvs_set_u8(nvs_handle, "led_pin", switch_config->hardware_config.led_pin);
    nvs_set_u8(nvs_handle, "led_lvl", switch_config->hardware_config.led_on_level);
    nvs_set_u8(nvs_handle, "int_btn_pin", switch_config->hardware_config.int_button_pin);
    nvs_set_u8(nvs_handle, "int_btn_lvl", switch_config->hardware_config.int_button_on_level);
    nvs_set_u8(nvs_handle, "ext_btn_pin", switch_config->hardware_config.ext_button_pin);
    nvs_set_u8(nvs_handle, "ext_btn_lvl", switch_config->hardware_config.ext_button_on_level);
    nvs_set_u8(nvs_handle, "sensor_pin", switch_config->hardware_config.sensor_pin);
    nvs_set_u8(nvs_handle, "sensor_type", switch_config->hardware_config.sensor_type);
    nvs_close(nvs_handle);
}

void zh_load_status(switch_config_t *switch_config)
{
    switch_config->status.status = HAONOFT_OFF;
    nvs_handle_t nvs_handle = 0;
    nvs_open("status", NVS_READWRITE, &nvs_handle);
    uint8_t status_is_present = 0;
    if (nvs_get_u8(nvs_handle, "present", &status_is_present) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_handle, "present", 0xFE);
        nvs_close(nvs_handle);
        zh_save_status(switch_config);
        return;
    }
    nvs_get_u8(nvs_handle, "relay_state", (uint8_t *)&switch_config->status.status);
    nvs_close(nvs_handle);
}

void zh_save_status(const switch_config_t *switch_config)
{
    nvs_handle_t nvs_handle = 0;
    nvs_open("status", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "relay_state", switch_config->status.status);
    nvs_close(nvs_handle);
}

void zh_gpio_init(switch_config_t *switch_config)
{
    gpio_config_t config = {0};
    if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
    {
        config.intr_type = GPIO_INTR_DISABLE;
        config.mode = GPIO_MODE_OUTPUT;
        config.pin_bit_mask = (1ULL << switch_config->hardware_config.relay_pin);
        config.pull_down_en = (switch_config->hardware_config.relay_on_level == ZH_HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = (switch_config->hardware_config.relay_on_level == ZH_HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
        if (gpio_config(&config) != ESP_OK)
        {
            switch_config->hardware_config.relay_pin = ZH_NOT_USED;
            switch_config->hardware_config.led_pin = ZH_NOT_USED;
            switch_config->hardware_config.int_button_pin = ZH_NOT_USED;
            switch_config->hardware_config.ext_button_pin = ZH_NOT_USED;
            switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
            return;
        }
        if (switch_config->hardware_config.led_pin != ZH_NOT_USED)
        {
            config.intr_type = GPIO_INTR_DISABLE;
            config.mode = GPIO_MODE_OUTPUT;
            config.pin_bit_mask = (1ULL << switch_config->hardware_config.led_pin);
            config.pull_down_en = (switch_config->hardware_config.led_on_level == ZH_HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
            config.pull_up_en = (switch_config->hardware_config.led_on_level == ZH_HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
            if (gpio_config(&config) != ESP_OK)
            {
                switch_config->hardware_config.led_pin = ZH_NOT_USED;
            }
        }
        if (switch_config->hardware_config.int_button_pin != ZH_NOT_USED)
        {
            config.intr_type = (switch_config->hardware_config.int_button_on_level == ZH_HIGH) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE;
            config.mode = GPIO_MODE_INPUT;
            config.pin_bit_mask = (1ULL << switch_config->hardware_config.int_button_pin);
            config.pull_down_en = (switch_config->hardware_config.int_button_on_level == ZH_HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
            config.pull_up_en = (switch_config->hardware_config.int_button_on_level == ZH_HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
            if (gpio_config(&config) != ESP_OK)
            {
                switch_config->hardware_config.int_button_pin = ZH_NOT_USED;
            }
        }
        if (switch_config->hardware_config.ext_button_pin != ZH_NOT_USED)
        {
            config.intr_type = (switch_config->hardware_config.ext_button_on_level == ZH_HIGH) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE;
            config.mode = GPIO_MODE_INPUT;
            config.pin_bit_mask = (1ULL << switch_config->hardware_config.ext_button_pin);
            config.pull_down_en = (switch_config->hardware_config.ext_button_on_level == ZH_HIGH) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
            config.pull_up_en = (switch_config->hardware_config.ext_button_on_level == ZH_HIGH) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
            if (gpio_config(&config) != ESP_OK)
            {
                switch_config->hardware_config.ext_button_pin = ZH_NOT_USED;
            }
        }
        if (switch_config->hardware_config.int_button_pin != ZH_NOT_USED || switch_config->hardware_config.ext_button_pin != ZH_NOT_USED)
        {
            switch_config->button_pushing_semaphore = xSemaphoreCreateBinary();
            xTaskCreatePinnedToCore(&zh_gpio_processing_task, "NULL", ZH_GPIO_STACK_SIZE, switch_config, ZH_GPIO_TASK_PRIORITY, NULL, tskNO_AFFINITY);
            gpio_install_isr_service(0);
            if (switch_config->hardware_config.int_button_pin != ZH_NOT_USED)
            {
                gpio_isr_handler_add(switch_config->hardware_config.int_button_pin, zh_gpio_isr_handler, switch_config);
            }
            if (switch_config->hardware_config.ext_button_pin != ZH_NOT_USED)
            {
                gpio_isr_handler_add(switch_config->hardware_config.ext_button_pin, zh_gpio_isr_handler, switch_config);
            }
        }
        if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED)
        {
            switch (switch_config->hardware_config.sensor_type)
            {
            case HAST_DS18B20:
                if (zh_onewire_init(switch_config->hardware_config.sensor_pin) != ESP_OK)
                {
                    switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
                }
                break;
            case HAST_DHT11:;
            case HAST_DHT22:;
                zh_dht_sensor_type_t sensor_type = (switch_config->hardware_config.sensor_type == HAST_DHT11) ? ZH_DHT11 : ZH_DHT22;
                switch_config->dht_handle = zh_dht_init(sensor_type, switch_config->hardware_config.sensor_pin);
                switch_config->hardware_config.sensor_pin = switch_config->dht_handle.sensor_pin;
                break;
            default:
                switch_config->hardware_config.sensor_type = HAST_NONE;
                switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
                break;
            }
        }
    }
}

void zh_gpio_set_level(switch_config_t *switch_config)
{
    if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
    {
        switch_config->gpio_processing = true;
        if (switch_config->status.status == HAONOFT_ON)
        {
            gpio_set_level(switch_config->hardware_config.relay_pin, (switch_config->hardware_config.relay_on_level == ZH_HIGH) ? ZH_HIGH : ZH_LOW);
            if (switch_config->hardware_config.led_pin != ZH_NOT_USED)
            {
                gpio_set_level(switch_config->hardware_config.led_pin, (switch_config->hardware_config.led_on_level == ZH_HIGH) ? ZH_HIGH : ZH_LOW);
            }
        }
        else
        {
            gpio_set_level(switch_config->hardware_config.relay_pin, (switch_config->hardware_config.relay_on_level == ZH_HIGH) ? ZH_LOW : ZH_HIGH);
            if (switch_config->hardware_config.led_pin != ZH_NOT_USED)
            {
                gpio_set_level(switch_config->hardware_config.led_pin, (switch_config->hardware_config.led_on_level == ZH_HIGH) ? ZH_LOW : ZH_HIGH);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // To disable the interrupt for the period of power stabilization after the relay is switched on (when a large load is connected).
        switch_config->gpio_processing = false;
    }
}

void zh_gpio_isr_handler(void *arg)
{
    switch_config_t *switch_config = arg;
    if (switch_config->gpio_processing == false)
    {
        xSemaphoreGiveFromISR(switch_config->button_pushing_semaphore, NULL);
    }
}

void zh_gpio_processing_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    for (;;)
    {
        xSemaphoreTake(switch_config->button_pushing_semaphore, portMAX_DELAY);
        switch_config->gpio_processing = true;
        switch_config->status.status = (switch_config->status.status == HAONOFT_ON) ? HAONOFT_OFF : HAONOFT_ON;
        zh_gpio_set_level(switch_config);
        zh_save_status(switch_config);
        if (switch_config->gateway_is_available == true)
        {
            zh_send_switch_status_message(switch_config);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); // To prevent button contact rattling. Value is selected experimentally.
        switch_config->gpio_processing = false;
    }
    vTaskDelete(NULL);
}

void zh_send_switch_attributes_message_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    const esp_app_desc_t *app_info = get_app_description();
    zh_attributes_message_t attributes_message = {0};
    attributes_message.chip_type = ZH_CHIP_TYPE;
    strcpy(attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    attributes_message.cpu_frequency = ZH_CPU_FREQUENCY;
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
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SWITCH_ATTRIBUTES_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_send_switch_config_message(const switch_config_t *switch_config)
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
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_switch_hardware_config_message(const switch_config_t *switch_config)
{
    zh_config_message_t config_message = {0};
    config_message = (zh_config_message_t)switch_config->hardware_config;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_HARDWARE;
    data.payload_data = (zh_payload_data_t)config_message;
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_switch_keep_alive_message_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    zh_keep_alive_message_t keep_alive_message = {0};
    keep_alive_message.online_status = ZH_ONLINE;
    keep_alive_message.message_frequency = ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_KEEP_ALIVE;
    data.payload_data = (zh_payload_data_t)keep_alive_message;
    for (;;)
    {
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_send_switch_status_message(const switch_config_t *switch_config)
{
    zh_status_message_t status_message = {0};
    status_message = (zh_status_message_t)switch_config->status;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_STATE;
    data.payload_data = (zh_payload_data_t)status_message;
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_sensor_config_message(const switch_config_t *switch_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_CONFIG;
    zh_config_message_t config_message = {0};
    zh_sensor_config_message_t sensor_config_message = {0};
    sensor_config_message.suggested_display_precision = 1;
    sensor_config_message.expire_after = ZH_SENSOR_STATUS_MESSAGE_FREQUENCY * 3;
    sensor_config_message.enabled_by_default = true;
    sensor_config_message.force_update = true;
    sensor_config_message.qos = 2;
    sensor_config_message.retain = true;
    char *unit_of_measurement;
    switch (switch_config->hardware_config.sensor_type)
    {
    case HAST_DS18B20:;
        sensor_config_message.unique_id = 2;
        sensor_config_message.sensor_device_class = HASDC_TEMPERATURE;
        unit_of_measurement = "°C";
        strcpy(sensor_config_message.unit_of_measurement, unit_of_measurement);
        config_message = (zh_config_message_t)sensor_config_message;
        data.payload_data = (zh_payload_data_t)config_message;
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        break;
    case HAST_DHT11:;
    case HAST_DHT22:;
        sensor_config_message.unique_id = 2;
        sensor_config_message.sensor_device_class = HASDC_TEMPERATURE;
        unit_of_measurement = "°C";
        strcpy(sensor_config_message.unit_of_measurement, unit_of_measurement);
        config_message = (zh_config_message_t)sensor_config_message;
        data.payload_data = (zh_payload_data_t)config_message;
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        sensor_config_message.unique_id = 3;
        sensor_config_message.sensor_device_class = HASDC_HUMIDITY;
        unit_of_measurement = "%";
        strcpy(sensor_config_message.unit_of_measurement, unit_of_measurement);
        config_message = (zh_config_message_t)sensor_config_message;
        data.payload_data = (zh_payload_data_t)config_message;
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        break;
    default:
        break;
    }
}

void zh_send_sensor_attributes_message_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    const esp_app_desc_t *app_info = get_app_description();
    zh_attributes_message_t attributes_message = {0};
    attributes_message.chip_type = ZH_CHIP_TYPE;
    attributes_message.sensor_type = switch_config->hardware_config.sensor_type;
    strcpy(attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    attributes_message.cpu_frequency = ZH_CPU_FREQUENCY;
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
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SENSOR_ATTRIBUTES_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_send_sensor_status_message_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    float humidity = 0.0;
    float temperature = 0.0;
    zh_sensor_status_message_t sensor_status_message = {0};
    sensor_status_message.sensor_type = switch_config->hardware_config.sensor_type;
    zh_status_message_t status_message = {0};
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_STATE;
    for (;;)
    {
        switch (switch_config->hardware_config.sensor_type)
        {
        case HAST_DS18B20:
        ZH_DS18B20_READ:
            switch (zh_ds18b20_read(NULL, &temperature))
            {
            case ESP_OK:
                sensor_status_message.temperature = temperature;
                break;
            case ESP_FAIL:
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                goto ZH_DS18B20_READ;
                break;
            case ESP_ERR_INVALID_CRC:
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                goto ZH_DS18B20_READ;
                break;
            default:
                break;
            }
            break;
        case HAST_DHT11:
        case HAST_DHT22:
        ZH_DHT_READ:
            switch (zh_dht_read(&switch_config->dht_handle, &humidity, &temperature))
            {
            case ESP_OK:
                sensor_status_message.humidity = humidity;
                sensor_status_message.temperature = temperature;
                break;
            case ESP_ERR_INVALID_RESPONSE:
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                goto ZH_DHT_READ;
                break;
            case ESP_ERR_TIMEOUT:
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                goto ZH_DHT_READ;
                break;
            case ESP_ERR_INVALID_CRC:
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                goto ZH_DHT_READ;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        status_message = (zh_status_message_t)sensor_status_message;
        data.payload_data = (zh_payload_data_t)status_message;
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SENSOR_STATUS_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch_config_t *switch_config = arg;
    zh_espnow_data_t data = {0};
    switch (event_id)
    {
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    case ZH_ESPNOW_ON_RECV_EVENT:;
        zh_espnow_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_ESPNOW_EVENT_HANDLER_EXIT;
        }
#else
    case ZH_NETWORK_ON_RECV_EVENT:;
        zh_network_event_on_recv_t *recv_data = event_data;
        if (recv_data->data_len != sizeof(zh_espnow_data_t))
        {
            goto ZH_NETWORK_EVENT_HANDLER_EXIT;
        }
#endif
        memcpy(&data, recv_data->data, recv_data->data_len);
        switch (data.device_type)
        {
        case ZHDT_GATEWAY:
            switch (data.payload_type)
            {
            case ZHPT_KEEP_ALIVE:
                if (data.payload_data.keep_alive_message.online_status == ZH_ONLINE)
                {
                    if (switch_config->gateway_is_available == false)
                    {
                        switch_config->gateway_is_available = true;
                        memcpy(&switch_config->gateway_mac, &recv_data->mac_addr, 6);
                        zh_send_switch_hardware_config_message(switch_config);
                        if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                        {
                            zh_send_switch_config_message(switch_config);
                            zh_send_switch_status_message(switch_config);
                            xTaskCreatePinnedToCore(&zh_send_switch_attributes_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->switch_attributes_message_task, tskNO_AFFINITY);
                            xTaskCreatePinnedToCore(&zh_send_switch_keep_alive_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->switch_keep_alive_message_task, tskNO_AFFINITY);
                            if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                            {
                                zh_send_sensor_config_message(switch_config);
                                xTaskCreatePinnedToCore(&zh_send_sensor_status_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->sensor_status_message_task, tskNO_AFFINITY);
                                xTaskCreatePinnedToCore(&zh_send_sensor_attributes_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->sensor_attributes_message_task, tskNO_AFFINITY);
                            }
                        }
                    }
                }
                else
                {
                    if (switch_config->gateway_is_available == true)
                    {
                        switch_config->gateway_is_available = false;
                        if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                        {
                            vTaskDelete(switch_config->switch_attributes_message_task);
                            vTaskDelete(switch_config->switch_keep_alive_message_task);
                            if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                            {
                                vTaskDelete(switch_config->sensor_attributes_message_task);
                                vTaskDelete(switch_config->sensor_status_message_task);
                            }
                        }
                    }
                }
                break;
            case ZHPT_SET:
                switch_config->status = data.payload_data.status_message.switch_status_message;
                zh_gpio_set_level(switch_config);
                zh_save_status(switch_config);
                zh_send_switch_status_message(switch_config);
                break;
            case ZHPT_HARDWARE:
                switch_config->hardware_config = data.payload_data.config_message.switch_hardware_config_message;
                zh_save_config(switch_config);
                esp_restart();
                break;
            case ZHPT_UPDATE:;
                const esp_app_desc_t *app_info = get_app_description();
                switch_config->update_partition = esp_ota_get_next_update_partition(NULL);
                zh_espnow_ota_message_t espnow_ota_message = {0};
                espnow_ota_message.chip_type = ZH_CHIP_TYPE;
                strcpy(espnow_ota_message.app_version, app_info->version);
#ifdef CONFIG_IDF_TARGET_ESP8266
                char *app_name = (char *)heap_caps_malloc(strlen(app_info->project_name) + 6, MALLOC_CAP_8BIT);
                memset(app_name, 0, strlen(app_info->project_name) + 6);
                sprintf(app_name, "%s.app%d", app_info->project_name, switch_config->update_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0 + 1);
                strcpy(espnow_ota_message.app_name, app_name);
                heap_caps_free(app_name);
#else
                strcpy(espnow_ota_message.app_name, app_info->project_name);
#endif
                data.device_type = ZHDT_SWITCH;
                data.payload_type = ZHPT_UPDATE;
                data.payload_data = (zh_payload_data_t)espnow_ota_message;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_BEGIN:
#ifdef CONFIG_IDF_TARGET_ESP8266
                esp_ota_begin(switch_config->update_partition, OTA_SIZE_UNKNOWN, &switch_config->update_handle);
#else
                esp_ota_begin(switch_config->update_partition, OTA_SIZE_UNKNOWN, (esp_ota_handle_t *)&switch_config->update_handle);
#endif
                switch_config->ota_message_part_number = 1;
                data.device_type = ZHDT_SWITCH;
                data.payload_type = ZHPT_UPDATE_PROGRESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_PROGRESS:
                if (switch_config->ota_message_part_number == data.payload_data.espnow_ota_message.part)
                {
                    ++switch_config->ota_message_part_number;
                    esp_ota_write(switch_config->update_handle, (const void *)data.payload_data.espnow_ota_message.data, data.payload_data.espnow_ota_message.data_len);
                }
                data.device_type = ZHDT_SWITCH;
                data.payload_type = ZHPT_UPDATE_PROGRESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_ERROR:
                esp_ota_end(switch_config->update_handle);
                break;
            case ZHPT_UPDATE_END:
                if (esp_ota_end(switch_config->update_handle) != ESP_OK)
                {
                    data.device_type = ZHDT_SWITCH;
                    data.payload_type = ZHPT_UPDATE_FAIL;
                    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
                    break;
                }
                esp_ota_set_boot_partition(switch_config->update_partition);
                data.device_type = ZHDT_SWITCH;
                data.payload_type = ZHPT_UPDATE_SUCCESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
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
#ifdef CONFIG_NETWORK_TYPE_DIRECT
    ZH_ESPNOW_EVENT_HANDLER_EXIT:
        heap_caps_free(recv_data->data);
        break;
    case ZH_ESPNOW_ON_SEND_EVENT:;
        zh_espnow_event_on_send_t *send_data = event_data;
        if (send_data->status == ZH_ESPNOW_SEND_FAIL && switch_config->gateway_is_available == true)
        {
            switch_config->gateway_is_available = false;
            if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
            {
                vTaskDelete(switch_config->switch_attributes_message_task);
                vTaskDelete(switch_config->switch_keep_alive_message_task);
                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                {
                    vTaskDelete(switch_config->sensor_attributes_message_task);
                    vTaskDelete(switch_config->sensor_status_message_task);
                }
            }
        }
        break;
#else
    ZH_NETWORK_EVENT_HANDLER_EXIT:
        heap_caps_free(recv_data->data);
        break;
    case ZH_NETWORK_ON_SEND_EVENT:;
        zh_network_event_on_send_t *send_data = event_data;
        if (send_data->status == ZH_NETWORK_SEND_FAIL && switch_config->gateway_is_available == true)
        {
            switch_config->gateway_is_available = false;
            if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
            {
                vTaskDelete(switch_config->switch_attributes_message_task);
                vTaskDelete(switch_config->switch_keep_alive_message_task);
                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                {
                    vTaskDelete(switch_config->sensor_attributes_message_task);
                    vTaskDelete(switch_config->sensor_status_message_task);
                }
            }
        }
        break;
#endif
    default:
        break;
    }
}