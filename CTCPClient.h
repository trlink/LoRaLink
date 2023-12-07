#ifndef __CTCPCLIENT__
#define __CTCPCLIENT__

//includes
//////////
#include <Arduino.h>
#include <WiFi.h>
#include "CTaskHandler.h"
#include "SkyNetProtocol.h"
#include "CLoRaLinkDataIF.h"

//defines
/////////
#define TCP_PACKET_TIMEOUT 60000    //timeout for a packet of SKYNET_PROTO_MAX_PACKET_LEN bytes...
#define TCPCLIENT_DEBUG
//#define TCPCLIENT_DEBUGX


typedef void(*ONTCPCLIENTDATA)(void *pClient, byte *pData, int nDataLen);
typedef void(*ONTCPCLIENTDISC)(void *pClient);



class CTCPClient : public CLoRaLinkDataIF
{
  public:

    CTCPClient(char *szIP, int nPort);
    CTCPClient(WiFiClient pClient);
    ~CTCPClient();

    bool isConnected();
    void setCallBacks(ONTCPCLIENTDATA pOnData, ONTCPCLIENTDISC pOnDisc);

    void handleTask();
    
    bool sendData(byte *pData, int nDataLength);

    void closeConnection();
    
    //some handy references... ;)
    void setObjectReference(void *pRef);
    void* getObjectReference();

    bool canTransmit();

    WiFiClient getClient();

  private:
  
    //variables
    ///////////
    WiFiClient      m_pClient;
    ONTCPCLIENTDATA m_cbOnData;
    ONTCPCLIENTDISC m_cbOnDisc;
    byte           *m_pData;
    int             m_nPos;
    int             m_nPacketSize;
    long            m_lTimeout;
    void           *m_pObjectReference; 
};


#endif
