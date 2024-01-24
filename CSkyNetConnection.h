#ifndef __SKYNETCONNECTION__
#define __SKYNETCONNECTION__
//includes
//////////
#include <Arduino.h>
#include "CSkyNetConnHandler.h"
#include "CTaskHandler.h"
#include "CDBTaskScheduler.h"
#include <mutex>


#define SYNET_CONN_INFO
#define SYNET_CONN_DEBUG
#define SYNET_CONN_ERROR
//#define SYNET_CONN_TDEBUG


/**
 * this struct will be used, to transport data to send from the connection
 * to the data transmission task (IPC).
 */
struct _sSkyNetTransmission
{
  int       nConnType;
  int       nTaskID;
  uint32_t  dwTime;
  int       nTries;
  bool      bMsgSent;
  bool      bNeedConf;
  int       nMsgLen;
  byte      *pMsg;
  uint32_t  dwMsgID;
  uint32_t  dwDestinationNodeID;
};


/*
 * this callback will be called, when a DATA_IND was received. The callback 
 * will add the data to the loralink protocol handler.
 */
typedef void(*tOnLoRaLinkProtocolData)(void *pProtocolMsg, byte *pData, int nDataLen);



class CSkyNetConnection : public CTaskIF
{
  public:
    CSkyNetConnection(CDBTaskScheduler *pdbTaskScheduler, tOnLoRaLinkProtocolData pOnLoRaLinkProtoData);
    ~CSkyNetConnection();

    //connection handler handling
    /////////////////////////////
    CSkyNetConnectionHandler* getHandlerByTaskID(int nTaskID);
    void addHandler(CSkyNetConnectionHandler *pHandler);
    void removeHandler(CSkyNetConnectionHandler *pHandler);
    int  countConnHandlerByType(int nType);
    bool isConnected();

    //protocol handling
    ///////////////////
    void onLoRaLinkProtocolData(void *pProtocolMsg, byte *pData, int nDataLen);

    //task handling
    ///////////////
    void handleTask();
    void removeTaskSchedule(uint32_t dwScheduleID, bool bSuccess);
    CDBTaskScheduler* getTaskScheduler();
    

    //skynet protocol handling
    //////////////////////////
    uint32_t getMessageID();

    void enqueueQueryRequest(uint32_t dwNodeID, char *szNodeName, bool bForward, uint32_t dwScheduleID);

    //this method send a message over all known connections
    void enqueueMsg(uint32_t dwDestinationNodeID, uint32_t dwMsgID, byte *pMsg, int nMsgLen, bool bNeedConf);

    void enqueueMsgForType(uint32_t dwDestinationNodeID, uint32_t dwMsgID, byte *pMsg, int nMsgLen, bool bNeedConf, int nConnType, int nExcludeTaskID);

    //this method queues a message to be sent over a specific connection
    //primarilly used by the connection handler...
    void enqueueMsg(uint32_t dwDestinationNodeID, uint32_t dwMsgID, int nTaskID, int nConnType, byte *pMsg, int nMsgLen, bool bNeedConf);

    

    //set the message ID of a message which can be 
    //removed after confirmation 
    void removeConfirmedMessage(uint32_t dwReceiverID, uint32_t dwMsgID);

    

  private:

    //variables
    ///////////
    CWSFLinkedList          *m_pList;
    CWSFLinkedList          *m_pTxList;
    uint32_t                m_dwMsgID;
    CDBTaskScheduler        *m_pdbTaskScheduler;
    tOnLoRaLinkProtocolData m_pOnLoRaLinkProtoData;
    long                    m_lNextTX;

    //this method sends the queued messages over a connection
    void handleQueuedMessages();
};




#endif
