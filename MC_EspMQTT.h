/*

  Module: MC_EspMQTT
  Author: Mario Calfizzi (MC)

  Description:
      **First ESP8266/ESP32 MQTT Broker And Client!**

      Transform your ESP in a your home MQTT broker, take the control of your Smart Home devices.

      This Library ia able to manage your MQTT comunication with your home devices. It is composed by an **MQTTBroker** and **MQTTClinet**. You can create in your Home Network One Broker and variuos clients, to manage Light, Swicth, etc.

   Location: https://github.com/calfizzi/MC_EspMQTT

*/

#pragma once
#ifndef MC_EspMQTT_h
#define MC_EspMQTT_h
#include <arduino.h>
#if defined(ESP8266)
  #include <ESP8266wifi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif
#include <vector>
#include <string>
#include <functional>

//#define MQTT_DEBUG
#define MQTT_DEBUG_LEVEL 0 // 0-10
#define TEST_MQTTMESSAGE_MANAGER 0


#define TCPClient WiFiClient
#define TCPServer WiFiServer


class MQTTMessage;
class MQTTClient;
class MQTTBroker;

class MQTTbuffer
{
protected:
  uint8_t *_getBuffer   ( size_t min_size, int shiftBytesFromCode=0); // max 4096
  void     _init        ( void );
  void     _freeBuffer  ( void );
  uint8_t  *buffer = NULL;
  size_t    buffer_size = 0;
};

class MQTTMessage : public MQTTbuffer
{

  public:
    friend MQTTClient;
    friend MQTTBroker;
    enum Type
    {
      Unknown     = 0x00,
      Connect     = 0x10,
      ConnAck     = 0x20,
      Publish     = 0x30,
      PubAck      = 0x40,
      PubRec      = 0x50,
      PubRel      = 0x60,
      PubComp     = 0x70,
      Subscribe   = 0x80,
      SubAck      = 0x90,
      UnSubscribe = 0xA0,
      UnSuback    = 0xB0,
      PingReq     = 0xC0,
      PingResp    = 0xD0,
      Disconnect  = 0xE0,
      Reserved    = 0xF0
    };
    enum State
    {
      undefined = -1,
      Complete = 0,
      SentConnection,
      SentSubscription,
      GotSubAck,
      GotSubscription,
      SentPublish,
      GotPublish,
      GotPubAck,
      SentPubRec,
      GotPubRec,
      SentPubRel,
      GotPubRel,
      GotPubComp,
      Error

    };
    enum QoSs
    {
      QoS0 = 0,
      QoS1,
      QoS2,
      QoS3,
    };
    enum ConnetionStatus
    {
      Accepted = 0,
      UnacceptableProtocolVersion,
      IdentifierRejected,
      BrokerUnavailable
    };
    enum ConnetionStatusExtendex
    {
      none =  0,
      SesionPresent = 256 // only  for MQTT MQTT-3.2.0-1
    };
#if TEST_MQTTMESSAGE_MANAGER 
    uint16_t messageID = 0;
    QoSs QoS = QoSs::QoS0;
#endif
    typedef struct _Topic
    {
      String name;
      String value;
      bool   retain;
      MQTTMessage::QoSs QoS;
      _Topic (String topicName, String topicValue, bool   topicRetain=true, MQTTMessage::QoSs topicQoS = MQTTMessage::QoS1) : 
        name    ( topicName),
        value   ( topicValue),
        retain  ( retain),
        QoS     ( QoS)  {}
      _Topic (String topicName, bool topicRetain=false, MQTTMessage::QoSs topicQoS = MQTTMessage::QoS1) :
        name    ( topicName),
        value   ( ""),
        retain  ( retain),
        QoS     ( QoS)  {}
      _Topic(){};
    } Topic;
    typedef struct _PublishMessage : Topic
    {
      bool dupFlag;
      int messageID;
    } PublishMessage;
                   ~MQTTMessage             ( void );
                    MQTTMessage             ( void );
#if TEST_MQTTMESSAGE_MANAGER 
                    MQTTMessage             ( MQTTClient * client );
#endif
protected:
    void            reset                   ( void );
    void            setQoS                  ( QoSs qos = QoSs::QoS0);
    void            setDupFlag              ( bool value);
    void            setRetain               ( bool value);
    Type            getType                 ( void );
    String          getTypeString           ( void );
    void            generateMessageLength   ( void );
    uint32_t        convertedMessageLength  ( void );
    uint32_t        getMessageLength        ( void );
    void            messageIndexToStart     ( void );
    QoSs            getQoS                  ( void );
    bool            getDup                  ( void );
    bool            getRetain               ( void );
    uint8_t         getNextByte             ( void );
    int             getNextInt              ( void );
    uint16_t        getRemainingBytes       ( uint8_t** bytes );
    String          bytesToString           ( uint8_t* bytes, size_t size);
    String          getRemainingString      ( void );
    String          getNextString           ( void );
    bool            hasNext                 ( void );
    void            readMessage             ( TCPClient &client );
    void            sendMessage             ( TCPClient &client );
    void            add                     ( uint8_t   value );
    void            add                     ( uint16_t  value );
    void            add                     ( String    value );
    void            addSubscribeTopic       ( String    Topic,  QoSs QoS = QoSs::QoS1 );
    void            addLast                 ( String value);
    void            addLast                 ( uint8_t *value, int valueSize);
    void            create                  ( Type type, QoSs qos = QoSs::QoS2, bool retain = false, bool dupFlag = false);
    void            hexdump                 ( const char* prefix, MQTTClient* client = NULL);
    void            operator=               ( MQTTMessage &msg);
    void            createPublishMessage    ( PublishMessage &publishMessage);
    void            createPublishMessage    ( String topic, String payload, QoSs qos = QoSs::QoS0, bool retain = false, bool dupFlag = false, uint16_t messageID = 0);
    void            createPublishMessage    ( String topic, uint8_t *payload, int payloadSize, QoSs qos = QoSs::QoS0, bool retain = false, bool dupFlag = false, uint16_t messageID = 0);
    void            createSubscribeMessage  ( void );
    PublishMessage  getAsPublishMessage     ( void );
#if TEST_MQTTMESSAGE_MANAGER 
    void            setClient               ( MQTTClient *client) {this->client=client;}
    void            readMessage             ( void );
    void            sendMessage             ( void );
    void            responseConnAck         ( uint16_t flag);
    State           handle                  ( void );
#endif
  protected:
    void           _init                    ( void );
    void            addBasic                ( void *data, int size);
    const uint16_t  MaxBufferLength = 4096;  //hard limit: 16k due to size decoding
    State           state;
    uint32_t        currentIndex = 0;
#if TEST_MQTTMESSAGE_MANAGER 
    MQTTClient     *client = NULL;
#endif
};

