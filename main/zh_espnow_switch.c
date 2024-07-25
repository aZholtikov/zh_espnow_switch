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
#ifdef CONFIG_IDF_TARGET_ESP8266
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
#else
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
#endif
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
    SETUP_INITIAL_SETTINGS:
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
        switch_config->hardware_config.measurement_frequency = CONFIG_MEASUREMENT_FREQUENCY;
#elif CONFIG_SENSOR_TYPE_DHT
        switch_config->hardware_config.sensor_pin = CONFIG_SENSOR_PIN;
        switch_config->hardware_config.sensor_type = HAST_DHT;
        switch_config->hardware_config.measurement_frequency = CONFIG_MEASUREMENT_FREQUENCY;
#else
        switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
        switch_config->hardware_config.sensor_type = HAST_NONE;
        switch_config->hardware_config.measurement_frequency = ZH_NOT_USED;
#endif
        zh_save_config(switch_config);
        return;
    }
    esp_err_t err = ESP_OK;
    err += nvs_get_u8(nvs_handle, "relay_pin", &switch_config->hardware_config.relay_pin);
    err += nvs_get_u8(nvs_handle, "relay_lvl", (uint8_t *)&switch_config->hardware_config.relay_on_level);
    err += nvs_get_u8(nvs_handle, "led_pin", &switch_config->hardware_config.led_pin);
    err += nvs_get_u8(nvs_handle, "led_lvl", (uint8_t *)&switch_config->hardware_config.led_on_level);
    err += nvs_get_u8(nvs_handle, "int_btn_pin", &switch_config->hardware_config.int_button_pin);
    err += nvs_get_u8(nvs_handle, "int_btn_lvl", (uint8_t *)&switch_config->hardware_config.int_button_on_level);
    err += nvs_get_u8(nvs_handle, "ext_btn_pin", &switch_config->hardware_config.ext_button_pin);
    err += nvs_get_u8(nvs_handle, "ext_btn_lvl", (uint8_t *)&switch_config->hardware_config.ext_button_on_level);
    err += nvs_get_u8(nvs_handle, "sensor_pin", &switch_config->hardware_config.sensor_pin);
    err += nvs_get_u8(nvs_handle, "sensor_type", (uint8_t *)&switch_config->hardware_config.sensor_type);
    err += nvs_get_u16(nvs_handle, "frequency", &switch_config->hardware_config.measurement_frequency);
    nvs_close(nvs_handle);
    if (err != ESP_OK)
    {
        goto SETUP_INITIAL_SETTINGS;
    }
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
    nvs_set_u16(nvs_handle, "frequency", switch_config->hardware_config.measurement_frequency);
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
            case HAST_DHT:;
                zh_dht_init_config_t dht_init_config = ZH_DHT_INIT_CONFIG_DEFAULT();
                dht_init_config.sensor_pin = switch_config->hardware_config.sensor_pin;
                if (zh_dht_init(&dht_init_config) != ESP_OK)
                {
                    switch_config->hardware_config.sensor_pin = ZH_NOT_USED;
                }
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
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_ATTRIBUTES;
    data.payload_data.attributes_message.chip_type = ZH_CHIP_TYPE;
    strcpy(data.payload_data.attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    data.payload_data.attributes_message.cpu_frequency = ZH_CPU_FREQUENCY;
    data.payload_data.attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(data.payload_data.attributes_message.app_name, app_info->project_name);
    strcpy(data.payload_data.attributes_message.app_version, app_info->version);
    for (;;)
    {
        data.payload_data.attributes_message.heap_size = esp_get_free_heap_size();
        data.payload_data.attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        data.payload_data.attributes_message.uptime = esp_timer_get_time() / 1000000;
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SWITCH_ATTRIBUTES_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_send_switch_config_message(const switch_config_t *switch_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data.config_message.switch_config_message.unique_id = 1;
    data.payload_data.config_message.switch_config_message.device_class = HASWDC_SWITCH;
    data.payload_data.config_message.switch_config_message.payload_on = HAONOFT_ON;
    data.payload_data.config_message.switch_config_message.payload_off = HAONOFT_OFF;
    data.payload_data.config_message.switch_config_message.enabled_by_default = true;
    data.payload_data.config_message.switch_config_message.optimistic = false;
    data.payload_data.config_message.switch_config_message.qos = 2;
    data.payload_data.config_message.switch_config_message.retain = true;
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_switch_hardware_config_message(const switch_config_t *switch_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_HARDWARE;
    data.payload_data.config_message.switch_hardware_config_message.relay_pin = switch_config->hardware_config.relay_pin;
    data.payload_data.config_message.switch_hardware_config_message.relay_on_level = switch_config->hardware_config.relay_on_level;
    data.payload_data.config_message.switch_hardware_config_message.led_pin = switch_config->hardware_config.led_pin;
    data.payload_data.config_message.switch_hardware_config_message.led_on_level = switch_config->hardware_config.led_on_level;
    data.payload_data.config_message.switch_hardware_config_message.int_button_pin = switch_config->hardware_config.int_button_pin;
    data.payload_data.config_message.switch_hardware_config_message.int_button_on_level = switch_config->hardware_config.int_button_on_level;
    data.payload_data.config_message.switch_hardware_config_message.ext_button_pin = switch_config->hardware_config.ext_button_pin;
    data.payload_data.config_message.switch_hardware_config_message.ext_button_on_level = switch_config->hardware_config.ext_button_on_level;
    data.payload_data.config_message.switch_hardware_config_message.sensor_pin = switch_config->hardware_config.sensor_pin;
    data.payload_data.config_message.switch_hardware_config_message.sensor_type = switch_config->hardware_config.sensor_type;
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_switch_keep_alive_message_task(void *pvParameter)
{
    switch_config_t *switch_config = pvParameter;
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_KEEP_ALIVE;
    data.payload_data.keep_alive_message.online_status = ZH_ONLINE;
    data.payload_data.keep_alive_message.message_frequency = ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY;
    for (;;)
    {
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        vTaskDelay(ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_send_switch_status_message(const switch_config_t *switch_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SWITCH;
    data.payload_type = ZHPT_STATE;
    data.payload_data.status_message.switch_status_message.status = switch_config->status.status;
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
}

void zh_send_sensor_config_message(const switch_config_t *switch_config)
{
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_CONFIG;
    data.payload_data.config_message.sensor_config_message.suggested_display_precision = 1;
    data.payload_data.config_message.sensor_config_message.expire_after = switch_config->hardware_config.measurement_frequency * 1.5; // + 50% just in case.
    data.payload_data.config_message.sensor_config_message.enabled_by_default = true;
    data.payload_data.config_message.sensor_config_message.force_update = true;
    data.payload_data.config_message.sensor_config_message.qos = 2;
    data.payload_data.config_message.sensor_config_message.retain = true;
    char *unit_of_measurement = NULL;
    // For compatibility with zh_espnow_sensor.
    data.payload_data.config_message.sensor_config_message.unique_id = 2;
    data.payload_data.config_message.sensor_config_message.sensor_device_class = HASDC_VOLTAGE;
    unit_of_measurement = "V";
    strcpy(data.payload_data.config_message.sensor_config_message.unit_of_measurement, unit_of_measurement);
    zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
    // For compatibility with zh_espnow_sensor.
    switch (switch_config->hardware_config.sensor_type)
    {
    case HAST_DS18B20:
        data.payload_data.config_message.sensor_config_message.unique_id = 3;
        data.payload_data.config_message.sensor_config_message.sensor_device_class = HASDC_TEMPERATURE;
        unit_of_measurement = "°C";
        strcpy(data.payload_data.config_message.sensor_config_message.unit_of_measurement, unit_of_measurement);
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        break;
    case HAST_DHT:
        data.payload_data.config_message.sensor_config_message.unique_id = 3;
        data.payload_data.config_message.sensor_config_message.sensor_device_class = HASDC_TEMPERATURE;
        unit_of_measurement = "°C";
        strcpy(data.payload_data.config_message.sensor_config_message.unit_of_measurement, unit_of_measurement);
        zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        data.payload_data.config_message.sensor_config_message.unique_id = 4;
        data.payload_data.config_message.sensor_config_message.sensor_device_class = HASDC_HUMIDITY;
        unit_of_measurement = "%";
        strcpy(data.payload_data.config_message.sensor_config_message.unit_of_measurement, unit_of_measurement);
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
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    data.payload_type = ZHPT_ATTRIBUTES;
    data.payload_data.attributes_message.chip_type = ZH_CHIP_TYPE;
    data.payload_data.attributes_message.sensor_type = switch_config->hardware_config.sensor_type;
    strcpy(data.payload_data.attributes_message.flash_size, CONFIG_ESPTOOLPY_FLASHSIZE);
    data.payload_data.attributes_message.cpu_frequency = ZH_CPU_FREQUENCY;
    data.payload_data.attributes_message.reset_reason = (uint8_t)esp_reset_reason();
    strcpy(data.payload_data.attributes_message.app_name, app_info->project_name);
    strcpy(data.payload_data.attributes_message.app_version, app_info->version);
    for (;;)
    {
        data.payload_data.attributes_message.heap_size = esp_get_free_heap_size();
        data.payload_data.attributes_message.min_heap_size = esp_get_minimum_free_heap_size();
        data.payload_data.attributes_message.uptime = esp_timer_get_time() / 1000000;
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
    zh_espnow_data_t data = {0};
    data.device_type = ZHDT_SENSOR;
    for (;;)
    {
        uint8_t attempts = 0;
    READ_SENSOR:;
        esp_err_t err = ESP_OK;
        switch (switch_config->hardware_config.sensor_type)
        {
        case HAST_DS18B20:
            err = zh_ds18b20_read(NULL, &temperature);
            if (err == ESP_OK)
            {
                data.payload_data.status_message.sensor_status_message.temperature = temperature;
                data.payload_data.status_message.sensor_status_message.voltage = 3.3; // For future development.
            }
            break;
        case HAST_DHT:
            err = zh_dht_read(&humidity, &temperature);
            if (err == ESP_OK)
            {
                data.payload_data.status_message.sensor_status_message.humidity = humidity;
                data.payload_data.status_message.sensor_status_message.temperature = temperature;
                data.payload_data.status_message.sensor_status_message.voltage = 3.3; // For future development.
            }
            break;
        default:
            break;
        }
        if (err == ESP_OK)
        {
            data.payload_type = ZHPT_STATE;
            data.payload_data.status_message.sensor_status_message.sensor_type = switch_config->hardware_config.sensor_type;
            zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
        }
        else
        {
            if (++attempts < ZH_SENSOR_READ_MAXIMUM_RETRY)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                goto READ_SENSOR;
            }
            data.payload_type = ZHPT_ERROR;
            char *message = (char *)heap_caps_malloc(150, MALLOC_CAP_8BIT);
            memset(message, 0, 150);
            sprintf(message, "Sensor %s reading error. Error - %s.", zh_get_sensor_type_value_name(switch_config->hardware_config.sensor_type), esp_err_to_name(err));
            strcpy(data.payload_data.status_message.error_message.message, message);
            zh_send_message(switch_config->gateway_mac, (uint8_t *)&data, sizeof(zh_espnow_data_t));
            heap_caps_free(message);
        }
        vTaskDelay(switch_config->hardware_config.measurement_frequency * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void zh_espnow_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch_config_t *switch_config = arg;
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
        zh_espnow_data_t *data = (zh_espnow_data_t *)recv_data->data;
        switch (data->device_type)
        {
        case ZHDT_GATEWAY:
            switch (data->payload_type)
            {
            case ZHPT_KEEP_ALIVE:
                if (data->payload_data.keep_alive_message.online_status == ZH_ONLINE)
                {
                    if (switch_config->gateway_is_available == false)
                    {
                        switch_config->gateway_is_available = true;
                        memcpy(switch_config->gateway_mac, recv_data->mac_addr, 6);
                        zh_send_switch_hardware_config_message(switch_config);
                        if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                        {
                            zh_send_switch_config_message(switch_config);
                            zh_send_switch_status_message(switch_config);
                            if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                            {
                                zh_send_sensor_config_message(switch_config);
                            }
                            if (switch_config->is_first_connection == false)
                            {
                                xTaskCreatePinnedToCore(&zh_send_switch_attributes_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->switch_attributes_message_task, tskNO_AFFINITY);
                                xTaskCreatePinnedToCore(&zh_send_switch_keep_alive_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->switch_keep_alive_message_task, tskNO_AFFINITY);
                                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                                {
                                    xTaskCreatePinnedToCore(&zh_send_sensor_status_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->sensor_status_message_task, tskNO_AFFINITY);
                                    xTaskCreatePinnedToCore(&zh_send_sensor_attributes_message_task, "NULL", ZH_MESSAGE_STACK_SIZE, switch_config, ZH_MESSAGE_TASK_PRIORITY, (TaskHandle_t *)&switch_config->sensor_attributes_message_task, tskNO_AFFINITY);
                                }
                                switch_config->is_first_connection = true;
                            }
                            else
                            {
                                vTaskResume(switch_config->switch_attributes_message_task);
                                vTaskResume(switch_config->switch_keep_alive_message_task);
                                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                                {
                                    vTaskResume(switch_config->sensor_status_message_task);
                                    vTaskResume(switch_config->sensor_attributes_message_task);
                                }
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
                            vTaskSuspend(switch_config->switch_attributes_message_task);
                            vTaskSuspend(switch_config->switch_keep_alive_message_task);
                            if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                            {
                                vTaskSuspend(switch_config->sensor_attributes_message_task);
                                vTaskSuspend(switch_config->sensor_status_message_task);
                            }
                        }
                    }
                }
                break;
            case ZHPT_SET:
                switch_config->status.status = data->payload_data.status_message.switch_status_message.status;
                zh_gpio_set_level(switch_config);
                zh_save_status(switch_config);
                zh_send_switch_status_message(switch_config);
                break;
            case ZHPT_HARDWARE:
                switch_config->hardware_config.relay_pin = data->payload_data.config_message.switch_hardware_config_message.relay_pin;
                switch_config->hardware_config.relay_on_level = data->payload_data.config_message.switch_hardware_config_message.relay_on_level;
                switch_config->hardware_config.led_pin = data->payload_data.config_message.switch_hardware_config_message.led_pin;
                switch_config->hardware_config.led_on_level = data->payload_data.config_message.switch_hardware_config_message.led_on_level;
                switch_config->hardware_config.int_button_pin = data->payload_data.config_message.switch_hardware_config_message.int_button_pin;
                switch_config->hardware_config.int_button_on_level = data->payload_data.config_message.switch_hardware_config_message.int_button_on_level;
                switch_config->hardware_config.ext_button_pin = data->payload_data.config_message.switch_hardware_config_message.ext_button_pin;
                switch_config->hardware_config.ext_button_on_level = data->payload_data.config_message.switch_hardware_config_message.ext_button_on_level;
                switch_config->hardware_config.sensor_pin = data->payload_data.config_message.switch_hardware_config_message.sensor_pin;
                switch_config->hardware_config.sensor_type = data->payload_data.config_message.switch_hardware_config_message.sensor_type;
                switch_config->hardware_config.measurement_frequency = data->payload_data.config_message.switch_hardware_config_message.measurement_frequency;
                zh_save_config(switch_config);
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
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_KEEP_ALIVE;
                data->payload_data.keep_alive_message.online_status = ZH_OFFLINE;
                data->payload_data.keep_alive_message.message_frequency = ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
                break;
            case ZHPT_UPDATE:;
                if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                {
                    vTaskSuspend(switch_config->switch_attributes_message_task);
                    vTaskSuspend(switch_config->switch_keep_alive_message_task);
                    if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                    {
                        vTaskSuspend(switch_config->sensor_attributes_message_task);
                        vTaskSuspend(switch_config->sensor_status_message_task);
                    }
                }
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_KEEP_ALIVE;
                data->payload_data.keep_alive_message.online_status = ZH_OFFLINE;
                data->payload_data.keep_alive_message.message_frequency = ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                const esp_app_desc_t *app_info = get_app_description();
                switch_config->update_partition = esp_ota_get_next_update_partition(NULL);
                strcpy(data->payload_data.ota_message.espnow_ota_data.app_version, app_info->version);
#ifdef CONFIG_IDF_TARGET_ESP8266
                char *app_name = (char *)heap_caps_malloc(strlen(app_info->project_name) + 6, MALLOC_CAP_8BIT);
                memset(app_name, 0, strlen(app_info->project_name) + 6);
                sprintf(app_name, "%s.app%d", app_info->project_name, switch_config->update_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0 + 1);
                strcpy(data->payload_data.ota_message.espnow_ota_data.app_name, app_name);
                heap_caps_free(app_name);
#else
                strcpy(data->payload_data.ota_message.espnow_ota_data.app_name, app_info->project_name);
#endif
                data->payload_type = ZHPT_UPDATE;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_BEGIN:
#ifdef CONFIG_IDF_TARGET_ESP8266
                esp_ota_begin(switch_config->update_partition, OTA_SIZE_UNKNOWN, &switch_config->update_handle);
#else
                esp_ota_begin(switch_config->update_partition, OTA_SIZE_UNKNOWN, (esp_ota_handle_t *)&switch_config->update_handle);
#endif
                switch_config->ota_message_part_number = 1;
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_UPDATE_PROGRESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_PROGRESS:
                if (switch_config->ota_message_part_number == data->payload_data.ota_message.espnow_ota_message.part)
                {
                    ++switch_config->ota_message_part_number;
                    esp_ota_write(switch_config->update_handle, (const void *)data->payload_data.ota_message.espnow_ota_message.data, data->payload_data.ota_message.espnow_ota_message.data_len);
                }
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_UPDATE_PROGRESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                break;
            case ZHPT_UPDATE_ERROR:
                esp_ota_end(switch_config->update_handle);
                if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                {
                    vTaskResume(switch_config->switch_attributes_message_task);
                    vTaskResume(switch_config->switch_keep_alive_message_task);
                    if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                    {
                        vTaskResume(switch_config->sensor_attributes_message_task);
                        vTaskResume(switch_config->sensor_status_message_task);
                    }
                }
                break;
            case ZHPT_UPDATE_END:
                if (esp_ota_end(switch_config->update_handle) != ESP_OK)
                {
                    data->device_type = ZHDT_SWITCH;
                    data->payload_type = ZHPT_UPDATE_FAIL;
                    zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                    if (switch_config->hardware_config.relay_pin != ZH_NOT_USED)
                    {
                        vTaskResume(switch_config->switch_attributes_message_task);
                        vTaskResume(switch_config->switch_keep_alive_message_task);
                        if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                        {
                            vTaskResume(switch_config->sensor_attributes_message_task);
                            vTaskResume(switch_config->sensor_status_message_task);
                        }
                    }
                    break;
                }
                esp_ota_set_boot_partition(switch_config->update_partition);
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_UPDATE_SUCCESS;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
                break;
            case ZHPT_RESTART:
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
                data->device_type = ZHDT_SWITCH;
                data->payload_type = ZHPT_KEEP_ALIVE;
                data->payload_data.keep_alive_message.online_status = ZH_OFFLINE;
                data->payload_data.keep_alive_message.message_frequency = ZH_SWITCH_KEEP_ALIVE_MESSAGE_FREQUENCY;
                zh_send_message(switch_config->gateway_mac, (uint8_t *)data, sizeof(zh_espnow_data_t));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
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
                vTaskSuspend(switch_config->switch_attributes_message_task);
                vTaskSuspend(switch_config->switch_keep_alive_message_task);
                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                {
                    vTaskSuspend(switch_config->sensor_attributes_message_task);
                    vTaskSuspend(switch_config->sensor_status_message_task);
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
                vTaskSuspend(switch_config->switch_attributes_message_task);
                vTaskSuspend(switch_config->switch_keep_alive_message_task);
                if (switch_config->hardware_config.sensor_pin != ZH_NOT_USED && switch_config->hardware_config.sensor_type != HAST_NONE)
                {
                    vTaskSuspend(switch_config->sensor_attributes_message_task);
                    vTaskSuspend(switch_config->sensor_status_message_task);
                }
            }
        }
        break;
#endif
    default:
        break;
    }
}