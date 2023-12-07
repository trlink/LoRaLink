//includes
//////////
#include "CTCPClient.h"
#include "HardwareConfig.h"


CTCPClient::CTCPClient(char *szIP, int nPort)
{
  #ifdef TCPCLIENT_DEBUG
    Serial.print(F("[CTPCLNT] Connect: "));
    Serial.print(szIP);
    Serial.print(F(":"));
    Serial.println(nPort);
  #endif
  
  if(this->m_pClient.connect(szIP, nPort) == false)
  {
    #ifdef TCPCLIENT_DEBUG
      Serial.println(F("[CTPCLNT] Connect Failed"));
    #endif
    
    this->m_nPos        = 1;
    this->m_nPacketSize = 1;
    this->m_lTimeout    = 0;
  }
  else 
  {
    #ifdef TCPCLIENT_DEBUG
      Serial.println(F("[CTPCLNT] Connect OK"));
    #endif
    
    this->m_nPos        = 0;
    this->m_nPacketSize = 0;
    this->m_lTimeout    = 0;
  };

  this->m_pObjectReference = NULL;
  this->m_cbOnData         = NULL;
  this->m_cbOnDisc         = NULL;
  this->m_pData            = new byte[SKYNET_PROTO_MAX_PACKET_LEN + 1];
};



CTCPClient::CTCPClient(WiFiClient pClient)
{
  #ifdef TCPCLIENT_DEBUG
    Serial.println(F("[CTPCLNT] Create Client"));
  #endif
    
  this->m_pClient           = pClient;
  this->m_nPos              = 0;
  this->m_nPacketSize       = 0;
  this->m_lTimeout          = 0;
  this->m_pObjectReference  = NULL;
  this->m_cbOnData          = NULL;
  this->m_cbOnDisc          = NULL;
  this->m_pData             = new byte[SKYNET_PROTO_MAX_PACKET_LEN + 1];
};


CTCPClient::~CTCPClient()
{
  if(this->m_pClient.connected() == true)
  {
    this->closeConnection();
  };

  delete this->m_pData;
};

bool CTCPClient::canTransmit()
{
  if(this->m_pClient.connected() == true)
  {
    return true;
  };

  return false;
};

bool CTCPClient::isConnected()
{
  return this->m_pClient.connected();
};

void CTCPClient::setObjectReference(void *pRef)
{
  this->m_pObjectReference = pRef;
};



void* CTCPClient::getObjectReference()
{
  return this->m_pObjectReference;
};

    

void CTCPClient::setCallBacks(ONTCPCLIENTDATA pOnData, ONTCPCLIENTDISC pOnDisc)
{
  this->m_cbOnData = pOnData;
  this->m_cbOnDisc = pOnDisc;
};



WiFiClient CTCPClient::getClient()
{
  return this->m_pClient;
};



bool CTCPClient::sendData(byte *pData, int nDataLength)
{
  #ifdef TCPCLIENT_DEBUG
    Serial.println(F("[CTPCLNT] --> sendData()"));
  #endif
  
  if(this->m_pClient.connected() == true)
  {
    this->m_pClient.write(nDataLength & 0xFF);
    this->m_pClient.write(pData, nDataLength);

    ResetWatchDog();
  };

  #ifdef TCPCLIENT_DEBUG
    Serial.println(F("[CTPCLNT] <-- sendData()"));
  #endif
  
  return this->m_pClient.connected();
};



void CTCPClient::closeConnection()
{
  #ifdef TCPCLIENT_DEBUG
    Serial.println(F("[CTPCLNT] Close Conn"));
  #endif
  
  this->m_pClient.stop();

  this->m_nPos        = 1;
  this->m_nPacketSize = 1;
  this->m_lTimeout    = 0;
};


void CTCPClient::handleTask()
{
  if(this->m_pClient.connected() == true)
  {
    #ifdef TCPCLIENT_DEBUGX
      Serial.println(F("[CTPCLNT] --> Handle RX"));
    #endif
    
    //reset timeout only when not 
    //receiving a packet
    if(this->m_nPacketSize == 0)
    {
      this->m_lTimeout    = millis() + TCP_PACKET_TIMEOUT;
    };

    //wait until packet complete, when something was received
    //or wait till timeout until something is received
    while((((this->m_pClient.available() > 0) && (this->m_lTimeout > millis())) || ((this->m_nPacketSize > 0) && (this->m_lTimeout > millis()))) && (this->m_pClient.connected() == true))
    {
      ResetWatchDog();

      if(this->m_pClient.available() > 0)
      {
        //start of packet
        if(this->m_nPacketSize == 0)
        {
          this->m_nPacketSize = this->m_pClient.read();
  
          #ifdef TCPCLIENT_DEBUG
            Serial.print(F("[CTPCLNT] wait for "));
            Serial.println(this->m_nPacketSize);
          #endif
  
          if((this->m_nPacketSize > SKYNET_PROTO_MAX_PACKET_LEN) || (this->m_nPacketSize <= 0))
          {
            #ifdef TCPCLIENT_DEBUG
              Serial.println(F("[CTPCLNT] Disc on Proto Err"));
            #endif
            
            //disc on error
            this->m_pClient.stop();
            this->m_cbOnDisc(this);
  
            break;
          }
          else
          {
            this->m_lTimeout    = millis() + TCP_PACKET_TIMEOUT;
            this->m_nPos        = 0;
          };
        }
        else
        {
          this->m_lTimeout              = millis() + TCP_PACKET_TIMEOUT;
          this->m_pData[this->m_nPos++] = this->m_pClient.read();
  
          if(this->m_nPos == this->m_nPacketSize)
          {
            #ifdef TCPCLIENT_DEBUG
              Serial.println(F("[CTPCLNT] PKT Complete"));
            #endif
            
            if(this->m_cbOnData != NULL)
            {
              this->m_cbOnData(this, (byte*)&this->m_pData, this->m_nPos);
            };
  
            this->m_nPos        = 0;
            this->m_nPacketSize = 0;
  
            break;
          };
        };
      };
    };

      
    if((this->m_nPacketSize > 0) && (this->m_lTimeout < millis()))
    {
      #ifdef TCPCLIENT_DEBUG
        Serial.println(F("[CTPCLNT] Disc on timeout"));
      #endif
      
      this->m_pClient.stop();
      
      if(this->m_cbOnDisc != NULL)
      {
        this->m_cbOnDisc(this);
      };
    };
  }
  else
  {
    #ifdef TCPCLIENT_DEBUG
      Serial.println(F("[CTPCLNT] Disconnected"));
    #endif
    
    if(this->m_cbOnDisc != NULL)
    {
      this->m_cbOnDisc(this);
    };
  };

  #ifdef TCPCLIENT_DEBUGX
    Serial.println(F("[CTPCLNT] <-- Handle RX"));
  #endif
};
