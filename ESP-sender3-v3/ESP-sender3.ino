#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <stdlib.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#define BUFFER_SIZE 100
#define outputPin A0

/* Web server configuration */
const char *updateHost = "espsender-webupdate";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

/* MQTT server configuration */
const char *mqttServer = "m20.cloudmqtt.com";
const int mqttPort = 10604;
const char *mqttUsername = "vozgepqg";
const char *mqttPassword = "lYfz7YB48mHL";
const char *mqttClientName = "espClient1";

/* Temperature sensor configuration */
int publishInterval = 20000;
long lastPublishMillis;
int analogValue;
float mv;
float t;

/* Network configuration */
boolean cFlag = false;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

boolean updateFlag = false;

void setup()
{
  Serial.begin(115200);
  Serial.println("Working...");
  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (!cFlag)
  {
    WiFiManager wifiManager;
    if (!wifiManager.startConfigPortal("ESPSender"))
    {
      delay(3000);
      ESP.reset();
      delay(5000);
    }
    cFlag = true;
  }
  if (cFlag)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      //Serial.println("Connected to network!");
      while (!client.connected())
      {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP8266Sender", mqttUsername, mqttPassword))
        {
          Serial.println("Connected!");
          client.setCallback(callback);
          client.subscribe("home/temperature");
          client.subscribe("home/update1");
        }
        else
        {
          Serial.println("Failed with state ");
          Serial.print(client.state());
          delay(5000);
        }
      }
      if (client.connected())
      {
        client.loop();
      }
      if (millis() - lastPublishMillis > publishInterval)
      {
       sendTemperature();
       lastPublishMillis = millis();
      }
    }
    delay(5000);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("\nMessage arrived in topic: ");
  Serial.print(topic);
  Serial.println("\nMessage: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  t = atof((char *)payload);
  if (t == 75.25)
  {
    startWebServer();
    updateFlag = true;
    delay(10000);
    while(updateFlag)
    {
      updateFirmware();
    }
  }
  Serial.println();
  Serial.println("#########################");
}

void sendTemperature()
{
  analogValue = analogRead(outputPin);
  mv = (analogValue/1024.0) * 3300;
  t = mv / 10;
  Serial.println("Temperature: ");
  Serial.print(t);
  {
    String payload = "";
    payload += t;
    payload += "";
    client.publish("home/temperature", (char*)payload.c_str());
  }
}

void startWebServer()
{
  MDNS.begin(updateHost);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
}

void updateFirmware()
{
  httpServer.handleClient();
}
