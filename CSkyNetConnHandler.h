#ifndef __SKYNETCONNHANDLER__
#define __SKYNETCONNHANDLER__

//includes
//////////
#include <Arduino.h>
#include "CTaskHandler.h"
#include "CLoRaLinkDataIF.h"




//defines
/////////
#define SKYNET_CONN_DEBUG                 1
#define SKYNET_CONN_INFO                  1
#define SKYNET_CONN_ERROR                 1


#define SKYNET_CONN_TYPE_LORA             1
#define SKYNET_CONN_TYPE_IP_CLIENT        2
#define SKYNET_CONN_TYPE_IP_SERVER        3

#define SKYNET_CONN_STATE_INIT            0
#define SKYNET_CONN_STATE_CONNECTING      1
#define SKYNET_CONN_STATE_CONNECTED       2


#define SKYNET_CONN_TIMEOUT_CONNECT       30000   /*time till we need to receive the HELLO_CONF*/
#define SKYNET_CONN_TIMEOUT_KEEPALIVE     300000  /*send something every 5 mins*/
#define SKYNET_CONN_TIMEOUT_SILENCE       900000



//this class handles the SkyNetProtocol-connections
//normally there are max 2 connections (IP and LoRa)...
//
class CSkyNetConnectionHandler : public CTaskIF
{
  public:
  
    CSkyNetConnectionHandler(CLoRaLinkDataIF *pModemConnection, int nConnType, void *pSkyNetConnection);
    ~CSkyNetConnectionHandler();

    void handleConnData(byte *pData, int nDataLength, int nRSSI, float fPacketSNR);
    void handleTask();

    int  getConnectionType();
    int  getState();


    bool canTransmit();

    bool sendData(byte *pData, int nDataLength);

    
  private:
    //variables
    ///////////
    CLoRaLinkDataIF   *m_pModemConnection;
    void              *m_pSkyNetConnection;
    int               m_nState;
    long              m_lStateTimer;
    long              m_lKeepAliveTimer;
    long              m_lSilenceTimer;
    int               m_nConnType;
    int               m_nReconnectCount;
    

    

    bool isKnownNode(uint32_t dwNodeID);
    bool insertOrUpdateIntoKnownNodes(uint32_t dwNodeID, char *szName, byte *pDevType, float fLocN, float fLocE, char *szLocOrientation, int nRSSI, float fPacketSNR);
    bool forwardMessage(void *pMessage, bool bConfirm);
};







#endif
