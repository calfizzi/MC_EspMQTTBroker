#include "MC_EspMQTT.h"



#if defined(MQTT_DEBUG) || (MQTT_DEBUG_LEVEL>1)
  #define mqttdebug(value) __debug(__LINE__, #value, value)
  #define mqttsdebug(value) __debug(__LINE__, value)
#else
  #define mqttdebug(value) 
  #define mqttsdebug(value)
#endif 
#if defined(MQTT_DEBUG) || (MQTT_DEBUG_LEVEL>0)
  #define mqttshow(value) __show(__LINE__, #value, value)
#else 
  #define mqttshow(value) 
#endif
#define mqttByteForLength(size) 1+ ((size>>8)>0? 1:0) + ((size>>16)>0? 1:0) + ((size>>24)>0? 1:0)
//#define mqttfdebug(formats, ...) {Serial.printf("%d - MQTT: ", __LINE__); Serial.print(" - MQTT: "); Serial.println(value);}

template <typename T> void __debug (uint16_t line, char* varName, T value)
{
  Serial.print(line); 
  Serial.print(" - MQTT: ");
  Serial.print(varName); 
  Serial.print(": "); 
  Serial.println(value);
} 

template <typename T> void __show(uint16_t line, char* varName, T value)
{
  Serial.print(line); 
  Serial.print(" - MQTT: ");
  Serial.print(varName); 
  Serial.print(": "); 
  Serial.println(value);
} 
void __debug(uint16_t line, String s)
{
#ifdef MQTT_DEBUG
  Serial.print(line); 
  Serial.print(" - MQTT: "); 
  Serial.println(s); 
#endif
}

TopicList AllTopics;

void _hexdump(const char* prefix, const char *buffer, size_t buffer_size, int state, int rowSize = 8, int split = 4)  
{
  Serial.print("\r\n");
  Serial.print(prefix);
  Serial.print(" size(");
  Serial.print(buffer_size);
  Serial.print("), state= ");
  Serial.print(state);
  Serial.println();
  String s = "";
  Serial.printf("%04d | ", 0);
  for (size_t i = 0; i < buffer_size; i++)
  {
    char c = buffer[i];
    s += c < 32 ? '.' : c;

    Serial.printf("%02X ", (int)c);
    int mod = (i + 1) % rowSize ;
    if (mod==4) 
    {
      Serial.print("  ");
      s+="  ";
    }
    if (mod== 0)
    {
      Serial.println("| " + s + " |");
      if (i+1<buffer_size)
        Serial.printf("%04d | ", i+1);
      s = "";
    }else if (i+1==buffer_size)
    {
      for (size_t i = 0; i < rowSize-mod; i++)
        Serial.print("   ");
      if (mod<split) Serial.print("  ");
      Serial.print("| " + s );
      for (size_t i = 0; i < rowSize-mod; i++)
        Serial.print(' ');
      if (mod<split) Serial.print("  ");
      Serial.println( " |");
    }

  }

}


uint8_t *MQTTbuffer::_getBuffer(size_t min_size, int shiftBytesFromCode) // max 4096
{
  if (min_size>buffer_size)
  {
    uint8_t *b = (uint8_t *)malloc(min_size);
    memset(b, 0, min_size);
    if (buffer_size>0)
    {
      b[0] = buffer[0];
      memcpy(&b[1+shiftBytesFromCode], &buffer[1], buffer_size-1);
      free(buffer);
    }
    buffer = b;
    buffer_size = min_size;
  }
  return buffer;
}
void MQTTbuffer::_init()
{
  if (buffer!=NULL)
    free(buffer);
  buffer = NULL;
  buffer_size = 0;
}
void MQTTbuffer::_freeBuffer()
{
  if (buffer_size!=0)
    free(buffer);
  buffer = NULL;
}
    

