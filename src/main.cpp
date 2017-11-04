#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <credentials.h>

#define SERIAL_BAUDRATE 115200
#define MQTT_CLIENT_ID "RC-MQTT-Gateway"
#define MQTT_TOPIC "ghs/livingroom/ir"
#define MQTT_TOPIC_COMMAND "ghs/livingroom/ir/command"
#define MQTT_TOPIC_ONLINE "ghs/livingroom/ir/online"
#define JSON_DOCUMENT_SIZE 256

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const uint16_t kRecvPin = D5;
const uint16_t kIrLed = D2; // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRrecv irrecv(kRecvPin);
IRsend irsend(kIrLed);
decode_results results;

void wifiSetup();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void mqttReconnect();

void setup()
{

  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();
  Serial.println();

  wifiSetup();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  irrecv.enableIRIn();
  irsend.begin();
}

void loop()
{
  if (!mqttClient.connected())
  {
    mqttReconnect();
  }
  mqttClient.loop();

  if (irrecv.decode(&results))
  {
    String jsonString = "{";

    // Protocol
    jsonString += "\"protocol\":\"";
    jsonString += typeToString(results.decode_type, results.repeat);
    jsonString += "\",";

    // Data
    jsonString += "\"data\":\"";
    jsonString += uint64ToString(results.value, 16);
    jsonString += "\",";

    // BitLength
    jsonString += "\"bitLength\":\"";
    jsonString += results.bits;
    jsonString += "\"";

    jsonString += "}";

    const char *payload = jsonString.c_str();

    mqttClient.publish(MQTT_TOPIC, payload);
    irrecv.resume(); // Receive the next value
  }
}

void mqttReconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, MQTT_TOPIC_ONLINE, 1, true, "false"))
    {
      Serial.println("Connected to mqtt server");

      mqttClient.publish(MQTT_TOPIC_ONLINE, "true", true);

      Serial.print("Subscribe to ");
      Serial.print(MQTT_TOPIC_COMMAND);
      Serial.println();

      mqttClient.subscribe(MQTT_TOPIC_COMMAND);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  StaticJsonDocument<JSON_OBJECT_SIZE(3)> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  const char *protocol = doc["protocol"];
  const char *data = doc["data"];
  int bitLength = doc["bitLength"];

  if (protocol && data && bitLength)
  {
    const decode_type_t decodeType = strToDecodeType(protocol);
    const uint64_t command = strtoll(data, NULL, 16);

    irrecv.disableIRIn();
    irsend.send(decodeType, command, bitLength);
    irrecv.enableIRIn();
  }
}

void wifiSetup()
{
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}
