//includes
//////////
#include "CSkyNetConnection.h"
#include "CLoRaLinkDatabase.h"
#include "CLoRaModem.h"
#include "SkyNetProtocol.h"
#include "SystemState.h"


CSkyNetConnection::CSkyNetConnection(CDBTaskScheduler *pdbTaskScheduler, tOnLoRaLinkProtocolData pOnLoRaLinkProtoData)
{
  this->m_pList                 = new CWSFLinkedList();
  this->m_pTxList               = new CWSFLinkedList();
  this->m_dwMsgID               = 0;
  this->m_pdbTaskScheduler      = pdbTaskScheduler;
  this->m_pOnLoRaLinkProtoData  = pOnLoRaLinkProtoData;
  this->m_lNextTX               = 0;
};



CSkyNetConnection::~CSkyNetConnection()
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  
  this->m_pList->itterateStart();
  
  do 
  {
    pTask = (CSkyNetConnectionHandler*)this->m_pList->getNextItem();

    if(pTask != NULL)
    {
      this->removeHandler(pTask);
    
      pTask = (CSkyNetConnectionHandler*)this->m_pList->getNextItem();
    };
  } while(pTask != NULL);
};


void CSkyNetConnection::removeConfirmedMessage(uint32_t dwReceiverID, uint32_t dwMsgID)
{
  //variables
  ///////////
  _sListItem            *item     = this->m_pTxList->getList();
  _sSkyNetTransmission  *pTransmission;

  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] --> removeConfirmedMessage() msg: "));
    Serial.print(dwReceiverID);
    Serial.print(F(" ID: "));
    Serial.println(dwMsgID);
  #endif
  
  if(this->m_pTxList->getItemCount() > 0)
  {
    while(item != NULL)
    {
      pTransmission = (_sSkyNetTransmission*)item->pItem;

      if(pTransmission != NULL)
      {
        #ifdef SYNET_CONN_DEBUG
          Serial.print(F("[CSNConn] check Queued msg to: "));
          Serial.print(pTransmission->dwDestinationNodeID);
          Serial.print(F(" msd id: "));
          Serial.println(pTransmission->dwMsgID);
        #endif
          
        if((pTransmission->dwDestinationNodeID == dwReceiverID) && (pTransmission->dwMsgID == dwMsgID))
        {
          #ifdef SYNET_CONN_DEBUG
            Serial.print(F("[CSNConn] remove Queued msg to: "));
            Serial.println(pTransmission->dwDestinationNodeID);
          #endif
          
          this->m_pTxList->removeItem(pTransmission);

          delete pTransmission->pMsg;
          delete pTransmission;

          #ifdef SYNET_CONN_DEBUG
            Serial.print(F("[CSNConn] <-- removeConfirmedMessage() msg to: "));
            Serial.println(dwReceiverID);
          #endif

          return;
        };
      };
      
      item = item->pNext;
    };
  };

  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] <-- removeConfirmedMessage(): unable to remove Queued msg to: "));
    Serial.println(dwReceiverID);
  #endif
};


CDBTaskScheduler* CSkyNetConnection::getTaskScheduler()
{
  return this->m_pdbTaskScheduler;
};


void CSkyNetConnection::enqueueQueryRequest(uint32_t dwNodeID, char *szNodeName, bool bForward, uint32_t dwScheduleID)
{
  //variables
  ///////////
  byte bData[256];
  int  nLength;
  uint32_t dwMsgID = this->getMessageID(); 

  Serial.print(F("[CSNConn] query dev: "));
  Serial.println(szNodeName);
  
  nLength = QUERY_REQ((byte*)&bData, dwMsgID, DeviceConfig.dwDeviceID, 0, DeviceConfig.dwDeviceID, 0, bForward, szNodeName, dwNodeID, dwScheduleID);

  this->enqueueMsg(0, dwMsgID, (byte*)&bData, nLength, false);
};


void CSkyNetConnection::onLoRaLinkProtocolData(void *pProtocolMsg, byte *pData, int nDataLen)
{
  this->m_pOnLoRaLinkProtoData(pProtocolMsg, pData, nDataLen);
};