MQTTMessage::MQTTMessage                                      ( void )
{
  _init();
}
MQTTMessage::~MQTTMessage                                     ( void )
{
  _freeBuffer();
}
void      MQTTMessage::reset                                  ( void )
{
  _init();
}
void      MQTTMessage::setQoS                                 ( QoSs qos)
{
  buffer = _getBuffer(2);
  buffer[0] = (buffer[0] & 0xF9) | (((int)qos) << 1);
}
void      MQTTMessage::setDupFlag                             ( bool value)
{
  buffer = _getBuffer(2);
  buffer[0] = (buffer[0] & 0xFB) | (value ? 8 : 0);
}
void      MQTTMessage::setRetain                              ( bool value)
{
  buffer = _getBuffer(2);
  buffer[0] = (buffer[0] & 0xFE) | (value ? 1 : 0);
}
MQTTMessage::Type MQTTMessage::getType                        ( void )
{
  if (buffer_size > 0)
    return (Type)(buffer[0] & 0xF0);
  else
    return Type::Unknown;
}
String    MQTTMessage::getTypeString                          ( void )
{
  String  t = "";
  switch (getType())
  {
  case Unknown:
    t = String(F("Unknown")); break;
  case Connect:
    t = String(F("Connect")); break;
  case ConnAck:
    t = String(F("ConnAck")); break;
  case Publish:
    t = String(F("Publish")); break;
  case PubAck:
    t = String(F("PubAck")); break;
  case PubRec:
    t = String(F("PubRec")); break;
  case PubRel:
    t = String(F("PubRel")); break;
  case PubComp:
    t = String(F("PubComp")); break;
  case Subscribe:
    t = String(F("Subscribe")); break;
  case SubAck:
    t = String(F("SubAck")); break;
  case UnSubscribe:
    t = String(F("UnSubscribe")); break;
  case UnSuback:
    t = String(F("UnSuback")); break;
  case PingReq:
    t = String(F("PingReq")); break;
  case PingResp:
    t = String(F("PingResp")); break;
  case Disconnect:
    t = String(F("Disconnect")); break;
  case Reserved:
    t = String(F("Reserved")); break;
  }
  return t;
}
void      MQTTMessage::generateMessageLength                  ( void )
{
  uint32_t size = convertedMessageLength();
  //mqttdebug(buffer_size);
  //mqttdebug(size);
  if (buffer_size - 2 != size)
  {
    int ByteCount = mqttByteForLength(size);
    //mqttdebug(ByteCount);
    _getBuffer(buffer_size + ByteCount - 1, ByteCount - 1); // alloc memory and shift message by ByteCount-1 
    for (size_t i = 0; i < ByteCount; i++)
    {
      buffer[i + 1] = size >> ((ByteCount - i - 1) * 8);
    }

  }
  else
    buffer[1] = size;
}
uint32_t  MQTTMessage::convertedMessageLength                 ( void )
{
  uint8_t b = 0;
  uint32_t returnData = 0;
  uint32_t size = buffer_size - 2;
  for (size_t i = 0; i < 4; i++)
  {
    b = size & B01111111;
    if (size > 127)
    {
      size = size >> 7;
      b |= B10000000;
      returnData |= b;
      returnData = returnData << 8;
      //Serial.printf("convertedMessageLength: %02X\r\n", b);
    }
    else
    {
      returnData |= b;
      break;
    }
  }
  return returnData;

}
uint32_t  MQTTMessage::getMessageLength                       ( void )
{
  uint32_t len = 0;
  uint32_t b = 0;
  if (buffer_size > 1)
  {
    for (size_t i = 1; i < 5; i++)
    {
      b = buffer[i] & B01111111;
      len += b << (7 * (i - 1));
      if (buffer[i] < 128)
        break;
    }
  }
  return len;
}
void      MQTTMessage::messageIndexToStart                    ( void )
{
  currentIndex = mqttByteForLength(convertedMessageLength()) + 1;
}
MQTTMessage::QoSs MQTTMessage::getQoS                         ( void )
{
  return (QoSs)((buffer[0] >> 1) & B00000011);
}
bool      MQTTMessage::getDup                                 ( void )
{
  return (QoSs)((buffer[0] >> 3) & B00000001);
}
bool      MQTTMessage::getRetain                              ( void )
{
  return (QoSs)((buffer[0]) & B00000001);
}
uint8_t   MQTTMessage::getNextByte                            ( void )
{
  return ((uint8_t)buffer[currentIndex++]);
}
int       MQTTMessage::getNextInt                             ( void )
{
  //size_t nextItemSize = ((uint16_t)buffer[currentIndex++])<<8 | ((uint16_t)buffer[currentIndex++]);
  return ((uint16_t)buffer[currentIndex++]) << 8 | ((uint16_t)buffer[currentIndex++]);
}
uint16_t  MQTTMessage::getRemainingBytes                      ( uint8_t** bytes )
{
  size_t size = buffer[1] - currentIndex + 1 + mqttByteForLength(convertedMessageLength());
  *bytes = (uint8_t*)malloc(size);
  memcpy(*bytes, &buffer[currentIndex], size);
  currentIndex += size;
  return size;
}
String    MQTTMessage::bytesToString                          ( uint8_t* bytes, size_t size)
{
  char *newBytes = (char*)malloc(size + 1);
  memcpy(newBytes, bytes, size);
  newBytes [size] = 0;
  String returnValue = String(newBytes);
  free(newBytes);
  return returnValue;
}
String    MQTTMessage::getRemainingString                     ( void )
{
  size_t size = buffer[1] - currentIndex + 1 + mqttByteForLength(convertedMessageLength());
  //mqttdebug(buffer[1]);
  //mqttdebug(currentIndex);
  //mqttdebug(size);
  char* str = (char*)malloc(size + 1);
  memcpy(str, &buffer[currentIndex], size);
  str[size] = '\0';
  String s = String(str);
  free(str);
  currentIndex += size;
  return s;
}
String    MQTTMessage::getNextString                          ( void )
{
  size_t nextItemSize = ((uint16_t)buffer[currentIndex++]) << 8 | ((uint16_t)buffer[currentIndex++]);
  char* str = (char*)malloc(nextItemSize + 1);
  memcpy(str, &buffer[currentIndex], nextItemSize);
  str[nextItemSize] = '\0';
  String s = String(str);
  free(str);
  currentIndex += nextItemSize;
  return s;
}
bool      MQTTMessage::hasNext                                ( void )
{
  return currentIndex + 1 < buffer_size;
}
void      MQTTMessage::readMessage                            ( TCPClient& client)
{
  this->reset();
  //mqttsdebug("reading message");
  uint8_t* buffer = _getBuffer(2);
if (!client.connected())   mqttshow(client.connected());
//  mqttshow(client.available());
//  buffer[0] = client.read();
//if (!client.connected())   mqttshow(client.connected());
//  buffer[1] = client.read();
  client.readBytes(buffer, 2);
if (!client.connected())   mqttshow(client.connected());
  //_hexdump("Consecutive", (char*)buffer, buffer_size,1);
        // read bytes nedded to evaluate message length
  for (size_t i = 1; i < 4; i++)
  {
    if (buffer[i] > 128)
    {
      buffer = _getBuffer(buffer_size + 1);
      buffer[i + 1] = client.read();
if (!client.connected()) mqttshow(client.connected());
      //_hexdump("Consecutive", (char*)buffer, buffer_size,1);
    }
    else
      break;
  }
if (!client.connected())  mqttshow(client.connected());
  //_hexdump("Consecutive", (char*)buffer, buffer_size,1);
  currentIndex = buffer_size;
  //mqttdebug(currentIndex);
  uint32_t len = getMessageLength();
  //mqttdebug(len);
  buffer = _getBuffer(currentIndex + len);
if (!client.connected())  mqttshow(client.connected());
  size_t len1 = client.readBytes(buffer + currentIndex, len);
if (!client.connected()) mqttshow(client.connected());
if (!client.connected()) mqttshow(len);
if (!client.connected()) mqttshow(len1);
if (!client.connected()) mqttshow(client.connected());
  hexdump(("New Message from: " + client.remoteIP().toString() + ", ").c_str());

}
void      MQTTMessage::sendMessage                            ( TCPClient& client)
{
  generateMessageLength();
  if (client && client.connected())
  {
    //mqttdebug(client.availableForWrite());
    int size = client.write(this->buffer, this->buffer_size);
    //client.flush();
    //delay(50);
    //mqttdebug(size);
    //hexdump("Sent");
  }
  if (client && !client.connected())
  {
    mqttsdebug("Client NOT Connected!");
    client.stop();
  }
  else if (!client)
    mqttsdebug("Client NOT Alive!");
}
void      MQTTMessage::add                                    ( uint8_t value)
{
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+1);
  tmp[oldSize] = value;
}
void      MQTTMessage::add                                    ( uint16_t value)
{
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+2);
  tmp[oldSize] = value>>8;
  tmp[oldSize+1] = value & 0xFF;
}
void      MQTTMessage::add                                    ( String value)
{
  uint16_t size = value.length();
  add(size);
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+value.length());
  memcpy(&tmp[oldSize] , value.c_str(), value.length());
}
void      MQTTMessage::addSubscribeTopic                      ( String Topic, QoSs QoS)
{
  this->add(Topic);
  this->add((uint8_t)QoS);
}
void      MQTTMessage::addLast                                ( String value)
{
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+value.length());
  memcpy(&tmp[oldSize] , value.c_str(), value.length());
}
void      MQTTMessage::addLast                                ( uint8_t *value, int valueSize)
{
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+valueSize);
  memcpy(&tmp[oldSize] , value, valueSize);
}
void      MQTTMessage::create                                 ( Type type, QoSs qos, bool retain , bool dupFlag)
{
  this->reset();
  buffer = _getBuffer(2);
  buffer[0]=(uint8_t)type | (dupFlag? 0x08 : 0) | (((int)qos)<<1) | (retain? 1:0) ;
  buffer[1]=0; // message size
  //state=Create;
}
void      MQTTMessage::operator=                              ( MQTTMessage &msg)
{
  this->currentIndex = 0;
  this->buffer_size =  msg.buffer_size;
  this->buffer = (uint8_t*)malloc(msg.buffer_size);

}
void      MQTTMessage::createPublishMessage                   ( PublishMessage &publishMessage)
{
  createPublishMessage(publishMessage.name, publishMessage.value, publishMessage.QoS, publishMessage.retain, publishMessage.dupFlag, publishMessage.messageID);
}
void      MQTTMessage::createPublishMessage                   ( String topic, String payload, QoSs qos, bool retain, bool dupFlag, uint16_t messageID)
{
  createPublishMessage(topic, (uint8_t*)payload.c_str(), payload.length(), qos, retain, dupFlag, messageID);
}
void      MQTTMessage::createPublishMessage                   ( String topic, uint8_t *payload, int payloadSize, QoSs qos, bool retain, bool dupFlag, uint16_t messageID)
{
  mqttsdebug("createPublishMessage"); 
  create(Publish, qos, retain, dupFlag);
//mqttdebug(buffer_size);
  //add( (uint16_t) topic.length());
  add(topic);
//mqttdebug(buffer_size);
  if(qos != QoSs::QoS0)
    add(messageID);
//mqttdebug(buffer_size);
  addLast(payload, payloadSize);
//mqttdebug(buffer_size);
      
  //buffer[1] = 2 + topic.length() + payload + payloadSize + (qos != QoSs::QoS0)? 2 : 0;
  //_getBuffer( buffer_size + buffer[1] );
  //buffer[2] = (topic.length()>>8) 
}
void MQTTMessage::createSubscribeMessage             ( void )
{
  this->_init();
  this->create(Type::Subscribe, QoSs::QoS1);
  uint16_t messageID = 1;
  this->add(messageID);
}
MQTTMessage::PublishMessage  MQTTMessage::getAsPublishMessage ( void )
{
  MQTTMessage::PublishMessage returnValue;
//mqttsdebug("Start - getAsPublishMessage");
  messageIndexToStart();
  if (getType() == Publish)
  {
    returnValue.retain      = getRetain();
//mqttsdebug("getAsPublishMessage");
    returnValue.QoS         = getQoS();
    returnValue.dupFlag     = getDup();
//mqttdebug(currentIndex);
    returnValue.name        = getNextString();
//mqttdebug(currentIndex);
    returnValue.messageID   = (returnValue.QoS==MQTTMessage::QoSs::QoS1 || returnValue.QoS==MQTTMessage::QoSs::QoS2)? getNextInt() : 0;
//mqttdebug(currentIndex);
    returnValue.value       = getRemainingString();
//mqttdebug(currentIndex);
//mqttdebug(returnValue.name);
//mqttdebug(returnValue.value);
  }
//mqttsdebug("getAsPublishMessage");
  return returnValue;
}
void      MQTTMessage::_init                                  ( void )
{
  MQTTbuffer::_init();
  state = undefined;
}
void      MQTTMessage::addBasic                               ( void *data, int size)
{
  size_t oldSize = buffer_size;
  uint8_t * tmp = _getBuffer(buffer_size+size);
  tmp[1] = buffer_size-2;
  memcpy(&tmp[oldSize], data, size);
}
void      MQTTMessage::hexdump                                ( const char* prefix, MQTTClient* client) 
{
#ifdef MQTT_DEBUG
  String  s =  String(prefix) ;
  String  t = getTypeString();
  s= "[ " + t + " ] " + s;
  if (client!=NULL)
  {
    s +=  " to " + client->clientID +" - " + client->client->remoteIP().toString() 
        + "( connected: " +  String(client->client->connected()) + " )" ;
  }
//mqttsdebug("_hexdump");

  _hexdump(s.c_str(), (const char*)buffer, buffer_size, (int)state);
#endif
}
#if TEST_MQTTMESSAGE_MANAGER 
void      MQTTMessage::readMessage                            ( void )
{
  if (this->client!=NULL)
  {
    this->readMessage(*this->client->client);
  }else
    state = State::Error;
}
void      MQTTMessage::sendMessage                            ( void )
{
  if (this->client!=NULL)
  {
    this->sendMessage(*this->client->client);
  }else
    state = State::Error;
}
void      MQTTMessage::responseConnAck                        ( uint16_t flag)
{
  this->create(MQTTMessage::ConnAck, MQTTMessage::QoS0);
  this->add((uint16_t)flag);
  this->sendMessage(*client->client);
}

