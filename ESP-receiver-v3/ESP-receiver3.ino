#include <IRrecv.h>
#include <IRsend.h>
#include <IRtimer.h>
#include <IRutils.h>
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#define TRIGGER_PIN 0

IRsend irsend(4);
//IR frequency 32KHz
int frequency = 32;

/* Web server configuration */
const char* updateHost = "espreceiver-webupdate";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

/* MQTT server configuration */
const char *mqttServer = "m20.cloudmqtt.com";
const int mqttPort = 10604;
const char *mqttUsername = "vozgepqg";
const char *mqttPassword = "lYfz7YB48mHL";
const char *mqttClientName = "espClient2";

/* IR data values */
uint16_t on[] = {4600, 4150, 800, 1350, 800, 300, 750, 1400, 750, 1400, 750, 300, 800, 300, 750, 1400, 750, 350, 750, 300, 750, 1450, 700, 350, 700, 400, 700, 1450, 700, 1450, 700, 400, 650, 1500, 650, 400, 700, 400, 650, 450, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1550, 600, 1500, 650, 1550, 600, 450, 600, 500, 600, 450, 650, 450, 650, 450, 600, 450, 650, 450, 600, 1550, 600, 500, 600, 450, 650, 1500, 600, 500, 600, 500, 600, 1550, 600, 1550, 600, 500, 600, 1500, 650, 1550, 600, 450, 600, 1550, 600, 1550, 600};
uint16_t off[] = {4500, 4200, 650, 1450, 700, 400, 650, 1450, 650, 1500, 650, 400, 650, 450, 650, 1500, 600, 450, 650, 450, 600, 1500, 650, 450, 600, 500, 600, 1500, 600, 1550, 600, 450, 600, 1550, 600, 450, 600, 1550, 600, 1550, 600, 1550, 600, 1550, 600, 450, 600, 1550, 600, 1500, 600, 1550, 600, 450, 600, 500, 600, 450, 600, 500, 550, 1550, 600, 500, 550, 500, 600, 1550, 600, 1500, 600, 1550, 600, 500, 550, 500, 600, 450, 600, 500, 600, 450, 600, 500, 600, 450, 600, 450, 600, 1550, 600, 1550, 600, 1550, 550, 1600, 550, 1550, 550};
float t; //temperature

/* Network configuration */
boolean cFlag = false;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

boolean updateFlag = false;

void setup()
{
  Serial.begin(115200);
  Serial.println("Working...");
  pinMode(TRIGGER_PIN, INPUT);
  irsend.begin();
  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (!cFlag)
  {
    WiFiManager wifiManager;
    if (!wifiManager.startConfigPortal("ESPReceiver"))
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
      Serial.println("Connected to network!");
      while (!client.connected())
      {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP8266Reader", mqttUsername, mqttPassword))
        {
          Serial.println("Connected!");
          client.setCallback(callback);
          client.subscribe("home/temperature");
          client.subscribe("home/update2");
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
      if (t < 24)
      {
        Serial.println(t);
        irsend.sendRaw(off, sizeof(off) / sizeof(off[0]), frequency);
        Serial.println("Command sent: off");
        delay(5000);
      }
      else if (t > 24)
      {
        Serial.println(t);
        irsend.sendRaw(on, sizeof(on) / sizeof(on[0]), frequency);
        Serial.println("Command sent: on");
        delay(5000);
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
