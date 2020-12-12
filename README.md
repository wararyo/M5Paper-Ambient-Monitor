# M5Paper Ambient Display

一日ごとの電力消費量をグラフで表示します。  
norifumiさんによる[M5StickC Wi-SUN HAT](https://github.com/rin-ofumi/m5stickc_wisun_hat) との併用を前提にしています。

## セットアップ

main.cppと同じディレクトリに `config.h` を追加します。

``` cpp
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Arduino.h>

const String AMBIENT_CHANNEL_ID = "[積算電力消費量のチャンネルID]";
const String AMBIENT_READ_KEY = "[積算電力消費量のRead Key]";
const char* WIFI_SSID = "[WiFi SSID]";
const char* WIFI_PASS = "[WiFi Pass]";

#endif
```