MQTTMessage::State MQTTMessage::handle                        ( void )
{
  /*
  if (client==NULL)
    this->state = Error;
  else
  {
    TCPClient *tpcClient = this->client->client;
    if(*tpcClient && tpcClient->connected() && tpcClient->available()>1)
    {
      #ifdef MQTT_DEBUG
        Serial.println();
        //mqttsdebug("Hanlde Client: " + client->remoteIP().toString());
        //Serial.println("MQTT: Hanlde Client");
        //Serial.print("MQTT: Hanlde Client addr - "); Serial.println((uint32_t)client, HEX); 
        //Serial.print("MQTT: Hanlde Client connected - "); Serial.println(client->connected()); 
        //Serial.print("MQTT: Hanlde Client available - "); Serial.println(client->available()); 
      #endif
mqttdebug(AllTopics.size());
//if (!tpcClient->connected())
//  mqttshow(client->connected());
      this->readMessage();
//if (!tpcClient->connected())
//  mqttshow(tpcClient->connected());
      //message.hexdump("DUMP");
      MQTTMessage::Type type      = this->getType();
      uint32_t MessageTotalLength = this->getMessageLength();
      uint8_t           flags         = 0 ;
      bool              retain        = 0 ;
      //MQTTMessage::QoSs QoS           = MQTTMessage::QoS0 ;
      bool              usernameFlag  = 0 ;
      bool              passwordFlag  = 0 ;
      bool              willFlag      = 0 ;
      bool              cleanStart    = 0 ;
      bool              dupFlag       = 0 ;
      switch (type)
      {
        case MQTTMessage::Type::Connect:
          {
            String            protocol      = this->getNextString();
            uint8_t           version       = this->getNextByte();
                              flags         = this->getNextByte();
                              retain        = (flags>>5) & B00000001;
                              usernameFlag  = (bool)((flags>>7) & B00000001);
                              passwordFlag  = (bool)((flags>>6) & B00000001);
                              QoS           = (QoSs)((flags>>4) & B00000011);
                              willFlag      = (bool)((flags>>2) & B00000001);
                              cleanStart    = (bool)((flags>>1) & B00000001);
            uint32_t          aliveTime     = ((uint32_t)this->getNextInt()); // in sec * 1000; //to millis
            String            username      = "";
            String            password      = "";
            if (this->hasNext())
              clientID = this->getNextString();
            if (this->hasNext())
              username = this->getNextString();
            if (this->hasNext())
              password = this->getNextString();

            #ifdef MQTT_DEBUG
              //mqttdebug(type                );
              //mqttdebug(MessageTotalLength  );
              Serial.println();
              Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
              Serial.printf (   "|     %4s     | %8s | %7s | %3s | %6s | %4s | %5s | %5s |\r\n", 
                                "Type","protocol", "version", "QoS","Retain", "Will", "Clean", "Alive");
              Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
              Serial.printf (   "| %12s |   %4s   |    %1d    |  %1d  |    %1d   |   %1d  |   %1d   | %5d |\r\n",
                             this->getTypeString(), protocol, version, QoS, retain, willFlag, cleanStart, aliveTime);
              Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
              Serial.printf (   "|    Client ID | %-56s |\r\n", clientID.c_str());
              Serial.println( F("+--------------+----------------------------------------------------------+"));
              Serial.printf (   "|     Username | %-56s |\r\n", username.c_str());
              Serial.println( F("+--------------+----------------------------------------------------------+"));
              Serial.printf (   "|     Password | %-56s |\r\n", password.c_str());
              Serial.println( F("+--------------+----------------------------------------------------------+"));
              //mqttdebug(protocol            );
              //mqttdebug(version             );
              //mqttdebug(retain              );
              //mqttdebug(QoS                 );
              //mqttdebug(willFlag            );
              //mqttdebug(cleanStart          );
              //mqttdebug(aliveTime           );
              //mqttdebug(clientID            );
              //mqttdebug(username            );
              //mqttdebug(password            );
            #endif

            this->reset();
            bool isValid = version>=3;
            bool isVersion4 = version>3;
            //delay(1000);
            if(!isValid )
            {
              this->responseConnAck(MQTTMessage::ConnetionStatus::UnacceptableProtocolVersion);
              this->state = State::Error;
            }
            isValid =  isValid && (( username == this->broker->username  && username == this->broker->password) ||
                                  ( this->broker->username.length()==0 && this->broker->password.length()==0));

            if (isValid)
            {
              this->responseConnAck (MQTTMessage::ConnetionStatus::Accepted | isVersion4? MQTTMessage::ConnetionStatusExtendex::SesionPresent : MQTTMessage::ConnetionStatusExtendex::none);
              this->state = State::Complete;
            }
            else 
            {
              this->responseConnAck (MQTTMessage::ConnetionStatus::IdentifierRejected);
              this->state = State::Error;
            }

          }
          break;
        case MQTTMessage::Type::ConnAck:
          this->isConnected = client->connected();
          this->state = State::Complete;
          break;
        case MQTTMessage::Type::Subscribe:
          {
            MQTTMessage::QoSs QoS = message.getQoS();
            uint16_t MessageId = message.getNextInt();
            String Topic1 = message.getNextString();
            MQTTMessage::QoSs QoS_Topic1 = (MQTTMessage::QoSs )(message.getNextByte() & B00000011);
            MQTTMessage::Topic t = {Topic1, "",false,QoS_Topic1};
            topics.push_back(t);

            while(message.hasNext())
            {
              String otherTopic = message.getNextString();
              MQTTMessage::QoSs QoS_otherTopic = (MQTTMessage::QoSs )(message.getNextByte() & B00000011);
              MQTTMessage::Topic t1 = {otherTopic, "",false,QoS_otherTopic};
              topics.push_back(t1);
            }
            #ifdef MQTT_DEBUG
              Serial.println();
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "|     %4s     | %9s | %3s | %6s | %4s | %5s |\r\n", 
                                "Type", "messageID", "QoS","Retain", "Will", "Clean");
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "| %12s |   %6d  |  %1d  |    %1d   |   %1d  |   %1d   |\r\n",
                              message.getTypeString(), MessageId, QoS, retain, willFlag, cleanStart);
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "|        Topic | %-39s |\r\n", Topic1.c_str());
              Serial.println( F("+--------------+-----------------------------------------+"));
            #endif
            if (QoS == MQTTMessage::QoS2)
            {
              MQTTMessage response;
              response.create(MQTTMessage::Type::SubAck, MQTTMessage::QoS0);
              response.add(MessageId);
              for (size_t i = 0; i < topics.size(); i++)
              {
                response.add((uint8_t)MQTTMessage::QoS1);
              }
              //mqttdebug(QoS);
              //mqttdebug(MessageId);
              //mqttdebug(Topic1);
              //mqttdebug(QoS_Topic1);
              response.hexdump("Sent", this);
              response.sendMessage(*client);
            }
            if(this->broker)
              this->broker->sendSubscribedMessages(Topic1, this);
          }
          break;
        case MQTTMessage::Type::SubAck:
          this->isConnected = client->connected();
          this->state = State::Complete;
          break;
        case MQTTMessage::Type::PingReq:
          {
            MQTTMessage response;
            response.create(MQTTMessage::Type::PingResp, MQTTMessage::QoS0);
            response.hexdump("Sent", this);
            response.sendMessage(*client);
          }
          break;
        case MQTTMessage::Type::Publish:
          {
            retain              = message.getRetain();
            QoS                 = message.getQoS();
            dupFlag             = message.getDup();
            String    topic     = message.getNextString();
            uint16_t   messageID = QoS!=MQTTMessage::QoSs::QoS0? message.getNextInt() : 0;
            uint8_t * payloadBytes = NULL;
            uint16_t payloadSize   = message.getRemainingBytes(&payloadBytes);
            String payload  = message.bytesToString(payloadBytes, payloadSize);

            #ifdef MQTT_DEBUG
              Serial.println();
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "|     %4s     | %9s | %3s | %6s | %4s | %5s |\r\n", 
                                "Type", "messageID", "QoS","Retain", "Will", "Clean");
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "| %12s |   %6d  |  %1d  |    %1d   |   %1d  |   %1d   |\r\n",
                              message.getTypeString(), messageID, QoS, retain, willFlag, cleanStart);
              Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
              Serial.printf (   "|        Topic | %-39s |\r\n", topic.c_str());
              Serial.println( F("+--------------+-----------------------------------------+"));
              Serial.printf (   "|      Payload | %-39s |\r\n", payload.c_str());
              Serial.println( F("+--------------+-----------------------------------------+"));
              //Serial.printf (   "|    Connected | %d                                      |\r\n", client->connected());
              //Serial.println( F("+--------------+-----------------------------------------+"));
            #endif
            //mqttdebug(message.currentIndex);
            //mqttdebug(QoS);
            //mqttdebug(retain);
            //mqttdebug(dupFlag);
            //mqttdebug(topic);
            //mqttdebug(messageID);
            //mqttdebug(payload);



            if (QoS==MQTTMessage::QoS1 || QoS==MQTTMessage::QoS2)
            {
              MQTTMessage response;
              response.create( QoS==MQTTMessage::QoS1? MQTTMessage::Type::PubAck : MQTTMessage::Type::PubRec, MQTTMessage::QoSs::QoS1);
  mqttshow(messageID);
              response.add(messageID);
              response.hexdump("Sent", this);
              response.sendMessage(*client);
              //int index = this->broker->IndexOf(topic);
            }
            if (retain)
            {
              mqttsdebug("Added to the topic list!");
              AllTopics.add(topic, payload, retain, QoS);
            }
            mqttsdebug("distribuitePublish");
            if (callback)
              callback(topic.c_str(), payloadBytes, payloadSize);
            if(this->broker)
              this->broker->distribuitePublish(topic,message);
          }
          break;
        case MQTTMessage::Type::Disconnect:
          this->client->stop();
          break;
        case MQTTMessage::Type::PubRec:
          {
            retain              = message.getRetain();
            QoS                 = message.getQoS();
            dupFlag             = message.getDup();
            uint16_t  NewMessageID = message.getNextInt();
            if (usedMessageIDs.exists(NewMessageID))
            {
              MQTTMessage msg;
              msg.create(MQTTMessage::Type::PubRel,MQTTMessage::QoS0, false, false);
              msg.add(NewMessageID);
              msg.sendMessage(*client);
              msg.hexdump("Sent" , this);
              usedMessageIDs.remove(NewMessageID);
            }else
            {
              Serial.printf("Message Size %d:\r\n", this->usedMessageIDs.size());
              for (size_t i = 0; i < this->usedMessageIDs.size(); i++)
                 Serial.printf("MessageId %d\r\n", this->usedMessageIDs[i]);
            
              Serial.println("The message id " + String (NewMessageID) + " doesn't exist!");
              Serial.println("Nettwork Error!\r\n\tClosing Client" + client->remoteIP().toString());
              client->stop();
            }
          }
          break;
        case MQTTMessage::Type::PubComp:
          break;

        case MQTTMessage::Type::PubRel:
          {
            retain              = message.getRetain();
            QoS                 = message.getQoS();
            dupFlag             = message.getDup();
            uint16_t  messageID = message.getNextInt();
            //if (NewMessageID==messageID)
            //{
              MQTTMessage msg;
              msg.create(MQTTMessage::Type::PubComp,MQTTMessage::QoS0, false, false);
              msg.add(messageID);
              msg.sendMessage(*client);
              msg.hexdump("Sent" , this);
            //}else
            //{
            //  Serial.println(String (messageID) + " <> " + String (NewMessageID));
            //  Serial.println("Network Error!\r\n\tClosing Client " + client->remoteIP().toString());
            //  client->stop();
            //}
          }
          break;
        case MQTTMessage::Type::UnSubscribe:
          mqttsdebug("UnSubscribe to manage");
          break;
      default:
        break;
      }
    }

  }
  */
  return this->state;
}
//typedef MQTTMessage::Topic Topic;
#endif