void CSkyNetConnection::removeTaskSchedule(uint32_t dwScheduleID, bool bSuccess)
{
  this->m_pdbTaskScheduler->removeSchedule(dwScheduleID, bSuccess);
};



uint32_t CSkyNetConnection::getMessageID()
{
  this->m_dwMsgID += 1;
  
  if(this->m_dwMsgID == 0xFFFF)
  {
    this->m_dwMsgID = 0;
  };
  
  return this->m_dwMsgID;
};




void CSkyNetConnection::enqueueMsg(uint32_t dwDestinationNodeID, uint32_t dwMsgID, byte *pMsg, int nMsgLen, bool bNeedConf)
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  
  this->m_pList->itterateStart();
  
  do 
  {
    pTask = (CSkyNetConnectionHandler*)this->m_pList->getNextItem();

    if(pTask != NULL)
    {
      this->enqueueMsg(dwDestinationNodeID, dwMsgID, pTask->getTaskID(), pTask->getConnectionType(), pMsg, nMsgLen, bNeedConf);
    };
  } while(pTask != NULL);
};



void CSkyNetConnection::enqueueMsg(uint32_t dwDestinationNodeID, uint32_t dwMsgID, int nTaskID, int nConnType, byte *pMsg, int nMsgLen, bool bNeedConf)
{
  //variables
  ///////////
  _sSkyNetTransmission *pTransmission;

  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] enqueue "));
    Serial.print(nMsgLen);
    Serial.print(F(" bytes for TaskID: "));
    Serial.println(nTaskID);
  #endif
  
  pTransmission            = new _sSkyNetTransmission;
  pTransmission->nTaskID   = nTaskID;
  pTransmission->nConnType = nConnType;
  pTransmission->nMsgLen   = nMsgLen;
  
  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] enqueue: "));
    Serial.print(nMsgLen);
    Serial.print(F(" bytes for TaskID: "));
    Serial.println(pTransmission->nTaskID);
  #endif

  pTransmission->dwTime               = millis();
  pTransmission->nTries               = 0;
  pTransmission->bMsgSent             = false;
  pTransmission->bNeedConf            = bNeedConf;
  pTransmission->dwMsgID              = dwMsgID;
  pTransmission->dwDestinationNodeID  = dwDestinationNodeID;
  pTransmission->pMsg                 = new byte[nMsgLen + 1];
  memcpy(pTransmission->pMsg, pMsg, nMsgLen);
  
  this->m_pTxList->addItem(pTransmission);
};



void CSkyNetConnection::enqueueMsgForType(uint32_t dwDestinationNodeID, uint32_t dwMsgID, byte *pMsg, int nMsgLen, bool bNeedConf, int nConnType, int nExcludeTaskID)
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  _sSkyNetTransmission *pTransmission;
      
  
  this->m_pList->itterateStart();
  
  do 
  {
    pTask = (CSkyNetConnectionHandler*)this->m_pList->getNextItem();

    if(pTask != NULL)
    {
      if((pTask->getConnectionType() == nConnType) && (pTask->getTaskID() != nExcludeTaskID))
      {
        #ifdef SYNET_CONN_DEBUG
          Serial.print(F("[CSNConn] enqueueMsgForType: type: "));
          Serial.print(nConnType);
          Serial.print(F(" exclude: "));
          Serial.print(nExcludeTaskID);
        #endif
        
        this->enqueueMsg(dwDestinationNodeID, dwMsgID, pTask->getTaskID(), pTask->getConnectionType(), pMsg, nMsgLen, bNeedConf);
      };
    };
  } while(pTask != NULL);
};



