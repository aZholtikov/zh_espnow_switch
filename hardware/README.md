# Tested on hardware

1. MOES 1CH 10A. Built on Tuya WiFi module WA2 (WB2S) (BK7231T chip). Replacement with ESP-02S (TYWE2S). [Photo](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch/src/branch/main/hardware/MOES_1CH_10A).

```text
    Using menuconfig:

    Relay GPIO number               12
    Relay ON level                  HIGH
    Led GPIO number                 4
    Led ON level                    LOW
    Internal button GPIO number     13
    Internal button trigger level   LOW

    Using MQTT:

    "12,1,4,0,13,0,255,0"
```

2. MINI 1CH 16A. Built on Tuya WiFi module WB2S (BK7231T chip). Replacement with ESP-02S (TYWE2S). [Photo](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch/src/branch/main/hardware/MINI_1CH_16A).

```text
    Using menuconfig:

    Relay GPIO number               13
    Relay ON level                  HIGH
    Led GPIO number                 4
    Led ON level                    LOW
    Internal button GPIO number     3
    Internal button trigger level   LOW
    External button GPIO number     14
    External button trigger level   HIGH

    Using MQTT:

    "13,1,4,0,3,0,14,1"
```

3. LIGHT E27 SOCKET. Built on Tuya WiFi module WA2 (WB2S) (BK7231T chip). Replacement with ESP-02S (TYWE2S). [Photo](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch/src/branch/main/hardware/LIGHT_E27_SOCKET).

```text
    Using menuconfig:

    Relay GPIO number               12
    Relay ON level                  HIGH
    Led GPIO number                 4
    Led ON level                    LOW
    Internal button GPIO number     13
    Internal button trigger level   LOW

    Using MQTT:

    "12,1,4,0,13,0,255,0"
```

4. TH 1CH 16A + DS18B20. Built on ITEAD WiFi module PSF-B85 (ESP8285 chip). Replacement not required. [Photo](http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch/src/branch/main/hardware/TH_1CH_16A).

```text
    Using menuconfig:

    Relay GPIO number               12
    Relay ON level                  HIGH
    Led GPIO number                 13
    Led ON level                    LOW
    Internal button GPIO number     0
    Internal button trigger level   LOW

    Using MQTT:

    "12,1,13,0,0,0,255,0"
```