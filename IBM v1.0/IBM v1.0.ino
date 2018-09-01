#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

/* customize the values below */
const char* ssid = "iPhone SE";
const char* password = "a1b2c3d4";

#define ORG "q00r3j"
#define DEVICE_TYPE "ESP8266"
#define DEVICE_ID "nodemcu"
#define TOKEN "IARnr0CyvSIYFRocGEnX"
/* customize the values above */

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";

void callback(char* topic, byte* payload, unsigned int payloadLength);

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 30000;
long lastPublishMillis;

void setup()
{
 Serial.begin(115200);
 Serial.println();
 dht.begin();
 wifiConnect();
 mqttConnect();
 initManagedDevice();
}

void loop() {
 if (millis() - lastPublishMillis > publishInterval)
 {
   publishData();
   lastPublishMillis = millis();
 }

 if (!client.loop())
 {
   mqttConnect();
   initManagedDevice();
 }
}

void wifiConnect()
{
 Serial.print("Connecting to ");
 Serial.print(ssid);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED)
 {
   delay(500);
   Serial.print(".");
 }
 Serial.print("nWiFi connected, IP address: ");
 Serial.println(WiFi.localIP());
}

void mqttConnect()
{
 if (!!!client.connected())
 {
   Serial.print("Reconnecting MQTT client to ");
   Serial.println(server);
   while (!!!client.connect(clientId, authMethod, token))
   {
     Serial.print(".");
     delay(500);
   }
   Serial.println();
 }
}

void initManagedDevice()
{
 if (client.subscribe("iotdm-1/response"))
 {
   Serial.println("subscribe to responses OK");
 }
 else
 {
   Serial.println("subscribe to responses FAILED");
 }

 if (client.subscribe(rebootTopic))
 {
   Serial.println("subscribe to reboot OK");
 }
 else
 {
   Serial.println("subscribe to reboot FAILED");
 }

 if (client.subscribe("iotdm-1/device/update"))
 {
   Serial.println("subscribe to update OK");
 }
 else
 {
   Serial.println("subscribe to update FAILED");
 }

 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.createObject();
 JsonObject& d = root.createNestedObject("d");
 JsonObject& metadata = d.createNestedObject("metadata");
 metadata["publishInterval"] = publishInterval;
 JsonObject& supports = d.createNestedObject("supports");
 supports["deviceActions"] = true;

 char buff[300];
 root.printTo(buff, sizeof(buff));
 Serial.println("publishing device metadata:");
 Serial.println(buff);
 if (client.publish(manageTopic, buff))
 {
   Serial.println("device Publish ok");
 }
 else
 {
   Serial.print("device Publish failed:");
 }
}

void publishData()
/* read temperature from DHT22 sensor and insert in payload */
float t = dht.readTemperature();

if (isnan(t)) /* check if any read failed and retry */
{
  Serial.println("Reading temperature from sensor failed!");
  return;
}

{
 String payload = "{\"d\":{\"temperature\":";
 payload += t;
 payload += "}}";

 Serial.print("Sending payload: ");
 Serial.println(payload);

 if (client.publish(publishTopic, (char*) payload.c_str()))
 {
   Serial.println("Publish OK");
 }
 else
 {
   Serial.println("Publish FAILED");
 }
}

void callback(char* topic, byte* payload, unsigned int payloadLength)
{
 Serial.print("callback invoked for topic: ");
 Serial.println(topic);

 if (strcmp (responseTopic, topic) == 0)
 {
   return; /* response printing */
 }

 if (strcmp (rebootTopic, topic) == 0)
 {
   Serial.println("Rebooting...");
   ESP.restart();
 }

 if (strcmp (updateTopic, topic) == 0)
 {
   handleUpdate(payload);
 }
}

void handleUpdate(byte* payload)
{
 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.parseObject((char*)payload);
 if (!root.success())
 {
   Serial.println("handleUpdate: payload parse FAILED");
   return;
 }
 Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial);
 Serial.println();

 JsonObject& d = root["d"];
 JsonArray& fields = d["fields"];
 for (JsonArray::iterator it = fields.begin(); it != fields.end(); ++it)
 {
   JsonObject& field = *it;
   const char* fieldName = field["field"];
   if (strcmp (fieldName, "metadata") == 0)
   {
     JsonObject& fieldValue = field["value"];
     if (fieldValue.containsKey("publishInterval"))
     {
       publishInterval = fieldValue["publishInterval"];
       Serial.print("publishInterval:"); Serial.println(publishInterval);
     }
   }
 }
}