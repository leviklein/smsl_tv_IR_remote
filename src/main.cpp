#include <Arduino.h>
#include <MillisTimerLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Ping.h>
#include "secrets.h"

#define LED_1 2

const String tv_url = "http://" + tv_ip.toString() + "/sony/";

enum STATES {
  WIFI_NOT_CONNECTED,
  TV_NOT_PINGABLE,
  TV_OFF,
  TV_ON
};

MillisTimerLib timer1(200);
MillisTimerLib timer2(500);
MillisTimerLib ping_timer(5000);
MillisTimerLib poll_timer(5000);

int STATE =  WIFI_NOT_CONNECTED;

// put function declarations here:

void setup() {
  Serial.begin(115200);

  pinMode(LED_1, OUTPUT);
  Serial.print("Program start!\n");

  WiFi.mode(WIFI_STA);
  // Attempt to connect to Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to wifi \"");
  Serial.print(ssid);
  Serial.println("\"");



}

bool check_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting.");
    WiFi.begin(ssid, password); // Attempt to reconnect
    STATE = WIFI_NOT_CONNECTED;
    return 1;
  }
  return 0;
}

bool ping_tv() {
  if (ping_timer.timer()) {
    if (!Ping.ping(tv_ip, 1)) {
      Serial.println("TV not pingable!");
      STATE = TV_NOT_PINGABLE;
      return 1;
    }

    Serial.println("TV pingable!!!");
    STATE = TV_OFF;
  }
  return 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (STATE) {
    case WIFI_NOT_CONNECTED: {
      Serial.print(".");
      if (timer2.timer()) {
        digitalWrite(LED_1, !digitalRead(LED_1));
        Serial.print("Blink!!! \n");

      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected.");
        STATE = TV_OFF;
        digitalWrite(LED_1, LOW);
      }
      delay(100);
      break;
    }
    case TV_OFF: {
      if(!check_wifi() && !ping_tv()) {
        if(poll_timer.timer()) {

        }
        // String url = tv_url + "guide/";
        // String json = "{\"method\":\"getSupportedApiInfo\",\"id\":5,\"params\":[{\"services\":[\"system\",\"avContent\"]}],\"version\":\"1.0\"}";

        // HTTPClient http;
        // http.begin(url);
        // http.addHeader("Content-Type", "application/json");

        // int httpCode = http.POST(json);

        // if (httpCode > 0) {
        //   Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        //   if (httpCode == HTTP_CODE_OK) {
        //     String payload = http.getString();
        //     Serial.println("Payload:");
        //     Serial.println(payload);
        //   }
        // } else {
        //   Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        // }
        //   http.end();
        // }

      }
      else {
        Serial.print("TV off ???? \n");

      }
      break;
    }
    case TV_NOT_PINGABLE: {
      ping_tv();
      if (timer1.timer()) {
        digitalWrite(LED_1, !digitalRead(LED_1));
        Serial.print("TV not pingable!!! \n");
      }
      break;
    }
  }

}