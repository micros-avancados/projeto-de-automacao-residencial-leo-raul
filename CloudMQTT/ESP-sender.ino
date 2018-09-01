#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
#define outputPin A0

/* Wireless connection configuration */
const char* ssid = "conecta";
const char* password = "teste123";

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

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Working...");
  wifiConnect();
  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to Wi-Fi!");
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

void wifiConnect()
{
  Serial.print("Connecting to network: ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
   delay(500);
   Serial.print(".");
  }
  Serial.print("Wi-Fi connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
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