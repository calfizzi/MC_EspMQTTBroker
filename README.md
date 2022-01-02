# MC_EspMQTTBroker Ver. 0.9
**First ESP8266/ESP32 MQTT Broker And Client!**

Transform your ESP in a your home MQTT broker, take the control of your Smart Home devices.

This Library ia able to manage your MQTT comunication with your home devices. It is composed by an **MQTTBroker** and **MQTTClinet**. You can create in your Home Network One Broker and variuos clients, to manage Light, Swicth, etc.

## MQTTBroker:
you can easly instance it in your Esp board
### Methods:
```c++
void begin              ( uint16_t port, char *username, char *password); // Listening Port (1883),  apply your Username 
                                                                          // and Password
void Handle             ( void );                                         // Loop function (using ESP32 you can place it 
                                                                          // in a specific thread)
void distribuitePublish ( String topic, MQTTMessage &message);            // Publish a specific topic/message to all 
                                                                          // subscribed clients   
                                                                          
// events
void onNewClient        ( MQTTOnNewClienCallbackFunction callback);       // New Client Callback to manage the clients 
                                                                          // Connections
void onSubscribe        ( MQTTOnSubscribeCallbackFunction callback);      // New Subscription Callback to manage the manage 
                                                                          // the subscriptions   
void onUnsubscribe      ( MQTTOnUnsubscribeCallbackFunction callback);    // Unsubscription Callback to manage the manage 
                                                                          // the unsubscriptions
void onPublish          ( MQTTOnPublishCallbackFunction callback);        // New Publish callback happen when a client send 
```
## MQTTClient:
you can easly instance it in your Esp board
### Methods:
```c++
  bool      connect         ( String clientID, String brokerAddress,  // Connect to the Brocker apply your Username,
                              String username, String password,       // Password and broker port
                              uint16_t port);
  void      handle          ( void );                                 // Loop function (using ESP32 you can place it in a 
                                                                      // specific thread)
  bool      connected       ( void );                                 // evaluate if still connected
  void      publish         ( String topicName, String payload,       // publish a specific Topic/message with retain and 
                              bool retain,                            // Qos options
                              MQTTMessage::QoSs QoS, 
                              uint16_t messageID);
  void      subscribe       ( String topicName,                       // Single Subscription to the broker
                              MQTTMessage::QoSs QoS);                
  void      subscribe       ( std::vector<String> &topicNames,        // Multiple Subscription to the broker
                              MQTTMessage::QoSs QoS);
  void      subscribe       ( MQTTSubscribeMessage &Message );        // One subscribe message with single or multiple 
                                                                      // topics

                              
  // events
  void      onMessage       ( MQTTOnMessageCallbackFunction callback);// New Message from the broker callback
  void      onConnect       ( MQTTOnConnectCallbackFunction callback);// on broker connectio callback
```

## Example of use...
### MQTTBroker: 
```c++
MQTTBroker broker;

void setup() 
{
  Serial.begin(115200);
  while(!Serial);
  // .... connect to your WiFi
  // .... connect to your WiFi
  // .... connect to your WiFi
  broker.begin();
}

void loop() {
  broker.Handle();
}
```

### MQTTClient: 
```c++

#define BUILTIN_LED 2

MQTTClient   client;
bool         lightStatus = LOW;
const char*  mqtt_broker = ""; // please enter your brocker address

void MQTTConnect()
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String MAC = WiFi.macAddress();
    MAC.replace((const char*)":",(const char*)"");
    String clientId = "ESP8266Client-" + MAC;
    if (client.connect(clientId, mqtt_broker)) {
      Serial.println("connected");
      delay(100);
      client.subscribe( "MyLight", MQTTMessage::QoSs::QoS1);
      client.publish  ( "MyLight", "0", true, MQTTMessage::QoSs::QoS1);
    } else {
      Serial.print("failed, rc=");
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void onMessageCallback(MQTTClient *client, const char* topic, uint8_t* payload, size_t length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (strcmp(topic, "MyLight")==0)
  {
    if ((char)payload[0] == '1') 
      lightStatus = HIGH; // Turn the LED on 
    else
      lightStatus = LOW;  // Turn the LED off
      
    digitalWrite(BUILTIN_LED, lightStatus);   
  }
}

void setup() 
{
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  // .... connect to your WiFi
  // .... connect to your WiFi
  // .... connect to your WiFi
  client.onMessage(onMessageCallback);
}

void loop() {
  if (!client.connected()) {
    MQTTConnect();
  }
  client.handle();
}
```

