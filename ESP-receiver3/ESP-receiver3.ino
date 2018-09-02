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

int outputpin = A0;
IRsend irsend(4);
//IR frequency = 38KHz
int frequency = 38;

/* Web server configuration */
const char* updateHost = "espreceiver-webupdate";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

/* MQTT server configuration */
const char *mqttServer = "m14.cloudmqtt.com";
const int mqttPort = 13577;
const char *mqttUsername = "zizudmqz";
const char *mqttPassword = "aSTXTcMLv9BS";
const char *mqttClientName = "espClient2";

/* IR data values */
uint16_t on[] = {4600, 4150, 800, 1350, 800, 300, 750, 1400, 750, 1400, 750, 300, 800, 300, 750, 1400, 750, 350, 750, 300, 750, 1450, 700, 350, 700, 400, 700, 1450, 700, 1450, 700, 400, 650, 1500, 650, 400, 700, 400, 650, 450, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1550, 600, 1500, 650, 1550, 600, 450, 600, 500, 600, 450, 650, 450, 650, 450, 600, 450, 650, 450, 600, 1550, 600, 500, 600, 450, 650, 1500, 600, 500, 600, 500, 600, 1550, 600, 1550, 600, 500, 600, 1500, 650, 1550, 600, 450, 600, 1550, 600, 1550, 600};
uint16_t off[] = {4500, 4200, 650, 1450, 700, 400, 650, 1450, 650, 1500, 650, 400, 650, 450, 650, 1500, 600, 450, 650, 450, 600, 1500, 650, 450, 600, 500, 600, 1500, 600, 1550, 600, 450, 600, 1550, 600, 450, 600, 1550, 600, 1550, 600, 1550, 600, 1550, 600, 450, 600, 1550, 600, 1500, 600, 1550, 600, 450, 600, 500, 600, 450, 600, 500, 550, 1550, 600, 500, 550, 500, 600, 1550, 600, 1500, 600, 1550, 600, 500, 550, 500, 600, 450, 600, 500, 600, 450, 600, 500, 600, 450, 600, 450, 600, 1550, 600, 1550, 600, 1550, 550, 1600, 550, 1550, 550};
float t; //temperature

/* Network configuration */
boolean cFlag = false;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Working...");
  pinMode(TRIGGER_PIN, INPUT);
  irsend.begin();
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
    }
    delay(5000);
    if (t < 26)
    {
      irsend.sendRaw(off, sizeof(off) / sizeof(off[0]), frequency);
      Serial.println("Command sent: off");
      delay(5000);
    }
    else if (t > 26)
    {
      irsend.sendRaw(on, sizeof(on) / sizeof(on[0]), frequency);
      Serial.println("Command sent: on");
      delay(5000);
    }
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