class MQTTSubscribeMessage : public MQTTMessage
{
  
public:
  MQTTSubscribeMessage() : MQTTMessage() 
  {
    this->createSubscribeMessage();
  }
  void addTopic(String Topic, QoSs QoS = QoS1)
  {
    MQTTMessage::addSubscribeTopic(Topic, QoS);
  }
  void addTopic(const char *Topic, QoSs QoS = QoS1)
  {
    MQTTMessage::addSubscribeTopic(String(Topic), QoS);
  }
};

class TopicList : public std::vector<MQTTMessage::Topic> 
{
public:
  int   indexOf ( String topic);
  void  add     ( String topic, String payload, bool retain, MQTTMessage::QoSs QoS);
};

typedef std::function< void(MQTTClient* client, const char* topic, uint8_t* payload,  size_t length) > MQTTOnMessageCallbackFunction;
typedef std::function< void(MQTTClient* client) > MQTTOnConnectCallbackFunction;

//typedef struct qos2PublishedMessages_s
//{
//  String Topic:
//};

template<typename _Tp> class mqttVector : 
  public std::vector<_Tp>
{
public:
  int indexOf(_Tp value)
  {
    for (size_t i = 0; i < this->size(); i++)
      if (this->at(i)==value)
        return i;
    return -1;
  }
  bool exists(_Tp value)
  {
    return this->indexOf(value)>=0;
  }
  bool remove(_Tp value)
  {
    //Serial.printf ("%d Remove ID = %d", __LINE__, (uint16_t)value);
    int index = this->indexOf(value);
    if (index>=0)
    {
      this->erase (this->begin()+index);
      return true;
    }
    return false;
  }
  void removeAt(int index)
  {
      this->erase (this->begin()+index);
  }
};

typedef struct messageIDItem_s
{
  MQTTMessage::QoSs QoS;
  uint16_t messageID;
  messageIDItem_s (){};
  messageIDItem_s (uint16_t messageID, MQTTMessage::QoSs QoS)
  {
    this->messageID = messageID;
    this->QoS = QoS;
  }

}messageIDItem;

