git clone -b esp8266 --recursive http://git.zh.com.ru/alexey.zholtikov/zh_espnow_switch.git
cd zh_espnow_switch
mkdir components
cd components
git clone http://git.zh.com.ru/alexey.zholtikov/zh_config.git
git clone -b esp8266 --recursive http://git.zh.com.ru/alexey.zholtikov/zh_espnow.git
git clone -b esp8266 --recursive http://git.zh.com.ru/alexey.zholtikov/zh_onewire.git
git clone http://git.zh.com.ru/alexey.zholtikov/zh_ds18b20.git
cd ..