#include <M5EPD.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "nscanf.h"
#include "config.h"

const String AMBIENT_HOST = "http://ambidata.io/api/v2/";

M5EPD_Canvas canvas(&M5.EPD);

DynamicJsonDocument fetch(String url)
{
  DynamicJsonDocument doc(20000);

  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      //jsonオブジェクトの作成
      deserializeJson(doc, http.getString());
    }
    else
    {
      Serial.println("Error on HTTP request");
    }
    http.end(); //リソースを解放
  }
  return doc;
}

void setup()
{
  M5.begin();
  M5.EPD.SetRotation(90);
  M5.EPD.Clear(true);
  M5.RTC.begin();
  WiFi.begin(WIFI_SSID,WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
  }

  DynamicJsonDocument doc = fetch(AMBIENT_HOST + String("channels/") + AMBIENT_CHANNEL_ID + String("/data?readKey=") + AMBIENT_READ_KEY + String("&start=2020-11-27+00:00:00&end=2020-11-29+00:00:00"));

  canvas.createCanvas(540, 960);
  canvas.setTextSize(3);
  canvas.drawString(doc[0]["created"].as<String>(), 45, 350);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loop()
{
}