CSkyNetConnectionHandler* CSkyNetConnection::getHandlerByTaskID(int nTaskID)
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  _sListItem               *pList = this->m_pList->getList();

  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] getHandlerByTaskID: "));
    Serial.println(nTaskID);
  #endif
  
  while(pList != NULL)
  {
    pTask = (CSkyNetConnectionHandler*)pList->pItem;

    if(pTask != NULL)
    {
      #ifdef SYNET_CONN_DEBUG
        Serial.print(F("[CSNConn] Check TaskID: "));
        Serial.println(pTask->getTaskID());
      #endif
      
      if(pTask->getTaskID() == nTaskID)
      {
         return pTask;
      };
    };

    pList = pList->pNext;
  };


  #ifdef SYNET_CONN_ERROR
    Serial.print(F("[CSNConn] getHandlerByTaskID: Not found: "));
    Serial.println(nTaskID);
  #endif

  return NULL;
};


void CSkyNetConnection::handleTask()
{
  //variables
  ///////////
  
  #ifdef SYNET_CONN_TDEBUG
    Serial.print(F("[CSNConn] --> handleTask: TaskID: "));
    Serial.println(this->getTaskID());
  #endif

  if(this->m_lNextTX < millis())
  {
    this->handleQueuedMessages();
  };
  
  #ifdef SYNET_CONN_TDEBUG
    Serial.print(F("[CSNConn] <-- handleTask: TaskID: "));
    Serial.println(this->getTaskID());
  #endif
};


void CSkyNetConnection::handleQueuedMessages()
{
  //variables
  ///////////
  CSkyNetConnectionHandler  *pConnHandler;
  _sSkyNetTransmission      *pTransmission;
  
  LLSystemState.lOutstandingMsgs = this->m_pTxList->getItemCount();
    
  if(LLSystemState.lOutstandingMsgs > 0)
  {
    this->m_pTxList->itterateStart();

    do
    {
      pTransmission = (_sSkyNetTransmission*)this->m_pTxList->getNextItem();
      
      if(pTransmission != NULL)
      {        
        #ifdef SYNET_CONN_DEBUG
          Serial.print(F("[CSNConn] dequeue msg ID: "));
          Serial.print(pTransmission->dwMsgID);
          Serial.print(F(" for TaskID: "));
          Serial.print(pTransmission->nTaskID);
          Serial.print(F(" sent: "));
          Serial.print(pTransmission->bMsgSent);
          Serial.print(F(" Receiver: "));
          Serial.print(pTransmission->dwDestinationNodeID);
          Serial.print(F(" msg size: "));
          Serial.print(pTransmission->nMsgLen);
          Serial.print(F(" tries: "));
          Serial.print(pTransmission->nTries);
          Serial.print(F(" msgs remaining: "));
          Serial.println(LLSystemState.lOutstandingMsgs);
        #endif

        if(pTransmission->bMsgSent == false)
        {
          pConnHandler = this->getHandlerByTaskID(pTransmission->nTaskID);
  
          if(pConnHandler != NULL)
          {
            if(((ModemConfig.bDisableLoRaModem == true) && (pConnHandler->getConnectionType() == SKYNET_CONN_TYPE_LORA)) || (DeviceConfig.dwDeviceID <= 0))
            {
              #ifdef SYNET_CONN_INFO
                Serial.println(F("[CSNConn] LoRa Modem disabled"));
              #endif
      
              //modem disabled
              this->m_pTxList->removeItem(pTransmission);

              delete pTransmission->pMsg;
              delete pTransmission;

              return;
            }
            else
            {
              if(pConnHandler->canTransmit() == true)
              {
                #ifdef SYNET_CONN_DEBUG
                  Serial.println(F("[CSNConn] Send Data"));
                #endif
                
                if(pConnHandler->sendData(pTransmission->pMsg, pTransmission->nMsgLen) == true)
                {
                  this->m_lNextTX = millis() + ModemConfig.lMessageTransmissionInterval;
                  
                  if(pTransmission->bNeedConf == true)
                  {
                    if(pTransmission->nTries < SKYNET_PROTO_RETRIES)
                    {
                      pTransmission->bMsgSent  = true;
                      pTransmission->nTries   += 1;
                      pTransmission->dwTime    = millis();
                    }
                    else
                    {
                      #ifdef SYNET_CONN_ERROR
                        Serial.print(F("[CSNConn] Error / Timeout send data via TaskID: "));
                        Serial.println(pTransmission->nTaskID);
                      #endif
    
                      this->m_pTxList->removeItem(pTransmission);

                      delete pTransmission->pMsg;
                      delete pTransmission;

                      return;
                    };
                  }
                  else
                  {
                    #ifdef SYNET_CONN_DEBUG
                      Serial.print(F("[CSNConn] remove msg "));
                      Serial.print(F(" for TaskID: "));
                      Serial.println(pTransmission->nTaskID);  
                    #endif
                    
                    this->m_pTxList->removeItem(pTransmission);

                    delete pTransmission->pMsg;
                    delete pTransmission;

                    return;
                  };
                }
                else
                {
                  #ifdef SYNET_CONN_ERROR
                    Serial.print(F("[CSNConn] Error send data via TaskID: "));
                    Serial.println(pTransmission->nTaskID);
                  #endif
                };
              }
              else
              {
                #ifdef SYNET_CONN_DEBUG
                  Serial.println(F("[CSNConn] Modem can't Transmit"));
                #endif
              };
            };
          }
          else 
          {
            #ifdef SYNET_CONN_DEBUG
              Serial.print(F("[CSNConn] Error find TaskID: "));
              Serial.println(pTransmission->nTaskID);
            #endif
  
            this->m_pTxList->removeItem(pTransmission);

            delete pTransmission->pMsg;
            delete pTransmission;

            return;
          };
        }
        else
        {
          if(millis() > (pTransmission->dwTime + SKYNET_PROTO_MSG_TIMEOUT))
          {
            if(pTransmission->nTries < SKYNET_PROTO_RETRIES)
            {
              #ifdef SYNET_CONN_ERROR
                Serial.print(F("[CSNConn] Error/Timeout sending data via TaskID: "));
                Serial.print(pTransmission->nTaskID);
                Serial.println(F(" - resend"));
              #endif
  
              pTransmission->bMsgSent = false;
            }
            else
            {
              #ifdef SYNET_CONN_ERROR
                Serial.print(F("[CSNConn] Error/Timeout sending data via TaskID: "));
                Serial.print(pTransmission->nTaskID);
                Serial.println(F(" - remove"));
              #endif
              
              this->m_pTxList->removeItem(pTransmission);

              delete pTransmission->pMsg;
              delete pTransmission;

              return;
            };
          };
        };
      };
      
      ResetWatchDog();

      LLSystemState.lOutstandingMsgs = this->m_pTxList->getItemCount();
    }
    while(pTransmission != NULL);
  }
  else
  {
    #ifdef SYNET_CONN_TDEBUG
      Serial.println(F("[CSNConn] no msgs queued"));
    #endif
  };
};