/*
class TopicList : std::vector<Topic>
{
public:
  void publish(String topicName,  TCPClient *client)
  {
    for (size_t i = 0; i < size(); i++)
    {
      if (topicName == this->at(i).name || topicName=="#")
        MQTTMessage
    }
    
  }
};
*/

int TopicList::indexOf  ( String topic)
{
  int returnValue = -1;
  for (size_t i = 0; i < this->size(); i++)
  {
    if (topic==(*this)[i].name)
      return i;
  }
  return returnValue;
}
void TopicList::add     ( String topic, String payload, bool retain, MQTTMessage::QoSs QoS)
{ 
  int index = indexOf(topic);
  if (index<0)
    this->push_back(MQTTMessage::Topic(topic, payload, retain, QoS));
  else
  {
    this->at(index).QoS = QoS;
    this->at(index).retain = retain;
    this->at(index).value = payload;
  }
}



MQTTClient::MQTTClient            ( MQTTBroker *broker, TCPClient &client)
{
  this->broker = broker;
  this->client =  new TCPClient(client);
  this->IP = this->client->remoteIP();
  alive_ms = millis()+5000; // 5 second before first ping
}
MQTTClient::MQTTClient            ( void )
{
  this->broker = NULL;
  this->client = new TCPClient();
}
MQTTClient::~MQTTClient           ( void )
{
  if(*client && client->connected())
  {
    client->stop();
  }
  delete client;
}
void MQTTClient::handle           ( void )
{
  if(*client && client->connected() && millis()>alive_ms)
  {
    uint16_t pingreq = MQTTMessage::Type::PingReq;
		client->write((const char*)(&pingreq), 2);
    alive_ms = millis() + 10000; // seconds
    #ifdef MQTT_DEBUG
      Serial.print("p");
    #endif    
  }
  evaluateMessage();
}
void MQTTClient::responseConnAck  ( uint16_t flag)
{
  MQTTMessage response;
  response.create(MQTTMessage::ConnAck, MQTTMessage::QoS0);
  response.add((uint16_t)flag);
  //response.hexdump("Sent", this);
  response.sendMessage(*client);
}
void MQTTClient::pingReq          ( TCPClient *client) 
{
  MQTTMessage msg;
  msg.create(MQTTMessage::Type::PingReq, MQTTMessage::QoS0, false, false);
  msg.sendMessage(*client);
  //msg.hexdump("Sent", this);
}
bool MQTTClient::connect          ( String clientID, String broker, String username, String password, uint16_t port)
{
  client->connect(broker.c_str(), port);
  this->messageID = 0;
  this->outMessageIDs.clear();
  this->inMessageIDs.clear();
  if (client->connected())
  {
    mqttsdebug("MQTT Broker Connected!");
    MQTTMessage msg;
    delay(500);
    msg.create(MQTTMessage::Type::Connect, MQTTMessage::QoS1 );
    msg.add("MQTT");
    msg.add((uint8_t) 4);
    msg.add((uint8_t) (2 | (username!="" ? 0x80 : 0) | (password!="" ? 0x40 : 0))); // clean start
    msg.add((uint16_t) 30); // 15 sec TimeOut
    msg.add(clientID);
    if (username)
      msg.add(username); 
    if (password)
      msg.add(password); 
//mqttdebug(client->connected());
    msg.sendMessage(*client);
    msg.hexdump("Sent", this);
    uint32_t ms = millis();
    while (!isConnected && millis()-ms<5000) // 5 sec time out
    {
      this->evaluateMessage();
    }
    if (!isConnected)
    {
      mqttsdebug("No Answer From the Broker");
      client->stop();
      return false;
    }
    this->IP = this->client->remoteIP();
    if (this->onConnectCallback!=NULL)
    {
      mqttsdebug("onConnectCallback Calling");
      this->onConnectCallback(this);
    }
    return true;

  }
  return false;
}
bool MQTTClient::connected        ( void )
{
  if (!isConnected ||  !client->connected() )
  {
    mqttshow(isConnected );
    mqttshow(client->connected() );
  }
  return isConnected & client->connected();
}
void MQTTClient::publish          ( String topicName,  MQTTMessage &message)
{
  if ((*client) && client->connected())
  {
    for (size_t i = 0; i < subscriptionTopics.size(); i++)
    {
      //if (topicName == topics[i].name || topics[i].name=="#")
      if (topicName == subscriptionTopics[i] || subscriptionTopics[i] =="#")
      {
//mqttsdebug("publish");
        MQTTMessage::PublishMessage publishMessage = message.getAsPublishMessage();
        publishMessage.QoS = this->QoS;
        if (this->QoS>0 && !this->outMessageIDs.exists(publishMessage.messageID))
          this->outMessageIDs.push_back(publishMessage.messageID);
          
          
        MQTTMessage msg; 
        msg.hexdump("building", this);
        msg.createPublishMessage(publishMessage);
        msg.sendMessage(*client);
        msg.hexdump("Distribuite Publish", this);
      }
    }
  }
}
void MQTTClient::publish          ( String topicName,  TCPClient *client, MQTTMessage *message)
{
  //mqttsdebug("publish: " + topicName);
  if ((*client) && client->connected())
  {
    for (size_t i = 0; i < AllTopics.size(); i++)
    {
      if (topicName == AllTopics[i].name || topicName=="#")
      {
        MQTTMessage msg;
        MQTTMessage *m = &msg;
        if (message!=NULL)
          m = message;
        else
        {
          m->create(MQTTMessage::Type::Publish, MQTTMessage::QoS1, true, true);
          m->add(AllTopics[i].name);
          m->add((uint16_t)1); // message id
          m->addLast(AllTopics[i].value);
        }
        m->sendMessage(*client);
        m->hexdump("Sent", this);

      }else
        mqttsdebug("Not Found: " + topicName);
    }
  }else
    mqttsdebug("Connection Closed: " + this->clientID);
}
void MQTTClient::publish          ( String topicName, String payload, bool retain, MQTTMessage::QoSs QoS, uint16_t messageID )
{
  if (messageID==0)
    this->messageID++;
  else
    this->messageID = messageID;
  if (!outMessageIDs.exists(this->messageID))
  {
    mqttshow(this->messageID);
    outMessageIDs.push_back(this->messageID);
  }

  MQTTMessage  msg;
  msg.createPublishMessage(topicName, payload, QoS, retain, true, this->messageID);
  msg.sendMessage(*client);
  msg.hexdump("Sent", this);
}
void MQTTClient::subscribe        ( String topicName, MQTTMessage::QoSs QoS)
{
  MQTTMessage  msg;
  msg.create(MQTTMessage::Type::Subscribe, MQTTMessage::QoS1, true);
  if (messageID==0)
    messageID++;
  msg.add(messageID);
  msg.add(topicName);
  msg.add((uint8_t)QoS);

  msg.sendMessage(*client);
  msg.hexdump("Sent", this);
}
void MQTTClient::subscribe        ( std::vector<String> &topicNames, MQTTMessage::QoSs QoS)
{
  MQTTMessage  msg;
  msg.create(MQTTMessage::Type::Subscribe, MQTTMessage::QoS1, true);
  if (messageID==0)
    messageID++;
  msg.add(messageID);
  for (size_t i = 0; i < topicNames.size(); i++)
  {
    msg.add(topicNames[i]);
    msg.add((uint8_t)QoS);
  }
  msg.sendMessage(*client);
  msg.hexdump("Sent", this);
}
void MQTTClient::subscribe        ( MQTTSubscribeMessage &Message )
{
  Message.sendMessage(*client);
  Message.hexdump("Sent", this);
}
void MQTTClient::evaluateMessage  ( void )
{
  if(*client && client->connected() && client->available()>1)
  {

    #ifdef MQTT_DEBUG
      Serial.println();
      //mqttsdebug("Hanlde Client: " + client->remoteIP().toString());
      //Serial.println("MQTT: Hanlde Client");
      //Serial.print("MQTT: Hanlde Client addr - "); Serial.println((uint32_t)client, HEX); 
      //Serial.print("MQTT: Hanlde Client connected - "); Serial.println(client->connected()); 
      //Serial.print("MQTT: Hanlde Client available - "); Serial.println(client->available()); 
    #endif
    mqttdebug(AllTopics.size());
if (!client->connected())
  mqttshow(client->connected());
    message.readMessage(*client);
if (!client->connected())
  mqttshow(client->connected());
    //message.hexdump("DUMP");
    MQTTMessage::Type type      = message.getType();
    uint32_t MessageTotalLength = message.getMessageLength();
    uint8_t           flags         = 0 ;
    bool              retain        = 0 ;
    //MQTTMessage::QoSs QoS           = MQTTMessage::QoS0 ;
    bool              usernameFlag  = 0 ;
    bool              passwordFlag  = 0 ;
    bool              willFlag      = 0 ;
    bool              cleanStart    = 0 ;
    bool              dupFlag       = 0 ;
    switch (type)
    {
      case MQTTMessage::Type::Connect:
        {
          String            protocol      = message.getNextString();
          uint8_t           version       = message.getNextByte();
                            flags         = message.getNextByte();
                            retain        = (flags>>5) & B00000001;
                            usernameFlag  = (MQTTMessage::QoSs)((flags>>7) & B00000001);
                            passwordFlag  = (MQTTMessage::QoSs)((flags>>6) & B00000001);
                            QoS           = (MQTTMessage::QoSs)((flags>>4) & B00000011);
                            willFlag      = (MQTTMessage::QoSs)((flags>>2) & B00000001);
                            cleanStart    = (MQTTMessage::QoSs)((flags>>1) & B00000001);
          uint32_t          aliveTime     = ((uint32_t)message.getNextInt()); // in sec * 1000; //to millis
          String            username      = "";
          String            password      = "";
          if (message.hasNext())
            clientID = message.getNextString();
          if (message.hasNext())
            username = message.getNextString();
          if (message.hasNext())
            password = message.getNextString();

          #ifdef MQTT_DEBUG
            //mqttdebug(type                );
            //mqttdebug(MessageTotalLength  );
            Serial.println();
            Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
            Serial.printf (   "|     %4s     | %8s | %7s | %3s | %6s | %4s | %5s | %5s |\r\n", 
                              "Type","protocol", "version", "QoS","Retain", "Will", "Clean", "Alive");
            Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
            Serial.printf (   "| %12s |   %4s   |    %1d    |  %1d  |    %1d   |   %1d  |   %1d   | %5d |\r\n",
                           message.getTypeString(), protocol, version, QoS, retain, willFlag, cleanStart, aliveTime);
            Serial.println( F("+--------------+----------+---------+-----+--------+------+-------+-------+"));
            Serial.printf (   "|    Client ID | %-56s |\r\n", clientID.c_str());
            Serial.println( F("+--------------+----------------------------------------------------------+"));
            Serial.printf (   "|     Username | %-56s |\r\n", username.c_str());
            Serial.println( F("+--------------+----------------------------------------------------------+"));
            Serial.printf (   "|     Password | %-56s |\r\n", password.c_str());
            Serial.println( F("+--------------+----------------------------------------------------------+"));
            //mqttdebug(protocol            );
            //mqttdebug(version             );
            //mqttdebug(retain              );
            //mqttdebug(QoS                 );
            //mqttdebug(willFlag            );
            //mqttdebug(cleanStart          );
            //mqttdebug(aliveTime           );
            //mqttdebug(clientID            );
            //mqttdebug(username            );
            //mqttdebug(password            );
          #endif

          message.reset();
          bool isValid = version>=3;
          bool isVersion4 = version>3;
          //delay(1000);
          if(!isValid )
          {
            responseConnAck (MQTTMessage::ConnetionStatus::UnacceptableProtocolVersion);
            return;
          }

          isValid =  isValid && (( username == this->broker->username  && username == this->broker->password) ||
                                ( this->broker->username.length()==0 && this->broker->password.length()==0));

          if (isValid)
            responseConnAck (MQTTMessage::ConnetionStatus::Accepted | isVersion4? MQTTMessage::ConnetionStatusExtendex::SesionPresent : MQTTMessage::ConnetionStatusExtendex::none);
          else
          {
            responseConnAck (MQTTMessage::ConnetionStatus::IdentifierRejected);
            return ;
          }
          if (this->broker !=NULL && this->broker->newClientCallback!=NULL)
          {
            this->broker->newClientCallback(this, clientID.c_str());
          }

          //delay(1000);
          //pingReq(client);
        }
        break;
      case MQTTMessage::Type::ConnAck:
        this->isConnected = client->connected();
        break;
      case MQTTMessage::Type::Subscribe:
        {
          MQTTMessage::QoSs QoS = message.getQoS();
          uint16_t MessageId = message.getNextInt();
          String Topic1 = message.getNextString();
          MQTTMessage::QoSs QoS_Topic1 = (MQTTMessage::QoSs )(message.getNextByte() & B00000011);
          //MQTTMessage::Topic t = {Topic1, "",false,QoS_Topic1};
          if (!subscriptionTopics.exists(Topic1))
            subscriptionTopics.push_back(Topic1);

          while(message.hasNext())
          {
            String otherTopic = message.getNextString();
            MQTTMessage::QoSs QoS_otherTopic = (MQTTMessage::QoSs )(message.getNextByte() & B00000011);
            //MQTTMessage::Topic t1 = {otherTopic, "",false,QoS_otherTopic};
            if (!subscriptionTopics.exists(otherTopic))
              subscriptionTopics.push_back(otherTopic);
          }
          #ifdef MQTT_DEBUG
            Serial.println();
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|     %4s     | %9s | %3s | %6s | %4s | %5s |\r\n", 
                              "Type", "messageID", "QoS","Retain", "Will", "Clean");
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "| %12s |   %6d  |  %1d  |    %1d   |   %1d  |   %1d   |\r\n",
                            message.getTypeString(), MessageId, QoS, retain, willFlag, cleanStart);
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|        Topic | %-39s |\r\n", Topic1.c_str());
            Serial.println( F("+--------------+-----------------------------------------+"));
          #endif
          if (QoS == MQTTMessage::QoS1)
          {
            MQTTMessage response;
            response.create(MQTTMessage::Type::SubAck, MQTTMessage::QoS0);
            response.add(MessageId);
            for (size_t i = 0; i < subscriptionTopics.size(); i++)
            {
              response.add((uint8_t)MQTTMessage::QoS1);
            }
            //mqttdebug(QoS);
            //mqttdebug(MessageId);
            //mqttdebug(Topic1);
            //mqttdebug(QoS_Topic1);
            response.hexdump("Sent", this);
            response.sendMessage(*client);
          }
          if(this->broker)
          {
            this->broker->sendSubscribedMessages(Topic1, this);
            if (this->broker->subscribeCallback!=NULL)
              this->broker->subscribeCallback(this, subscriptionTopics);
          }
        }
        break;
      case MQTTMessage::Type::PingReq:
        {
          MQTTMessage response;
          response.create(MQTTMessage::Type::PingResp, MQTTMessage::QoS0);
          response.hexdump("Sent", this);
          response.sendMessage(*client);
        }
        break;
      case MQTTMessage::Type::Publish:
        {
          retain              = message.getRetain();
          QoS                 = message.getQoS();
          dupFlag             = message.getDup();
          String    topic     = message.getNextString();
          uint16_t   messageID = QoS!=MQTTMessage::QoSs::QoS0? message.getNextInt() : 0;
          uint8_t * payloadBytes = NULL;
          uint16_t payloadSize   = message.getRemainingBytes(&payloadBytes);
          String payload  = message.bytesToString(payloadBytes, payloadSize);

          #ifdef MQTT_DEBUG
            Serial.println();
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|     %4s     | %9s | %3s | %6s | %4s | %5s |\r\n", 
                              "Type", "messageID", "QoS","Retain", "Will", "Clean");
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "| %12s |   %6d  |  %1d  |    %1d   |   %1d  |   %1d   |\r\n",
                            message.getTypeString(), messageID, QoS, retain, willFlag, cleanStart);
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|        Topic | %-39s |\r\n", topic.c_str());
            Serial.println( F("+--------------+-----------------------------------------+"));
            Serial.printf (   "|      Payload | %-39s |\r\n", payload.c_str());
            Serial.println( F("+--------------+-----------------------------------------+"));
            //Serial.printf (   "|    Connected | %d                                      |\r\n", client->connected());
            //Serial.println( F("+--------------+-----------------------------------------+"));
          #endif
          //mqttdebug(message.currentIndex);
          //mqttdebug(QoS);
          //mqttdebug(retain);
          //mqttdebug(dupFlag);
          //mqttdebug(topic);
          //mqttdebug(messageID);
          //mqttdebug(payload);



          if (QoS==MQTTMessage::QoS1 || QoS==MQTTMessage::QoS2)
          {
            MQTTMessage response;
            response.create( QoS==MQTTMessage::QoS1? MQTTMessage::Type::PubAck : MQTTMessage::Type::PubRec, MQTTMessage::QoSs::QoS1);
mqttshow(messageID);
            response.add(messageID);
            response.hexdump("Sent", this);
            response.sendMessage(*client);
            //int index = this->broker->IndexOf(topic);
          }
          if (retain)
          {
mqttsdebug("Added to the topic list!");
            AllTopics.add(topic, payload, retain, QoS);
          }
