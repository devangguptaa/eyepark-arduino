#include <Arduino.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson (use v6.xx)
#include <ESP8266WiFi.h>
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <SoftwareSerial.h>
#include <WiFiClientSecure.h>
#include "secrets.h"
#include <time.h>

const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[] = "entrygate/valid";

uint8_t DST = 0;
WiFiClientSecure net;

// ESP TX => Mega Pin 19 (2 in binary is 10)
// ESP RX => Mega Pin 3 (3 in binary is 11)
SoftwareSerial Mega(19, 18);

// SSL Certificates for AWS IoT Core Authorization
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

PubSubClient client(net);

unsigned long lastMillis = 0;
time_t now;
time_t nowish = 1510592825;

// **************
void connectToWiFi(String init_str);
void NTPConnect(void);
void checkWiFiThenMQTT(void);
void connectToMqtt();
void messageReceived(char *topic, byte *payload, unsigned int length);
void sendDataToAWS(void);
void setup();
void loop();
// **************

/**
 * Connects to the WiFi network
 */
void connectToWiFi(String init_str)
{
  Serial.print(init_str);

  WiFi.hostname(THINGNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println(" ok!");
}

/**
 * Connects to the SNTP server to set the cuurent Date and Time
 */
void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 28800, DST * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);

  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println(" done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

/**
 * Function checks if the ESP8266 is connected to the WiFi network and then connects to AWS IoT Core
 */
void checkWiFiThenMQTT(void)
{
  connectToWiFi("Checking WiFi");
  connectToMqtt();
}

/**
 * Function connects to AWS Iot Core using MQTT Protocol
 */
void connectToMqtt()
{
  Serial.print("MQTT connecting ");
  while (!client.connected())
  {
    if (client.connect(THINGNAME))
    {
      Serial.println("connected!");
    }
    else
    {
      Serial.print("failed, reason -> ");
      Serial.println(client.state());
      Serial.println(" < try again in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * Handler function for when a message is received from the AWS IoT Core on entrygate/valid topic
 * Prints the message on the Serial Monitor. Mega reads from the Serial monitor.
 * @param topic The topic the message was received on
 * @param payload The message received
 * @param length The length of the message received
 */
void messageReceived(char *topic, byte *payload, unsigned int length)
{
  String s = "";
  for (int i = 0; i < length; i++)
  {
    s = s + ((char)payload[i]);
  }
  Serial.println(s);
}

/**
 * Reads JSON data from the Mega and sends it to AWS IoT Core
 * The JSON data contains the topic and message to be sent to AWS IoT Core
 */
void sendDataToAWS(void)
{
  StaticJsonDocument<200> doc;

  // read data coming from Uno board and put into variable "doc"
  DeserializationError error = deserializeJson(doc, Serial.readString());

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print("deserializeJson() failed.");
    return;
  }
  const char *mqttTopic = doc["topic"];
  // parsing succeeded, continue and set time
  doc["time"] = String(millis());
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  if (!client.publish(mqttTopic, jsonBuffer, false))
  {
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Setup");

  Mega.begin(115200); // your esp's baud rate might be different
  delay(2000);

  connectToWiFi(String("Attempting to connect to SSID: ") + String(ssid));

  NTPConnect();

  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(messageReceived);
  connectToMqtt();
  client.subscribe(MQTT_SUB_TOPIC);
}

void loop()
{
  now = time(nullptr);

  if (!client.connected())
  {
    checkWiFiThenMQTT();
  }
  else
  {
    client.loop();
    if (millis() - lastMillis > 5000)
    {
      lastMillis = millis();
      sendDataToAWS();
    }
  }
}
