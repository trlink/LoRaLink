#ifndef __CTCPSERVER__
#define __CTCPSERVER__

//includes
//////////
#include <Arduino.h>
#include <WiFi.h>
#include "CTCPClient.h"
#include "CTaskHandler.h"


#define TCPSERVER_DEBUG 1



//callbacks
///////////
typedef void(*ONTCPCONNECT)(void *pClient);



class CTCPServer : public CTaskIF
{
  public:
    CTCPServer(int nPort, ONTCPCONNECT onConnectCB);
    ~CTCPServer();

    void handleTask();
    
  private:
  
    //variables
    ///////////
    WiFiServer  *m_pServer;
    ONTCPCONNECT m_onConnectCB;
};

#endif