mqttsdebug("distribuitePublish");
          if (onMessageCallback)
            onMessageCallback(this, topic.c_str(), payloadBytes, payloadSize);
          if(this->broker)
          {
            this->broker->distribuitePublish(topic,message);
            MQTTMessage::Topic t(topic, payload, retain, QoS);
            if (this->broker->publishCallback!=NULL)
              this->broker->publishCallback(this, t);
          }
        }
        break;
      case MQTTMessage::Type::Disconnect:
        this->client->stop();
        break;
      case MQTTMessage::Type::PubRec: // received only if sent publish with QoS2 it need to answer with a PubRel
        {
          retain              = message.getRetain();
          QoS                 = message.getQoS();
          dupFlag             = message.getDup();
          uint16_t  NewMessageID = message.getNextInt();
          if (outMessageIDs.exists(NewMessageID))
          {
            MQTTMessage msg;
            msg.create(MQTTMessage::Type::PubRel,MQTTMessage::QoS0, false, false);
            msg.add(NewMessageID);
            msg.sendMessage(*client);
            msg.hexdump("Sent" , this);
          }else
          {
            Serial.printf("%d - Out Message Size %d:\r\n", __LINE__, this->outMessageIDs.size());
            for (size_t i = 0; i < this->outMessageIDs.size(); i++)
               Serial.printf("%d - Out MessageId %d\r\n", __LINE__, this->outMessageIDs[i]);
            
            Serial.println("The Out message id " + String (NewMessageID) + " doesn't exist!");
            Serial.println("Nettwork Error!\r\n\tClosing Client" + client->remoteIP().toString());
            client->stop();
          }
        }
        break;
      case MQTTMessage::Type::PubAck: // received only if sent publish with QoS1
        {
          uint16_t  messageID = message.getNextInt();
          mqttshow(String("PubAck Remove messageID= ") + String(messageID));
          outMessageIDs.remove(messageID);
        }
        break;
      case MQTTMessage::Type::PubRel: // received only if got publish with QoS2 and Answered width a PubRec it need to answer with a PupComp
        {
          retain              = message.getRetain();
          QoS                 = message.getQoS();
          dupFlag             = message.getDup();
          uint16_t  messageID = message.getNextInt();
          //if (NewMessageID==messageID)
          //{
            MQTTMessage msg;
            msg.create(MQTTMessage::Type::PubComp,MQTTMessage::QoS0, false, false);
            msg.add(messageID);
            msg.sendMessage(*client);
            msg.hexdump("Sent" , this);
            mqttshow(String("PubRel Remove messageID= ") + String(messageID));
            inMessageIDs.remove(messageID);
          //}else
          //{
          //  Serial.println(String (messageID) + " <> " + String (NewMessageID));
          //  Serial.println("Network Error!\r\n\tClosing Client " + client->remoteIP().toString());
          //  client->stop();
          //}
        }
        break;
      case MQTTMessage::Type::PubComp: // received only if you sent publish with QoS2 and aswered to a PubRec with a OubRel
        {
          uint16_t  messageID = message.getNextInt();
          mqttshow(String("PubComp Remove messageID= ") + String(messageID));
          outMessageIDs.remove(messageID);
        }
        break;

      case MQTTMessage::Type::UnSubscribe:
        {
          MQTTMessage::QoSs   QoS       = message.getQoS();
          uint16_t            MessageId = message.getNextInt();
          String              Topic1    = message.getNextString();
          mqttVector<String>  Removed;
          //MQTTMessage::Topic t = {Topic1, "",false,QoS_Topic1};
          if (subscriptionTopics.exists(Topic1))
          {
            Removed.push_back(Topic1);
            subscriptionTopics.remove(Topic1);
          }
          while(message.hasNext())
          {
            String otherTopic = message.getNextString();
            //MQTTMessage::Topic t1 = {otherTopic, "",false,QoS_otherTopic};
            if (subscriptionTopics.exists(otherTopic))
            {
              Removed.push_back(otherTopic);
              subscriptionTopics.remove(otherTopic);
            }
          }
          #ifdef MQTT_DEBUG
            Serial.println();
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|     %4s     | %9s | %3s | %6s | %4s | %5s |\r\n", 
                              "Type", "messageID", "QoS","Retain", "Will", "Clean");
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "| %12s |   %6d  |  %1d  |    %1d   |   %1d  |   %1d   |\r\n",
                            message.getTypeString(), MessageId, QoS, retain, willFlag, cleanStart);
            Serial.println( F("+--------------+-----------+-----+--------+------+-------+"));
            Serial.printf (   "|        Topic | %-39s |\r\n", Topic1.c_str());
            Serial.println( F("+--------------+-----------------------------------------+"));
          #endif
          MQTTMessage response;
          response.create(MQTTMessage::Type::UnSuback, MQTTMessage::QoS0);
          response.add(MessageId);
          response.hexdump("Sent", this);
          response.sendMessage(*client);
          if(this->broker)
          {
            this->broker->sendSubscribedMessages(Topic1, this);
            if (this->broker->unsubscribeCallback!=NULL)
              this->broker->unsubscribeCallback(this, Removed);
          }
        }        break;
    default:
      break;
    }
  }//else if(! (*client) || !client->connected()) mqttsdebug("Not Connected!");
      

}
void MQTTClient::onMessage        ( MQTTOnMessageCallbackFunction callback)
{
  this->onMessageCallback = callback;
}
void MQTTClient::onConnect        ( MQTTOnConnectCallbackFunction callback)
{
  this->onConnectCallback = callback;
}


