#include <Arduino.h>
#include <MillisTimerLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Ping.h>
#include <Preferences.h>
#include "secrets.h"
#include "smsl_keys.h"

Preferences preferences;

#include "TinyIRSender.hpp"

const String tv_url = "http://" + tv_ip.toString() + "/sony/";

enum STATES {
  WIFI_NOT_CONNECTED,
  TV_NOT_PINGABLE,
  TV_OFF,
  TV_ON
};

enum COMMANDS {
  POWER_ON,
  MUTE

};

MillisTimerLib timer1(100);       // fast blink
MillisTimerLib timer2(500);       // short blink
MillisTimerLib ping_timer(5000);
MillisTimerLib poll_timer(5000);
MillisTimerLib volume_timer(1500);

int STATE =  WIFI_NOT_CONNECTED;
int ping_fail = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Program start!\n");

  // Initialize preferences with a namespace (like a folder)
  preferences.begin("ir-remote", false); // "my-app" is the namespace, false means read/write

  // Write an integer
  VOLUME = preferences.getInt("volume");
  MUTED = preferences.getBool("muted");

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

int ping_tv() {
  if (ping_timer.timer()) {
    if (!Ping.ping(tv_ip, 1)) {
      ping_fail += 1;
      if (ping_fail == 3) {
        Serial.println("TV not pingable!");
        STATE = TV_NOT_PINGABLE;
        ping_fail = 0;
        return 1;
      }
      else {
        return 3;
      }
    }
    else {
      ping_fail = 0;
      return 2;
    }

    // Serial.println("TV pingable!!!");
    // STATE = TV_OFF;
  }
  return 0;
}

bool parseBraviaResponse(const String& response, JsonDocument& doc) {
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    Serial.print(F("Response was: "));
    Serial.println(response); // Print the raw response for debugging
    return false;
  }
  return true;
}

bool send_tv_api(const String& url, const String& body, JsonDocument& doc) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    bool exitCode = 1;
    int httpCode = http.POST(body);

    if (httpCode > 0) {
      // Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Serial.println("Payload:");
        // Serial.println(payload);
        if (parseBraviaResponse(payload, doc)) {
          // Example: Extracting a value (adapt to your specific response structure)
          // Serial.println("parse success");
          exitCode = 0;
        }
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      exitCode = 1;
    }
    http.end();
    return exitCode;
}

bool check_tv_on() {
  if (poll_timer.timer()) {
    String url = tv_url + "system/";
    String json = "{\"method\":\"getPowerStatus\",\"id\":50,\"params\":[],\"version\":\"1.0\"}";
    JsonDocument doc;
    Serial.println("Checking if TV is on");
    if (!send_tv_api(url, json, doc)) {
      String status = doc["result"][0]["status"];
      if (status.equals("active")) {
        STATE = TV_ON;
        Serial.println("TV is ON.");
        return 0;
      }
      else {
        STATE = TV_OFF;
        return 1;
      }
    }
    else {
      return 1;
    }
  }
  return 0;
}

std::pair<int, bool> get_tv_volume() {
  if (volume_timer.timer()) {
    String url = tv_url + "audio/";
    String json = "{\"method\":\"getVolumeInformation\",\"id\":33,\"params\":[],\"version\":\"1.0\"}";
    JsonDocument doc;
    if (!send_tv_api(url, json, doc)) {
      JsonArray data = doc["result"];
      for (JsonArray data1 : data) {
        for (JsonVariant data2 : data1) {
          String target = data2["target"];
          if (target.equals("speaker")) {
            int volume = data2["volume"];
            bool mute = data2["mute"];
            Serial.printf("correct! volume %d mute %d\n", volume, mute);

            return std::make_pair(volume, mute);
          }
        }
      }
    }
  }
  return std::make_pair(-1, false);
}
bool sendIRcommand(std::string code, uint8_t repeats) {
  Serial.printf("Sending code %s, %d times\n", code.c_str(), repeats);
  Serial.printf("address: %d, code %d, repeats %d\n", IR_CODES[code].first, IR_CODES[code].second, repeats);
  Serial.flush();

  for (int i = 0; i < repeats; i++) {
    sendNEC(IR_SEND_PIN, IR_CODES[code].first, IR_CODES[code].second, 1, false);
    delay(20);
  }
  return 0;
}


bool processVolume(int volume) {
  if (volume == -1) {
    return 1;
  }
  Serial.println("processing volume");
  int storedVolume = preferences.getInt("volume");
  Serial.printf("volume %d saved_vol %d\n", volume, storedVolume);
  int diff = (volume - storedVolume)/2;
  if (diff == 0) {
    return 0;
  }
  std::string command;
  if (diff > 0) {
    command = "UP";
  }
  else {
    command = "DOWN";
  }
  int repeats = abs(diff);
  preferences.putInt("volume", volume);
  sendIRcommand(command, repeats);
  return 0;
}

bool processMute(bool muted) {
  bool storedMute = preferences.getBool("muted");
  if (storedMute == muted) {
    return 0;
  }
  else {
    Serial.println("processing mute");
    sendIRcommand("MUTE", 1);
    preferences.putBool("muted", muted);
  }
  return 0;
}

void loop() {
  switch (STATE) {
    case WIFI_NOT_CONNECTED: {
      Serial.print(".");
      if (timer2.timer()) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        // Serial.print("Blink!!! \n");

      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected.");
        STATE = TV_OFF;
        digitalWrite(LED_BUILTIN, LOW);
      }
      delay(100);
      break;
    }
    case TV_OFF: {
      digitalWrite(LED_BUILTIN, LOW);

      if(!check_wifi() && !ping_tv()) {
        check_tv_on();
        if (STATE == TV_ON) { // TV turned on
          sendIRcommand("POWER",  1);
          delay(2000);
        }
      }
      else {
        Serial.print("TV off ???? \n");
      }
      break;
    }
    case TV_NOT_PINGABLE: {
      if (timer1.timer()) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }
      if(ping_tv() == 2) {
        STATE = TV_ON;
      }
      break;
    }
    case TV_ON: {
      digitalWrite(LED_BUILTIN, HIGH);
      if(!check_wifi() && !ping_tv() && !check_tv_on()) {
        std::pair<int, bool> result = get_tv_volume();
        if (result.first != -1 ) {
          Serial.printf("%d first, %d second \n", result.first, result.second);
          processVolume(result.first);
          processMute(result.second);
        }
      }
      else {
      }
      if (STATE == TV_OFF) { // TV turned off
          sendIRcommand("POWER",  1);
          delay(2000);
      }
      break;
    }
  }

}