//class mqttMessageIDVector : public  mqttVector<messageIDItem>
//{
//public:
//  void add(uint16_t messageID, MQTTMessage::QoSs QoS)
//  {
//    this->push_back(messageIDItem(messageID, QoS);
//  }
//  int indexOf(messageSequence value)
//  {
//    for (size_t i = 0; i < this->size(); i++)
//      if (this->at(i) == value)
//        return i;
//    return -1;
//  }
//  bool exists(_Tp value)
//  {
//    return this->indexOf(value) >= 0;
//  }
//  void remove(int index)
//  {
//    this->erase(this->begin() + index);
//  }
//};

class MQTTClient
{
private:
  bool isConnected = false;
  uint16_t                        messageID= 0;
  mqttVector<uint16_t>            inMessageIDs;
  mqttVector<uint16_t>            outMessageIDs;
  //mqttVector<MQTTMessage *>       PendingMessages;
protected:
  MQTTBroker                     *broker;
  TCPClient                      *client;
  MQTTMessage                     message;
  uint32_t                        alive_ms = 1000;
  mqttVector<String>              subscriptionTopics;
  String                          clientID;
  MQTTMessage::QoSs               QoS;
  MQTTOnMessageCallbackFunction   onMessageCallback = NULL;
  MQTTOnConnectCallbackFunction   onConnectCallback = NULL;
            MQTTClient      ( MQTTBroker *broker, TCPClient &client);
  void      publish         ( String topicName,  MQTTMessage &message);
  void      publish         ( String topicName,  TCPClient *client, MQTTMessage *message = NULL);
public:
  IPAddress IP;
  friend    MQTTMessage;
  friend    MQTTBroker;
            MQTTClient      ( void );
           ~MQTTClient      ( void );
  void      evaluateMessage ( void );
  void      handle          ( void );
  void      responseConnAck ( uint16_t flag);
  void      pingReq         ( TCPClient *client) ;
  bool      connect         ( String clientID, String broker, String username = "", String password = "", uint16_t port = 1883);
  bool      connected       ( void );
  void      publish         ( String topicName, String payload, bool retain = true, MQTTMessage::QoSs QoS = MQTTMessage::QoS1, uint16_t messageID = 0);
  void      subscribe       ( String topicName, MQTTMessage::QoSs QoS = MQTTMessage::QoS1);
  void      subscribe       ( std::vector<String> &topicNames, MQTTMessage::QoSs QoS = MQTTMessage::QoS1);
  void      subscribe       ( MQTTSubscribeMessage &Message );
  // events
  void      onMessage       ( MQTTOnMessageCallbackFunction callback);
  void      onConnect       ( MQTTOnConnectCallbackFunction callback);
};

typedef std::function< void(MQTTClient* client, const char *clientID) >         MQTTOnNewClienCallbackFunction;
typedef std::function< void(MQTTClient* client, std::vector<String> &Topics) >  MQTTOnSubscribeCallbackFunction;
typedef std::function< void(MQTTClient* client, std::vector<String> &Topics) >  MQTTOnUnsubscribeCallbackFunction;
typedef std::function< void(MQTTClient* client, MQTTMessage::Topic &Topic) >    MQTTOnPublishCallbackFunction;
//typedef std::function< void(MQTTClient* Client) > MQTTOnConnectCallbackFunction;


class MQTTBroker
{
public:
  friend MQTTClient;
  void  begin                   ( uint16_t port = 1883, char *username = NULL, char *password =NULL );
        MQTTBroker              ( void );
       ~MQTTBroker              ( void );
  void  Handle                  ( void );
  int   IndexOf                 ( TCPClient *client);
  int   IndexOf                 ( String topic);
  void  distribuitePublish      ( String topic, MQTTMessage &message);
  void  sendSubscribedMessages  ( String topic, MQTTClient *client);
  void  onNewClient             ( MQTTOnNewClienCallbackFunction callback);
  void  onSubscribe             ( MQTTOnSubscribeCallbackFunction callback);
  void  onUnsubscribe           ( MQTTOnUnsubscribeCallbackFunction callback);
  void  onPublish               ( MQTTOnPublishCallbackFunction callback);
protected:
  void  showClients             ( void );
  String username=""; 
  String password=""; 
  MQTTOnNewClienCallbackFunction    newClientCallback   = NULL;
  MQTTOnSubscribeCallbackFunction   subscribeCallback   = NULL;
  MQTTOnUnsubscribeCallbackFunction unsubscribeCallback = NULL;
  MQTTOnPublishCallbackFunction     publishCallback     = NULL;
private:
  uint16_t port = 1883 ;
  TCPServer *server = NULL;
  std::vector<MQTTClient*> clients;
};


#endif
