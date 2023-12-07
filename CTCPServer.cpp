//includes
//////////
#include "CTCPServer.h"


CTCPServer::CTCPServer(int nPort, ONTCPCONNECT onConnectCB)
{
  this->m_onConnectCB = onConnectCB;
  this->m_pServer     = new WiFiServer(nPort);
  this->m_pServer->begin();
  this->m_pServer->setNoDelay(true);
};
 

CTCPServer::~CTCPServer()
{
  delete this->m_pServer;
};


void CTCPServer::handleTask()
{
  if(this->m_pServer->hasClient() == true)
  {
    #if TCPSERVER_DEBUG == 1
      Serial.println(F("CTCPServer: client connect"));
    #endif
    
    CTCPClient *pClient = new CTCPClient(this->m_pServer->available());      

    this->m_onConnectCB(pClient);
  };
};
    
  
