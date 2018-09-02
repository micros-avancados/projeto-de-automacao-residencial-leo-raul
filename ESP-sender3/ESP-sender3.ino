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
const char *mqttServer = "m14.cloudmqtt.com";
const int mqttPort = 13577;
const char *mqttUsername = "zizudmqz";
const char *mqttPassword = "aSTXTcMLv9BS";
const char *mqttClientName = "espClient1";

/* Temperature sensor configuration */
int publishInterval = 15000;
long lastPublishMillis;
int analogValue;
float mv;
float t;

/* Network configuration */
boolean cFlag = false;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Working...");
  wifiConnect();
  startWebServer();
  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (!cFlag)
  {
    WiFiManager wifiManager;
    if (!wifiManager.startConfigPortal("OnDemandAP"))
    {
      delay(3000);
      cFlag = false;
      ESP.reset();
      delay(5000);
    }
    cFlag = true;
  }
  if (cflag)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected to network!");
      while (!client.connected())
      {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP8266Reader", mqttUsername, mqttPassword))
        {
          Serial.println("Connected!");
          client.setCallback(callback);
          client.subscribe("home/temperature");
        }
        else
        {
          Serial.print("Failed with state ");
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

void wifiConnect()
{
  Serial.print("Connecting to network...");
  WiFi.begin();
  if (!cFlag)
  {
    Serial.println("No network configuration found!");
  }
  if (cFlag)
  {
  Serial.print("Wi-Fi connected! IP address: ");
  Serial.println(WiFi.localIP());
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  if (topic == "update")
  {
    updateFirmware();
  }
  Serial.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  t = atof((char *)payload);
  Serial.println();
  Serial.println("#########################");
}

void sendTemperature()
{
  analogValue = analogRead(outputPin);
  mv = (analogValue/1024.0) * 3300;
  t = mv / 10;
  Serial.print("Temperature: ");
  Serial.print(t);
  client.publish("home/temperature", String(t));
}

void startWebServer()
{
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
}

void updateFirmware()
{
  httpServer.handleClient();
}
