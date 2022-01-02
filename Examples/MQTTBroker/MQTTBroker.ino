/*
 Name:    MC_EspMQTTBroker.ino
 Created:  02/01/2022 12:00:00
 Author:  Calfizzi Mario
*/
#if defined(ESP8266)
  #include <ESP8266wifi.h>
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <wifi.h>
  #include <ESPmDNS.h>
#endif
#include <MC_EspMQTT.h>

const char* hostname  = "MQTTBroker";
const char* ssid      = ""; // Please change it
const char* password  = "";


void startWiFi()
{
  uint32_t ms = millis();
	if (strlen(ssid)==0)
		Serial.println("****** PLEASE MODIFY ssid/password *************");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setHostname(hostname);
 
  Serial.print("WiFi connection..");
  while (WiFi.status() != WL_CONNECTED && millis()-ms <10000) {   
    Serial.print('.');
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED )
  {
    ESP.restart();
    //esp_restart();
  }
  Serial.println();
  WiFi.setHostname(hostname);
  if (!MDNS.begin    ( hostname )) 
  {
    Serial.println("Error setting up MDNS responder ( " + String(hostname) +  " )!");
  }else
    Serial.println("mDNS responder started ( " + String(hostname) +  " )");

  Serial.println( "\r\nConnected to " + String(ssid) + ", local IP address: " + String(WiFi.localIP().toString()));  

}

MQTTBroker broker;

void onNewClient(MQTTClient* client, const char *clientID)
{
  Serial.println();
  Serial.print(client->IP.toString()); 
  Serial.print(" Connected  as ");
  Serial.println(clientID);
}
void onSubscribe(MQTTClient* client, std::vector<String> &Topics)
{
  Serial.println();
  Serial.print(client->IP.toString()); 
  Serial.println(" subscribed to: ");
  for (size_t i = 0; i < Topics.size(); i++)
    Serial.println("  - " + Topics[i]);
  
}
void onUnsubscribe(MQTTClient* client, std::vector<String> &Topics)
{
  Serial.println();
  Serial.print(client->IP.toString()); 
  Serial.println(" unsubscribed to: ");
  for (size_t i = 0; i < Topics.size(); i++)
    Serial.println("  - " + Topics[i]);
}
void onPublish(MQTTClient* client, MQTTMessage::Topic &Topic)
{
  Serial.println();
  Serial.print(client->IP.toString()); 
  Serial.println(" published : ");
  Serial.println("  - Topic: " + Topic.name);
  Serial.println("  - Value: " + Topic.value);
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Started!");

  startWiFi();

  broker.onNewClient    ( onNewClient);
  broker.onSubscribe    ( onSubscribe);
  broker.onUnsubscribe  ( onUnsubscribe);
  broker.onPublish      ( onPublish);
  broker.begin();
  Serial.println("MQTT Broker Started");
}

// the loop function runs over and over again until power down or reset
void loop() {
  broker.handle();
}
