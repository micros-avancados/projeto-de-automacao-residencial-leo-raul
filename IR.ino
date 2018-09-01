#include <IRrecv.h>
#include <IRsend.h>
#include <IRtimer.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>

#define TRIGGER_PIN 0

int outputpin = A0;

IRsend irsend(4);
//IR frequency = 38KHz
int frequency = 38;

const char *mqttServer = "m14.cloudmqtt.com";
const int mqttPort = 13577;
const char *mqttUsername = "zizudmqz";
const char *mqttPassword = "aSTXTcMLv9BS";

double t;

uint16_t on[] = {4600, 4150, 800, 1350, 800, 300, 750, 1400, 750, 1400, 750, 300, 800, 300, 750, 1400, 750, 350, 750, 300, 750, 1450, 700, 350, 700, 400, 700, 1450, 700, 1450, 700, 400, 650, 1500, 650, 400, 700, 400, 650, 450, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1500, 650, 1550, 600, 1500, 650, 1550, 600, 450, 600, 500, 600, 450, 650, 450, 650, 450, 600, 450, 650, 450, 600, 1550, 600, 500, 600, 450, 650, 1500, 600, 500, 600, 500, 600, 1550, 600, 1550, 600, 500, 600, 1500, 650, 1550, 600, 450, 600, 1550, 600, 1550, 600};
uint16_t off[] = {4500, 4200, 650, 1450, 700, 400, 650, 1450, 650, 1500, 650, 400, 650, 450, 650, 1500, 600, 450, 650, 450, 600, 1500, 650, 450, 600, 500, 600, 1500, 600, 1550, 600, 450, 600, 1550, 600, 450, 600, 1550, 600, 1550, 600, 1550, 600, 1550, 600, 450, 600, 1550, 600, 1500, 600, 1550, 600, 450, 600, 500, 600, 450, 600, 500, 550, 1550, 600, 500, 550, 500, 600, 1550, 600, 1500, 600, 1550, 600, 500, 550, 500, 600, 450, 600, 500, 600, 450, 600, 500, 600, 450, 600, 450, 600, 1550, 600, 1550, 600, 1550, 550, 1600, 550, 1550, 550};

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("\nWorking...");
  pinMode(TRIGGER_PIN, INPUT);
  irsend.begin();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
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
        client.subscribe("home/temperature");
      }
      else
      {
        Serial.print("Failed with state ");
        Serial.print(client.state());
        delay(2000);
      }
    }
  }

  delay(1000);

  if (client.loop())
  {
    //Serial.print("Entered loop...");
  }
  else
  {
    if (client.connect("ESP8266Reader", mqttUsername, mqttPassword))
    {
      Serial.println("Connected!");
      client.subscribe("home/temperature");
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  if (t < 25)
  {
    irsend.sendRaw(off, sizeof(off) / sizeof(off[0]), frequency);
    Serial.println("Command sent: off");
    delay(2000);
  }
  else if (t > 25)
  {
    irsend.sendRaw(on, sizeof(on) / sizeof(on[0]), frequency);
    Serial.println("Command sent: on");
    delay(2000);
  }
}