# ESP-NOW switch

ESP-NOW based switch for ESP32 ESP-IDF. Alternate firmware for Tuya/SmartLife/eWeLink WiFi switches.

There are two branches - for ESP8266 family and for ESP32 family. Please use the appropriate one.

## Features

1. Saves the last state when the power is turned off.
2. Automatically adds switch configuration to Home Assistan via MQTT discovery as a switch.
3. Update firmware from HTTPS server via ESP-NOW.
4. Optional support one external DS18B20 sensor.

## Notes

1. For initial settings use "menuconfig -> ZH ESP-NOW Switch Configuration". After first boot all settings will be stored in NVS memory for prevente change during OTA firmware update.
2. To restart the switch, send the "restart" command to the root topic of the switch (example - "homeassistant/espnow_switch/24-62-AB-F9-1F-A8").
3. To update the switch firmware, send the "update" command to the root topic of the switch (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7"). The update path should be like as "https://your_server/zh_espnow_switch_esp32.bin". The time and success of the update depends on the load on WiFi channel 1. Average update time is less than one minute. The online status of the update is displayed in the root switch topic.

## Build and flash

Run the following command to firmware build and flash module:

```text
cd your_projects_folder
bash <(curl -Ls http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch/raw/branch/esp32/install.sh)
cd zh_espnow_switch
idf.py menuconfig
idf.py all
idf.py -p (PORT) flash
```

## Attention

1. A gateway is required. For details see [zh_gateway](http://git.zh.com.ru/alexey.zholtikov/zh_gateway).