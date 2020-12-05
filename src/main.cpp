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

bool parseDateTime(String str, RTC_Date *date, RTC_Time *time) {
  // 年を正しく解釈するため、全ての%dをlongとして解釈するようにnscanfを改変している
  long year,mon,day,hour,min,sec,millis;
  int result = nscanf(str.c_str(), "%d-%d-%dT%d:%d:%d.%d", &year, &mon, &day, &hour, &min, &sec, &millis);
  if(result > 0) {
    date->year = year;
    date->mon = mon;
    date->day = day;
    time->hour = hour;
    time->min = min;
    time->sec = sec;
    return result;
  }
  else return result;
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

  String url = AMBIENT_HOST + String("channels/") + AMBIENT_CHANNEL_ID + String("/data?readKey=") + AMBIENT_READ_KEY + String("&start=2020-11-27+00:00:00&end=2020-11-29+00:00:00");
  DynamicJsonDocument doc = fetch(url);

  canvas.createCanvas(540, 960);
  canvas.setTextSize(3);

  RTC_Date date;
  RTC_Time time;
  parseDateTime(doc[0]["created"].as<String>(),&date,&time);
  canvas.drawString(String(date.year)+String(date.mon)+String(date.day)+String(time.hour)+String(time.min)+String(time.sec), 45, 350);

  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  M5.shutdown();
}

void loop()
{
}