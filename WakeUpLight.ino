/*
 This code is just a simple demo, feel free to add your own
 touch to it!
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#ifndef STASSID
#define STASSID "PUT UR SSID HERE"
#define STAPSK  "PUT UR PASSWORD HERE"
#endif

#define LED 4
#define PIR 5

uint8_t light_state = 0;
uint16_t light_updaterate = 0;

const char* ssid = STASSID;
const char* password = STAPSK;

/* ============================= NTP ============================= */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/* ============================= TCP? ============================== */
WiFiServer wifiServer(8421);

/* ======================== STATIC IP ============================= */
IPAddress staticIP(10, 0, 0, 148);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1);

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(PIR, INPUT);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);

  String newHostname = "DeskNodeMCU";

  WiFi.hostname(newHostname.c_str());
  WiFi.config(staticIP, gateway, subnet, dns);
  
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* ============================= NTP ============================= */
  timeClient.setTimeOffset(3 * 3600); // Set timezone (timezone * 3600)
  timeClient.begin();                 // Start the timeclient

  while(1) {
    bool success = timeClient.update();                // Get the time from the NTP

    if (success == false) {
      digitalWrite(LED, 1);
      delay(125);
      digitalWrite(LED, 0);
      delay(125);

      digitalWrite(LED, 1);
      delay(125);
      digitalWrite(LED, 0);
      delay(125);

      digitalWrite(LED, 1);
      delay(125);
      digitalWrite(LED, 0);

      delay(5000);
    }else{
      digitalWrite(LED, 0);
      delay(250);
      digitalWrite(LED, 1);
      delay(250);
      digitalWrite(LED, 0);
      delay(250);
      break;
    }
  }
  
  /* ============================= TCP ============================= */
  // Open putty, use the raw connection to port 8421 and try it out!
  // The commands can be found below ("1;", "0;", "time;" and "mot;")
  // This is just for testing and otherwise you don't need this
  wifiServer.begin();
}

long oldLightUpdate = 0;

void loop() {
  ArduinoOTA.handle();

  // Update the time from the NTP every eight hours in case that the onboard time drifts
  // Not sure if this is neccessary or not.
  if (millis() - oldLightUpdate > 8 * 60 * 60 * 1000) {
    timeClient.update();
    oldLightUpdate = millis();
  }

  // Get the time (from the device)
  uint8_t hour = timeClient.getHours();
  uint8_t minute = timeClient.getMinutes();

  // 6:30
  // Turn the light on
  if (hour == 6 && minute == 30) {
    light_state = 1;
    light_updaterate = 300;
  }

  // 7-22
  if (hour >= 7 && hour < 21) {
    light_state = 1;
    light_updaterate = 300;
  }

  // 22:00
  // Turn the light off
  if (hour == 22 && minute == 0) {
    light_state = 0;
    light_updaterate = 300;
  }

  runLight();


  /* === HANDLE THE TCP CONNECTIONS === */
  WiFiClient client = wifiServer.available();

  uint8_t mot = 0;

  if (client) {
    if (client.connected())
    {
      Serial.println("Client Connected");
    }

    while (client.connected()) {
      ArduinoOTA.handle();
      runLight();

      if(mot == 1) {
        if(digitalRead(PIR)) {
          light_state = 1;
          light_updaterate = 0;
        }else{
          light_state = 0;
          light_updaterate = 0;
        }
      }

      while (client.available() > 0) {

        // read data from the connected client
        String msg = client.readStringUntil(';');
        client.flush();

        client.print(msg);

        if (msg == "time") {
          client.println(timeClient.getFormattedTime());
          client.println("LIGHT: "+String(light_state));
        }

        if (msg == "1") {
          light_state = 1;
          light_updaterate = 0;
          mot = 0;
        }

        if (msg == "0") {
          light_state = 0;
          light_updaterate = 0;
          mot = 0;
        }

        if (msg == "mot") {
          mot = 1;
        }

      }

    }
    client.stop();
    
  }
}
