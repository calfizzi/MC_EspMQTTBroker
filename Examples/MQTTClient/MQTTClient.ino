/*
 Name:		MQTTClient.ino
 Created:	1/2/2022 12:00:00 
 Author:	Calfizzi Mario
*/
#if defined(ESP8266)
  #include <ESP8266wifi.h>
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <wifi.h>
  #include <ESPmDNS.h>
#endif
#include <MC_EspMQTT.h>
// Update these with values suitable for your network.
//byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xE0 };

const char* ssid                  = "";
const char* password              = ""; // Please Change It
const char* hostname              = "MQTTClient";
const char* mqtt_server           = ""; // Please Change It

const char* LightTopic            = "MyLight";
const char* LightBrightnessTopic  = "MyLightBrightness";
#if defined(ESP32)
  const int   pwmFreq               = 5000;
  const int   pwmChannel            = 0;
  const int   pwmResolution         = 8;
#endif

uint8_t     lightPWMValue         = 0;
bool        lightStatus           = LOW;

#define     BUILTIN_LED           2

MQTTClient  client;

void setup_wifi() {
  uint32_t ms = millis();

  delay(10);
	if (strlen(ssid)==0)
		Serial.println("****** PLEASE MODIFY ssid/password *************");
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.setHostname(hostname);

  while (WiFi.status() != WL_CONNECTED && millis()-ms <10000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED )
    ESP.restart();

  Serial.println();
  WiFi.setHostname(hostname);
  if (!MDNS.begin    ( hostname )) 
    Serial.println("Error setting up MDNS responder ( " + String(hostname) +  " )!");
  else
    Serial.println("mDNS responder started ( " + String(hostname) +  " )");

  randomSeed(micros());

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void setupPWMChannel()
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
#if defined(ESP8266)
  analogWrite(BUILTIN_LED, 255-lightPWMValue);
#elif defined(ESP32)
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(BUILTIN_LED, pwmChannel);
  ledcWrite(pwmChannel, lightPWMValue);
#endif
}
int payloadToInt(uint8_t *payload, size_t length)
{
    char *value = (char*)malloc(length+1);
    memcpy(value, payload, length);
    value[length] = '\0';
    int returnValue = atoi(value);
    free(value);
    return returnValue ;

}
void onMessageCallback(MQTTClient *client, const char* topic, uint8_t* payload, size_t length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (strcmp(topic, LightTopic)==0)
  {
    if ((char)payload[0] == '1') {
      lightStatus = HIGH; // Turn the LED on 
      if (lightPWMValue==0)
      {
        lightPWMValue = 255;
        client->publish(LightBrightnessTopic, String((int)lightPWMValue), true, MQTTMessage::QoS2);
      }
      #if defined(ESP8266)
        analogWrite (BUILTIN_LED, 255-lightPWMValue);
      #elif defined(ESP32)
        ledcWrite(pwmChannel, lightPWMValue);
      #endif
    } else {
      lightStatus = LOW; // Turn the LED off 
      #if defined(ESP8266)
        analogWrite (BUILTIN_LED, 255);
      #elif defined(ESP32)
        ledcWrite(pwmChannel, 0);
      #endif
    }
  }else if (strcmp(topic, LightBrightnessTopic)==0) // Turn the LED on (Note that LOW is the voltage level
  {
    lightPWMValue = payloadToInt(payload, length);
    if (lightPWMValue == 0)
      lightStatus = LOW;
    else
      lightStatus = HIGH;
    if (lightStatus)
      #if defined(ESP8266)
        analogWrite (BUILTIN_LED, 255-lightPWMValue);
      #elif defined(ESP32)
        ledcWrite(pwmChannel, lightPWMValue);
      #endif
    client->publish(LightTopic, String((int)lightStatus), true, MQTTMessage::QoS2);
  }
}

void ConnectMQTTBroker() {
  // Loop until we're reconnected

  if (strlen(ssid)==0)
		Serial.println("****** PLEASE MODIFY ssid/password *************");

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String MAC = WiFi.macAddress();
    MAC.replace((const char*)":",(const char*)"");

    String clientId = "ESPClient-" + MAC;
    // Attempt to connect
    if (client.connect(clientId, mqtt_server)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      delay(100);
      MQTTSubscribeMessage subscribe;
      subscribe.addTopic(LightTopic);
      subscribe.addTopic(LightBrightnessTopic);
      client.subscribe(subscribe);
      client.publish(LightTopic, "0", true,MQTTMessage::QoS2);
      client.publish(LightBrightnessTopic, String(lightPWMValue), true,MQTTMessage::QoS2);
    } else {
      Serial.print("failed, rc=");
      //Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  setupPWMChannel();
#if defined(ESP32)
  ledcWrite(pwmChannel, 0);
#endif
  Serial.begin(115200);
  setup_wifi();
  client.onMessage(onMessageCallback);
}


void loop() {
  if (!client.connected())
    ConnectMQTTBroker();
  client.handle();
}
