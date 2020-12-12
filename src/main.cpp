#include <M5EPD.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define LGFX_M5PAPER
#include <efontEnableJa.h>
#include <efontFontData.h>
#include <LovyanGFX.hpp>

#include "nscanf.h"
#include "config.h"

// 何日分のデータを算出し保持するか
#define DATA_COUNT 7

const String AMBIENT_HOST = "http://ambidata.io/api/v2/";

static LGFX gfx;

DynamicJsonDocument fetch(String url)
{
  DynamicJsonDocument doc(65535);

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
    return true;
  }
  else return false;
}

String dateTimeToString(RTC_Date date, RTC_Time time) {
  return String(date.year)+String("-")+String(date.mon)+String("-")+String(date.day)
    +String("+")+String(time.hour)+String(":")+String(time.min)+String(":")+String(time.sec);
}

String dateToString(RTC_Date date) {
  // return String(date.year)+String("-")+String(date.mon)+String("-")+String(date.day);
  return String(date.mon)+String("/")+String(date.day);
}

typedef struct DateValuePair
{
    RTC_Date date;
    float value;
    DateValuePair() : date(), value() {}
    DateValuePair(RTC_Date date, float value) : date(date), value(value) {}
} date_value_pair_t;

// 日毎に翌日との差分を取得する(翌日がない場合は最新値との差分)
// 積算消費電力から一日ごとの消費電力を算出するのに使用
// 新しいデータ->古いデータの順に並んでいるという想定
// 実装した後に思ったけどもっと効率の良い方法ありますね
void generateValueOfDays(JsonArray doc, DateValuePair * out, uint8_t outSize) {
  uint8_t dataCursor = 0;
  
  gfx.println(String(doc.size()));
  for(int i = 0; i < doc.size(); i++) {
    JsonVariant v = doc[i];
    RTC_Date date;
    RTC_Time time;
    parseDateTime(v["created"].as<String>(),&date,&time);
    // 時刻が0:00だったらデータを遡り翌日の0:00を探す
    if(time.hour == 0 && time.min == 0 && time.sec == 0) {
      Serial.println(v["created"].as<String>() + v["d1"].as<String>());
      for(int ti = i-1;ti >= 0;ti--) {
        if(ti < 0) ti = 0;
        JsonVariant tv = doc[ti];
        RTC_Date tDate;
        RTC_Time tTime;
        parseDateTime(tv["created"].as<String>(),&tDate,&tTime);
        if(ti == 0 || (tTime.hour == 0 && tTime.min == 0 && tTime.sec == 0)) {
          // 翌日の0:00または最新の値に到達したので、dataへ追加する
          out[dataCursor] = DateValuePair(date,tv["d1"].as<float>() - v["d1"].as<float>());
          dataCursor++;
          if(dataCursor == outSize) return;
          break;
        }
      }
    }
  }
  return;
}

// 関数にすることでリターン時にdocが破棄される
void fetchData(DateValuePair *out) {
  // Ambientから情報を取得
  String url = AMBIENT_HOST + String("channels/") + AMBIENT_CHANNEL_ID + String("/data?readKey=") + AMBIENT_READ_KEY + String("&n=420");
  DynamicJsonDocument doc = fetch(url);
  // データの変換
  generateValueOfDays(doc.as<JsonArray>(),out,DATA_COUNT);
}

void drawGraph(DateValuePair *data, uint8_t dataSize, int32_t x, int32_t y, int32_t width, int32_t height) {
  const int32_t labelWidth = 36; //ラベル領域の幅
  // 軸を描画
  gfx.setColor(TFT_BLACK);
  gfx.drawFastHLine(x+labelWidth,y+height-labelWidth,width-labelWidth);//X軸
  gfx.drawFastVLine(x+labelWidth,y,height-labelWidth);//Y軸
  gfx.setTextDatum(textdatum_t::middle_right);
  gfx.setFont(&fonts::lgfxJapanGothic_24);

  // Y軸のグリッドとラベル
  int32_t innerHeight = height - labelWidth;
  for(int i = 0; i < 10; i++) {
    gfx.drawFastHLine(x+labelWidth,y+(i * innerHeight/10),width-labelWidth,TFT_LIGHTGREY);
    gfx.setColor(TFT_BLACK);
    gfx.drawString(String((10-i)),x + labelWidth-8,y+(i * innerHeight/10));
  }

  // X軸の棒とラベル
  gfx.setColor(TFT_BLACK);
  gfx.setTextDatum(textdatum_t::middle_center);
  int32_t innerWidth = width - labelWidth;
  int32_t barWidth_d2 = (int32_t) (innerWidth * 0.7f / dataSize / 2); //棒グラフの各棒の幅/2
  for(int i = 0; i < dataSize; i++) {
    int32_t xCenter = x+labelWidth + (innerWidth*(i+1)/(dataSize+1));
    int32_t barHeight = (int32_t)(innerHeight * (data[dataSize-1-i].value * 0.1f));
    gfx.fillRect(xCenter-barWidth_d2,y+innerHeight-barHeight,barWidth_d2*2,barHeight);
    gfx.drawString(dateToString(data[dataSize-1-i].date),xCenter,y+innerHeight+24);
  }
}

void setup()
{
  M5.begin();
  M5.RTC.begin();
  gfx.init();
  gfx.setRotation(3);
  gfx.setEpdMode(epd_mode_t::epd_quality);
  gfx.setFont(&fonts::lgfxJapanGothic_36);
  WiFi.begin(WIFI_SSID,WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
  }

  // データを取得
  DateValuePair data[DATA_COUNT];
  fetchData(data);

  // 描画開始
  gfx.setTextColor(TFT_BLACK);
  for(DateValuePair i : data) {
    gfx.println(dateToString(i.date)+String(" : ")+String(i.value));
  }
  drawGraph(data,DATA_COUNT,300,46,598,466);
  delay(3000);

  // 電源OFF
  M5.shutdown();
}

void loop()
{
}