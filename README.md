# ESP-NOW switch

ESP-NOW based switch for ESP32 ESP-IDF and ESP8266 RTOS SDK. Alternate firmware for Tuya/SmartLife/eWeLink WiFi switches.

## Tested on

1. ESP8266 RTOS_SDK v3.4
2. ESP32 ESP-IDF v5.2

## Features

1. Saves the last state when the power is turned off.
2. Automatically adds switch configuration to Home Assistan via MQTT discovery as a switch.
3. Update firmware from HTTPS server via ESP-NOW.
4. Optional support one external 1-wire sensor (DS18B20/DHT11/DHT22/AM2302/AM2320).
5. Direct or mesh work mode.

## Notes

1. Work mode must be same with [gateway](https://github.com/aZholtikov/zh_gateway) work mode.
2. ESP-NOW mesh network based on the [zh_network](https://github.com/aZholtikov/zh_network).
3. For initial settings use "menuconfig -> ZH ESP-NOW Switch Configuration". After first boot all settings will be stored in NVS memory for prevente change during OTA firmware update.
4. To restart the switch, send the "restart" command to the root topic of the switch (example - "homeassistant/espnow_switch/24-62-AB-F9-1F-A8").
5. To update the switch firmware, send the "update" command to the root topic of the switch (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7"). The update path should be like as "https://your_server/zh_espnow_switch_esp32.bin" (for ESP32) or "https://your_server/zh_espnow_switch_esp8266.app1.bin + https://your_server/zh_espnow_switch_esp8266.app2.bin" (for ESP8266). Average update time is up to some minutes. The online status of the update will be displayed in the root switch topic.
6. To change initial settings of the switch (except work mode), send the X1,X2,X3,X4,X5,X6,X7,X8,X9,X10,X11 command to the hardware topic of the switch (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7/hardware"). The configuration will only be accepted if it does not cause errors. The current configuration status is displayed in the configuration topic of the switch (example - "homeassistant/espnow_switch/70-03-9F-44-BE-F7/config").

MQTT configuration message should filled according to the template "X1,X2,X3,X4,X5,X6,X7,X8,X9,X10". Where:

1. X1 - Relay GPIO number. 0 - 48 (according to the module used), 255 if not used.
2. X2 - Relay ON level. 1 for high, 0 for low. Only affected when X1 is used.
3. X3 - Led GPIO number. 0 - 48 (according to the module used), 255 if not used.
4. X4 - Led ON level. 1 for high, 0 for low. Only affected when X3 is used.
5. X5 - Internal button GPIO number. 0 - 48 (according to the module used), 255 if not used.
6. X6 - Internal button trigger level. 1 for high, 0 for low. Only affected when X5 is used.
7. X7 - External button GPIO number. 0 - 48 (according to the module used), 255 if not used.
8. X8 - External button trigger level. 1 for high, 0 for low. Only affected when X7 is used.
9. X9 - Sensor GPIO number. 0 - 48 (according to the module used). 255 if not used.
10. X10 - Sensor type. 1 for DS18B20, 8 for DHT. Only affected when X9 is used.
11. X11 - Sensor measurement frequency. 1 - 65536 seconds. Only affected when X9 is used.

## Build and flash

Run the following command to firmware build and flash module:

```text
cd your_projects_folder
git clone --recurse-submodules https://github.com/aZholtikov/zh_espnow_switch.git
cd zh_espnow_switch
```

For ESP32 family:

```text
idf.py set-target (TARGET) // Optional
idf.py menuconfig
idf.py build
idf.py flash
```

For ESP8266 family:

```text
make menuconfig
make
make flash
```

## Tested on hardware

See [here](https://github.com/aZholtikov/zh_espnow_switch/tree/main/hardware).

## Attention

1. A gateway is required. For details see [zh_gateway](https://github.com/aZholtikov/zh_gateway).
2. Use the "make ota" command instead of "make" to prepare 2 bin files for OTA update (for ESP8266).

Any [feedback](mailto:github@azholtikov.ru) will be gladly accepted.
