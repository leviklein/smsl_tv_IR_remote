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

MillisTimerLib timer1(200);
MillisTimerLib timer2(500);
MillisTimerLib ping_timer(5000);
MillisTimerLib poll_timer(5000);
MillisTimerLib volume_timer(1000);

int STATE =  WIFI_NOT_CONNECTED;

// put function declarations here:

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Program start!\n");

  // Initialize preferences with a namespace (like a folder)
  preferences.begin("ir-remote", false); // "my-app" is the namespace, false means read/write

  // Write an integer
  preferences.getInt("volume");

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
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Payload:");
        Serial.println(payload);
        if (parseBraviaResponse(payload, doc)) {
          // Example: Extracting a value (adapt to your specific response structure)
          Serial.println("parse success");
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
      STATE = TV_OFF;
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
            Serial.printf("volume %d mute %d\n", volume, mute);

            return std::make_pair(volume, mute);
          }
        }
      }
    }
  }
  return std::make_pair(-1, false);
}
bool sendIRcommand(std::string code, int repeats) {
  Serial.printf("Sending code %s, %d times\n", code.c_str(), repeats);
  Serial.flush();
  sendNEC(IR_SEND_PIN, IR_CODES[code].first, IR_CODES[code].second, repeats, true);
  delay(1000);
  return 0;
}

bool processVolume(int volume) {
  if (volume == -1) {
    return 1;
  }
  Serial.println("processing volume");
  int storedVolume = preferences.getInt("volume");
  Serial.printf("volume %d saved_vol %d\n", volume, storedVolume);
  int diff = volume - storedVolume;
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
  int repeats;
  abs(diff) > 1 ? repeats = abs(diff) : repeats = 1;
  preferences.putInt("volume", volume);
  sendIRcommand(command, repeats);
  return 0;
}

void loop() {
  switch (STATE) {
    case WIFI_NOT_CONNECTED: {
      Serial.print(".");
      if (timer2.timer()) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        Serial.print("Blink!!! \n");

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
      }
      else {
        Serial.print("TV off ???? \n");
      }
      break;
    }
    case TV_NOT_PINGABLE: {
      ping_tv();
      if (timer1.timer()) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        Serial.print("TV not pingable!!! \n");
      }
      break;
    }
    case TV_ON: {
      digitalWrite(LED_BUILTIN, HIGH);
      if(!check_wifi() && !ping_tv() && !check_tv_on()) {
        std::pair<int, bool> result = get_tv_volume();
        processVolume(result.first);
        if (result.second) {
          sendIRcommand("MUTE", 1);
        }
      }
      else {
      }
      break;
    }
  }

}