void  MQTTBroker::begin                   ( uint16_t port, char *username, char *password)
{
  if (username!=NULL)
    this->username = String( username); 
  else
    this->username = String("");
  if (password!=NULL)
    this->password, String(password); 
  else
    this->password = String("");
  this->port = port;
  server = new TCPServer(port);
  server->begin(port);
}
MQTTBroker::MQTTBroker                    ( void )
{
  this->username = String("");
  this->password = String("");
}
MQTTBroker::~MQTTBroker                   ( void )
{
  delete server;
}

void  MQTTBroker::showClients             ( void )
{
  Serial.println("\r\nClient count: " + String(clients.size()));
  for (size_t i = 0; i < clients.size(); i++)
  {
    Serial.printf("%2d) %s %s\r\n", i, clients[i]->IP.toString().c_str(), clients[i]->client->connected()? "Connected" : "Not Connected");
  }
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
}

void  MQTTBroker::Handle                  ( void )
{
  if (server->hasClient())
  {
    TCPClient tcp_client = server->available();
      
    int index = this->IndexOf(&tcp_client);
//mqttdebug( index );

              
    if (index<0 || !clients[index]->client->connected())
    {
#ifdef MQTT_DEBUG
//mqttsdebug ("New Client Available");
//mqttdebug (tcp_client.remoteIP().toString());
#endif
      clients.push_back(new MQTTClient(this, tcp_client));
#ifdef MQTT_DEBUG
//mqttsdebug("Client Stored");
#endif
//mqttdebug(tcp_client.remoteIP());
//mqttdebug(clients.size());
      showClients();
    }else if (!clients[index]->client->connected())
    {
Serial.printf("replacing (%d): (%s)\r\n", index,  clients[index]->IP.toString().c_str());
//mqttdebug((uint32_t)clients[index]->client);
      clients[index]->client->stop();
      showClients();
//mqttsdebug("Old Client Stoped")
      clients[index]->client = &tcp_client;
      showClients();
//mqttsdebug("new Client Replaced")
    }
  }
  for (int i = ((int)clients.size())-1; i >=0 ; i--)
  {
    //mqttdebug(i);
    if (!clients[i]->client->connected())
    {
      Serial.printf("Stop client not connected (%d): (%s)\r\n", i,  clients[i]->IP.toString().c_str());
      //mqttsdebug("Deletein Row");
      //mqttdebug(i);
      clients[i]->client->stop();
      delete clients[i];
      clients.erase (clients.begin() + i);
      showClients();
    }
  }
    
  for (size_t i = 0; i < clients.size(); i++)
  {
    //mqttsdebug("Client Handled");
    clients[i]->handle();
  }
}
int   MQTTBroker::IndexOf                 ( TCPClient *client)
{
  for (size_t i = 0; i < clients.size(); i++)
  {
//mqttdebug(clients[i]->client->remoteIP().toString());
//mqttdebug(client->remoteIP().toString());
    if (clients[i]->client->remoteIP() == client->remoteIP())
      return i;
  }
  return -1;
}
int   MQTTBroker::IndexOf                 ( String topic)
{
  for (size_t i = 0; i < clients.size(); i++)
  {
    for (size_t j = 0; j < clients[i]->subscriptionTopics.size(); j++)
    {
      //if (clients[i]->topics[j].name == topic || clients[i]->topics[j].name == "#")
      if (clients[i]->subscriptionTopics[j]== topic || clients[i]->subscriptionTopics[j] == "#")
        return i;
    }
  }
  return -1;
}
void  MQTTBroker::distribuitePublish      ( String topic, MQTTMessage &message)
{
  for (size_t i = 0; i < clients.size(); i++)
  {
    clients[i]->publish(topic, message);

  }
}
void  MQTTBroker::sendSubscribedMessages  ( String topic, MQTTClient *client)
{
  mqttsdebug("sendSubscribedMessages");
  if (topic == "#")
  {
    for (size_t i = 0; i < AllTopics.size(); i++)
    {
      MQTTMessage response;
      MQTTMessage::Topic topic = AllTopics[i];
      response.createPublishMessage(topic.name, topic.value, MQTTMessage::QoS1);
      response.sendMessage(*client->client);
      response.hexdump("Sent", client);
    }
  }
  else
  {
    int index = AllTopics.indexOf(topic);
    if (index >= 0)
    {
      MQTTMessage response;
      MQTTMessage::Topic topic = AllTopics[index];
      response.createPublishMessage(topic.name, topic.value, MQTTMessage::QoS1);
      response.sendMessage(*client->client);
      response.hexdump("Sent", client);
    }
  }

}
void  MQTTBroker::onNewClient             ( MQTTOnNewClienCallbackFunction callback)
{
  this->newClientCallback = callback;
}
void  MQTTBroker::onSubscribe             ( MQTTOnSubscribeCallbackFunction callback)
{
  this->subscribeCallback = callback;
}
void  MQTTBroker::onUnsubscribe           ( MQTTOnUnsubscribeCallbackFunction callback)
{
  this->unsubscribeCallback = callback;
}
void  MQTTBroker::onPublish               ( MQTTOnPublishCallbackFunction callback)
{
  this->publishCallback = callback;
}