void CSkyNetConnection::addHandler(CSkyNetConnectionHandler *pHandler)
{
  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] Add Handler TaskID: "));
    Serial.println(pHandler->getTaskID());
  #endif
  
  this->m_pList->addItem(pHandler);
};



bool CSkyNetConnection::isConnected()
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  _sListItem               *pList = this->m_pList->getList();

  while(pList != NULL)
  {
    pTask = (CSkyNetConnectionHandler*)pList->pItem;

    if(pTask != NULL)
    {
      if(pTask->getState() == SKYNET_CONN_STATE_CONNECTED)
      {
         return true;
      };
    };

    pList = pList->pNext;
  };
  
  return false;
};


int  CSkyNetConnection::countConnHandlerByType(int nType)
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pTask; 
  _sListItem               *pList = this->m_pList->getList();
  int nCount = 0;

  while(pList != NULL)
  {
    pTask = (CSkyNetConnectionHandler*)pList->pItem;

    if(pTask != NULL)
    {
      if(pTask->getConnectionType() == nType)
      {
         nCount += 1;
      };
    };

    pList = pList->pNext;
  };
  
  return nCount;
};



void CSkyNetConnection::removeHandler(CSkyNetConnectionHandler *pHandler)
{
  #ifdef SYNET_CONN_DEBUG
    Serial.print(F("[CSNConn] Remove Handler TaskID: "));
    Serial.println(pHandler->getTaskID());
  #endif
  
  this->m_pList->removeItem(pHandler);
};
