//includes
//////////
#include "CSkyNetConnHandler.h"
#include "SkyNetProtocol.h"
#include "LoRaLinkConfig.h"
#include "CTCPClient.h"
#include "CWSFClockHelper.h"
#include "CLoRaLinkDatabase.h"
#include "CSkyNetConnection.h"
#include "CSkyNetRouting.h"




CSkyNetConnectionHandler::CSkyNetConnectionHandler(CLoRaLinkDataIF *pModemConnection, int nConnType, void *pSkyNetConnection)
{
  this->m_nState                  = SKYNET_CONN_STATE_INIT;
  this->m_pSkyNetConnection       = pSkyNetConnection;
  this->m_pModemConnection        = pModemConnection;
  this->m_lStateTimer             = 0;
  this->m_nConnType               = nConnType;
  this->m_lSilenceTimer           = millis();
  this->m_nReconnectCount         = 0;
};


CSkyNetConnectionHandler::~CSkyNetConnectionHandler()
{
};

bool CSkyNetConnectionHandler::canTransmit()
{
  return this->m_pModemConnection->canTransmit();
};

int  CSkyNetConnectionHandler::getState()
{
  return this->m_nState;
};

int  CSkyNetConnectionHandler::getConnectionType()
{
  return this->m_nConnType;
};



void CSkyNetConnectionHandler::handleConnData(byte *pData, int nDataLength, int nRSSI, float fPacketSNR)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szTemp[20];
  byte *pAnswer = new byte[SKYNET_PROTO_MAX_PACKET_LEN + 1];
  int  nAnswerLen;
  

  #if SKYNET_CONN_DEBUG == 1
    Serial.print(F("--> handleConnData("));
    Serial.print(this->getTaskID());
    Serial.println(F(")"));
  #endif
  
  if(decodeSkyNetProtocolMessage(pData, nDataLength, (_sSkyNetProtocolMessage*)&msg) == true)
  {
    //reset timer, when something received
    this->m_lSilenceTimer   = millis() + SKYNET_CONN_TIMEOUT_SILENCE;


    //check if the dev is "blocked"
    //this is for debugging purposes
    if(strlen(DeviceConfig.szBlockedNodes) > 0)
    {
      memset(szTemp, 0, sizeof(szTemp));
      sprintf_P(szTemp, PSTR("%lu"), msg.nSenderID);

      if(strstr(DeviceConfig.szBlockedNodes, szTemp) != NULL)
      {
        #if SKYNET_CONN_DEBUG == 1
          Serial.print(F("[CHandler] Packet from: "));
          Serial.print(msg.nSenderID);
          Serial.println(F(" is blocked"));
        #endif


        delete pAnswer;

        return;
      };
    };

    
    #if SKYNET_CONN_INFO == 1
      Serial.print(F("[CHandler] handle MSG from: "));
      Serial.print(msg.nSenderID);
      Serial.print(F(" for: "));
      Serial.print(msg.nReceiverID);
      Serial.print(F(" Origin ID: "));
      Serial.print(msg.nOriginID);
      Serial.print(F(" via: "));
      Serial.print(msg.nViaID);
      Serial.print(F(" MsgID: "));
      Serial.print(msg.dwMsgID);
      Serial.print(F(" Hops: "));
      Serial.println(msg.nHopCount);
    #endif

    //check if sender dev is configurred
    if((msg.nSenderID > 0) && (msg.nSenderID != DeviceConfig.dwDeviceID))
    {
      //update node timeout, so that the
      //node remains in the routing table
      UpdateNodeTimeout(msg.nSenderID);
      
      //check sub-commands
      switch((msg.bMsgType & 0xF0) >> 4)
      {
        case SKYNET_SUBCMD_REQ:
        {
          switch(msg.bMsgType & 0x0F)
          {
            case SKYNET_CMD_QUERY:
            {
              //variables
              ///////////
              CWSFFileDBRecordset rs(g_pNodeTable);
              uint32_t dwScheduleID = 0;
              uint32_t dwDevID = 0;
              char     szDevName[30];
              int      nPos = 0;
              bool     bFwd = false;
              bool     bFound = false;
              uint32_t dwOldMsgID;
              int      nDevType = 0;
              
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] QUERY_REQ from: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" for: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" Origin ID: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
  
  
              if((msg.nReceiverID == DeviceConfig.dwDeviceID) || (msg.nReceiverID == 0))
              {
                memset(szDevName, 0, sizeof(szDevName));
  
                bFwd = (msg.pData[nPos] == 1 ? true : false);
                nPos += 1;
  
                memcpy((char*)&szDevName, msg.pData + nPos + 1, msg.pData[nPos]);
                nPos += msg.pData[nPos] + 1;
  
                dwDevID = ReadDWORD(msg.pData + nPos);
                nPos   += sizeof(uint32_t);
  
                dwScheduleID = ReadDWORD(msg.pData + nPos);
                nPos   += sizeof(uint32_t);
                            
                //query by dev id?
                if(dwDevID > 0)
                {
                  if(dwDevID != DeviceConfig.dwDeviceID)
                  {
                    if(FindDeviceByNodeID(&rs, dwDevID) == true)
                    {
                      memset(szDevName, 0, sizeof(szDevName));
                      rs.getData(1, (void*)&szDevName, sizeof(szDevName)); 
                      rs.getData(2, (void*)&nDevType, sizeof(nDevType));
  
                      #if SKYNET_CONN_INFO == 1
                        Serial.print(F("[CHandler] Found node with name: "));
                        Serial.print(szDevName);
                        Serial.print(F(" of type: "));
                        Serial.println(nDevType);
                      #endif
                      
                      nAnswerLen = QUERY_RESP(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.nOriginID, 0, (char*)&szDevName, dwDevID, dwScheduleID, nDevType);
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
  
                      bFound = true;
                    };
                  }
                  else
                  {
                    #if SKYNET_CONN_INFO == 1
                      Serial.print(F("[CHandler] Found node with name (me): "));
                      Serial.println(DeviceConfig.szDevName);
                    #endif
                    
                    //he is query my dev
                    nAnswerLen = QUERY_RESP(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.nOriginID, 0, (char*)&DeviceConfig.szDevName, DeviceConfig.dwDeviceID, dwScheduleID, DeviceConfig.nDeviceType);
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
  
                    bFound = true;
                  };
                };
  
  
                //search by name
                if(bFound == false)
                {
                  if((strlen(szDevName) > 0) && (dwDevID == 0))
                  {
                    if(strcasecmp(szDevName, DeviceConfig.szDevName) != 0)
                    {
                      if(FindDeviceByNodeName(&rs, szDevName) == true)
                      {
                        rs.getData(0, (void*)&dwDevID, sizeof(dwDevID)); 
                        rs.getData(2, (void*)&nDevType, sizeof(nDevType));
      
                        #if SKYNET_CONN_INFO == 1
                          Serial.print(F("[CHandler] Found node with ID: "));
                          Serial.print(dwDevID);
                          Serial.print(F(" of type: "));
                          Serial.println(nDevType);
                        #endif
      
                        nAnswerLen = QUERY_RESP(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.nOriginID, 0, (char*)&szDevName, DeviceConfig.dwDeviceID, dwScheduleID, nDevType);
                        ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);

                        bFound = true;
                      };
                    }
                    else
                    {
                      #if SKYNET_CONN_INFO == 1
                        Serial.print(F("[CHandler] Found node (me) with ID: "));
                        Serial.println(dwDevID);
                      #endif
                        
                      //he is query my dev
                      nAnswerLen = QUERY_RESP(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.nOriginID, 0, (char*)&DeviceConfig.szDevName, DeviceConfig.dwDeviceID, dwScheduleID, DeviceConfig.nDeviceType);
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);

                      bFound = true;
                    };
                  };
                };


                //if it was not possible to resolve by name or ID forward the request
                if(bFound == false)
                {
                  
                };
              }
              else
              {
                if(msg.nViaID == DeviceConfig.dwDeviceID)
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.print(F("[CHandler] forward msg to: "));
                    Serial.println(msg.nReceiverID);
                  #endif
  
                  nAnswerLen = PROTOCOL_MSG_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, !this->forwardMessage((_sSkyNetProtocolMessage*)&msg, true));
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
              };
            };
            break;
  
            
            case SKYNET_CMD_HELLO:
            {
              //variables
              ///////////
              uint32_t              dwMsgID;
              uint32_t              dwNewVia;
              _sSkyNetRoutingEntry  route;
              bool                  bForwarded = false;
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] HELLO_REQ from: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" for: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" Origin ID: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
  
  
              
              if(msg.nReceiverID == DeviceConfig.dwDeviceID)
              {
                //store sender as via
                dwNewVia   = msg.nSenderID;
                dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                
                //check if we got a request from a device 
                //for another device (forwarded request). 
                //if true, check the routing table, if we have 
                //another (maybe direct route) 
                if((dwNewVia != 0) && (dwNewVia != msg.nOriginID))
                {
                  if(SearchBestMatchingRoute((msg.nOriginID != 0 ? msg.nOriginID : msg.nSenderID), (_sSkyNetRoutingEntry*)&route) == true)
                  {
                    #if SKYNET_CONN_INFO == 1
                      Serial.print(F("[CHandler] HELLO_REQ for: "));
                      Serial.print(msg.nOriginID);
                      Serial.print(F(" returned new Via: "));
                      Serial.println(route.dwViaNode);
                    #endif

                    bForwarded  = true;
                    dwNewVia    = route.dwViaNode;

                    nAnswerLen = HELLO_IND(pAnswer, dwMsgID, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, dwNewVia, DeviceConfig.szDevName, DeviceConfig.nDeviceType & 0xFF, DeviceConfig.fLocN, DeviceConfig.fLocE, DeviceConfig.cPosOrientation, 0);
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, dwMsgID, route.pConnHandler->getTaskID(), route.pConnHandler->getConnectionType(), pAnswer, nAnswerLen, false);       
                  };
                };

                if(bForwarded == false)
                {
                  nAnswerLen = HELLO_IND(pAnswer, dwMsgID, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, DeviceConfig.szDevName, DeviceConfig.nDeviceType & 0xFF, DeviceConfig.fLocN, DeviceConfig.fLocE, DeviceConfig.cPosOrientation, 0);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
              }
              else
              {
                if(msg.nViaID == DeviceConfig.dwDeviceID)
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.print(F("[CHandler] forward msg to: "));
                    Serial.println(msg.nReceiverID);
                  #endif
                  
                  nAnswerLen = PROTOCOL_MSG_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, !this->forwardMessage((_sSkyNetProtocolMessage*)&msg, true));
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
              };
            };
            break;
          };
        };
        break;
  
        case SKYNET_SUBCMD_RESP:
        {
          switch(msg.bMsgType & 0x0F)
          {
            case SKYNET_CMD_QUERY:
            {
              //variables
              ///////////
              char                szDev[30];
              uint32_t            dwDev;
              uint32_t            dwScheduleID;
              uint32_t            dwMsgID;
              int                 nPos = 0;
              int                 nLen;
              int                 nDevType = 0;
              float               fLocN = 0.0;
              float               fLocE = 0.0;
              char                szLocOrientation[2];
              byte                bDevType[2];
  
              szLocOrientation[0] = 'N';
              bDevType[0] = DEVICE_TYPE_UNKNOWN;
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] QUERY_RESP"));
                Serial.print(F(" from: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" for: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" Origin ID: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
  
              if(msg.nDataLen > 0)
              {
                nLen = msg.pData[nPos++];
  
                memset(szDev, 0, sizeof(szDev));
                memcpy(szDev, pData + nPos, nLen);
  
                nPos += nLen;
  
                dwDev = ReadDWORD(msg.pData + nPos);
                nPos += sizeof(uint32_t);
  
                dwScheduleID = ReadDWORD(msg.pData + nPos);
                nPos += sizeof(uint32_t);
  
                nDevType = ReadINT(msg.pData + nPos);
                nPos += sizeof(int);
  
                #if SKYNET_CONN_DEBUG == 1
                  Serial.print(F("[CHandler] QUERY_RESP: devID: "));
                  Serial.print(dwDev);
                  Serial.print(F(" devtype: "));
                  Serial.print(nDevType);
                  Serial.print(F(" devName: "));
                  Serial.println(szDev);
                #endif
                
                //for me?
                if(msg.nReceiverID == DeviceConfig.dwDeviceID)
                {
                  //check if known by sender
                  if((dwDev > 0) && (DeviceConfig.dwDeviceID != dwDev))
                  {
                    insertIntoRoutingTable(dwDev, (dwDev == msg.nSenderID ? 0 : msg.nSenderID), (dwDev == msg.nSenderID ? msg.nHopCount : msg.nHopCount + 1), nDevType, this);
                  
                    //check if node is known by me
                    if(this->isKnownNode(dwDev) == false)
                    { 
                      bDevType[0] = nDevType & 0xFF;
                      
                      //insert entry into known nodes & request data from node          
                      this->insertOrUpdateIntoKnownNodes(dwDev, szDev, (byte*)&bDevType, fLocN, fLocE, (char*)&szLocOrientation, nRSSI, fPacketSNR); 
    
                      //send hello req to get the missing data
                      dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                      nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, dwDev, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                      
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(dwDev, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                    }
                    else
                    {
                      //dev is now known, remove schedule
                      if(dwScheduleID > 0)
                      {
                        //remove task
                        ((CSkyNetConnection*)this->m_pSkyNetConnection)->removeTaskSchedule(dwScheduleID, true);
                      };
                    };
                  }
                  else
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] QUERY_RESP: devID: "));
                      Serial.print(dwDev);
                      Serial.println(F(" is me"));
                    #endif
                  };
                }
                else
                {
                  if(msg.nViaID == DeviceConfig.dwDeviceID)
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] forward msg to: "));
                      Serial.println(msg.nReceiverID);
                    #endif
                    
                    nAnswerLen = PROTOCOL_MSG_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, !this->forwardMessage((_sSkyNetProtocolMessage*)&msg, true));
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                  };
                };
              };
            };
            break;
          };
        };
        break;
  
        case SKYNET_SUBCMD_IND:
        {
          switch(msg.bMsgType & 0x0F)
          {
            case SKYNET_CMD_DATA:
            {
              //variables
              ///////////
              uint32_t dwOldMsgID;
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] DATA_IND: "));
                Serial.print(F("Origin: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" Sender: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" Receiver: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.print(msg.nHopCount);
                Serial.print(F(" Payload DataLen: "));
                Serial.println(msg.nDataLen);
              #endif
  
              if((msg.nReceiverID == DeviceConfig.dwDeviceID) || (msg.nReceiverID == 0))
              {
                //don't answor messages, which are send to everyone
                //(position_ind), the sending dev doesn't expect them, or a response is
                //send by LoRaLink-Protocol itself...
                if(msg.nReceiverID != 0)
                {
                  nAnswerLen = DATA_CONF(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, SKYNET_RESPONSE_OK);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
                
                ((CSkyNetConnection*)this->m_pSkyNetConnection)->onLoRaLinkProtocolData((void*)&msg, msg.pData, msg.nDataLen);
              }
              else if(msg.nViaID == DeviceConfig.dwDeviceID)
              {
                #if SKYNET_CONN_DEBUG == 1
                  Serial.print(F("[CHandler] forward msg to: "));
                  Serial.println(msg.nReceiverID);
                #endif
                
                if(this->forwardMessage((_sSkyNetProtocolMessage*)&msg, true) == false)
                {
                  //send a query request...
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueQueryRequest(msg.nReceiverID, (char*)"", false, 0);
  
                  //send response
                  nAnswerLen = DATA_CONF(pAnswer, msg.dwMsgID, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, SKYNET_RESPONSE_NO_ROUTE);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                }
                else
                {
                  //send receive confirmation 
                  nAnswerLen = PROTOCOL_MSG_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, dwOldMsgID, false);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
              };
            };
            break;
            
            //informs all local nodes, that the sender is still there...
            //used to update the last heard flag, or to request a hello...
            case SKYNET_CMD_KEEPALIVE:
            {
              //variables
              ///////////
              CWSFFileDBRecordset rs(g_pNodeTable);
              bool bSendReq = false;
              uint32_t dwMsgID;
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] KEEPALIVE_IND: "));
                Serial.print(F("Origin: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" Sender: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" Receiver: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
  
  
              if(((msg.nReceiverID == 0) || (msg.nReceiverID == DeviceConfig.dwDeviceID)) && (msg.nSenderID != DeviceConfig.dwDeviceID))
              {
                //check if sender is in the routing & node table,
                //if not, request a hello_ind...
                if(this->isKnownNode(msg.nSenderID) == false)
                {             
                  #if SKYNET_CONN_INFO == 1
                    Serial.print(F("[CHandler] Node unknown, send HELLO_REQ: to: "));
                    Serial.println(msg.nSenderID);
                  #endif
  
                  //request a hello, routing & node will be written when the answer comes... 
                  dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                  nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                  
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, true);
                }
                else
                {
                  //update last heard data
                  updateLastHeard(msg.nSenderID, ClockPtr->getUnixTimestamp());
                };
              }
              else
              {
                #if SKYNET_CONN_INFO == 1
                  Serial.println(F("[CHandler] Msg Proto error"));
                #endif
              };
            };
            break;
            
            case SKYNET_CMD_HELLO:
            {
              //variables
              ///////////
              byte bDevType;
              char szName[30];
              float fLocN, fLocE;
              int nLen = 0, nPos = 0;
              uint32_t dwTime;
              char szLocOrientation[2];
              DateTime dtTime;
            
              memset(szName, 0, sizeof(szName));
              
              bDevType = msg.pData[nPos++];
              nLen = msg.pData[nPos++];
              
              memcpy(szName, msg.pData + nPos, nLen);
              nPos += nLen;
  
              fLocN = ReadFloat(msg.pData + nPos);
              nPos += sizeof(float);
  
              fLocE = ReadFloat(msg.pData + nPos);
              nPos += sizeof(float);
  
              szLocOrientation[0] = msg.pData[nPos++];
  
              dwTime = ReadDWORD(msg.pData + nPos);
              nPos += sizeof(uint32_t);
  
              //check if sender dev is configurred
              if((msg.nSenderID > 0) && (msg.nOriginID > 0) && (msg.nSenderID != DeviceConfig.dwDeviceID))
              {
                #if SKYNET_CONN_INFO == 1
                  Serial.print(F("[CHandler] HELLO_IND from: "));
                  Serial.print(szName);
                  Serial.print(F(" from: "));
                  Serial.print(msg.nSenderID);
                  Serial.print(F(" for: "));
                  Serial.print(msg.nReceiverID);
                  Serial.print(F(" Origin ID: "));
                  Serial.print(msg.nOriginID);
                  Serial.print(F(" via: "));
                  Serial.print(msg.nViaID);
                  Serial.print(F(" Hops: "));
                  Serial.print(msg.nHopCount);
                  Serial.print(F(" DevType: "));
                  Serial.println(bDevType);
                #endif
  
  
                //the device send it's time, if I don't have a valid time source,
                //set my time to the others devices time...
                nPos = ClockPtr->getUpdateSource(); 
                
                if((nPos == UPDATE_SOURCE_NONE) || (ClockPtr->getUpdateSource() == UPDATE_SOURCE_NETWORK))
                {
                  if(dwTime > 0)
                  {
                    dtTime = DateTime((time_t)dwTime);
      
                    ClockPtr->SetDateTime(dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), UPDATE_SOURCE_NETWORK);
  
                    #if SKYNET_CONN_INFO == 1
                      Serial.print(F("[CHandler] Update local time from NET, set to: "));
                      Serial.print(ClockPtr->unixTimestampToString(dwTime));
                      Serial.print(F(" / "));
                      Serial.println(dwTime);
                    #endif
    
                    //update schedules after reboot or initial time change
                    if(nPos == UPDATE_SOURCE_NONE)
                    {
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->getTaskScheduler()->rescheduleAfterTimechange();
                    };
                  }
                  else
                  {
                    #if SKYNET_CONN_INFO == 1
                      Serial.println(F("[CHandler] Dev send no time info"));
                    #endif
                  };
                };
                
  
                //write into node table
                this->insertOrUpdateIntoKnownNodes(msg.nSenderID, szName, (byte*)&bDevType, fLocN, fLocE, (char*)&szLocOrientation, nRSSI, fPacketSNR); 
  
                //check routing for the origin dev
                //when orig != sender and not DeviceConfig.dwDeviceID, it's a remote node,
                //otherwise a local (direct connected) node...  
                insertIntoRoutingTable(msg.nOriginID, (((msg.nOriginID == msg.nSenderID) || (msg.nReceiverID == 0)) ? 0 : msg.nSenderID), (msg.nOriginID == msg.nSenderID ? msg.nHopCount : msg.nHopCount + 1), (int)bDevType, this);
                
                //remote node, forwarded, if not known, put the sender into routing
                if(msg.nSenderID != msg.nOriginID)
                {
                  insertIntoRoutingTable(msg.nSenderID, 0, 0, (int)bDevType, this);
                };
  
                //check if we know the transport node...
                if((msg.nViaID != 0) && (msg.nViaID != DeviceConfig.dwDeviceID) && (msg.nViaID != msg.nSenderID))
                {
                  insertIntoRoutingTable(msg.nViaID, msg.nSenderID, 1, (int)bDevType, this);
                };


                //if the device is working as a bridge (has IP connections
                //we need to forward the HELLO, so that the rest of the
                //network is aware of that node...
                #if LORALINK_HARDWARE_LORA == 1
                  if((msg.nReceiverID == 0) && (this->getConnectionType() == SKYNET_CONN_TYPE_LORA))
                  {
                    #if SKYNET_CONN_INFO == 1
                      Serial.println(F("[CHandler] --> fwd HELLO_IND from LORA -> IP "));
                    #endif
                    
                    nAnswerLen    = HELLO_IND(pAnswer, msg.dwMsgID, msg.nSenderID, 0, msg.nSenderID, DeviceConfig.dwDeviceID, szName, (int)bDevType, fLocN, fLocE, szLocOrientation[0], msg.nHopCount + 1);
  
                    //forward to IP nodes
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsgForType(0, msg.dwMsgID, pAnswer, nAnswerLen, false, SKYNET_CONN_TYPE_IP_CLIENT, this->getTaskID());
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsgForType(0, msg.dwMsgID, pAnswer, nAnswerLen, false, SKYNET_CONN_TYPE_IP_SERVER, this->getTaskID());

                    #if SKYNET_CONN_INFO == 1
                      Serial.println(F("[CHandler] <-- fwd HELLO_IND from LORA -> IP "));
                    #endif
                  };
                #endif
                
                if((msg.nReceiverID == 0) && (((this->getConnectionType() == SKYNET_CONN_TYPE_IP_CLIENT) || (this->getConnectionType() == SKYNET_CONN_TYPE_IP_SERVER))))
                {
                  #if SKYNET_CONN_INFO == 1
                    Serial.println(F("[CHandler] fwd HELLO_IND from IP -> LORA, IP -> other IP"));
                  #endif
                  
                  nAnswerLen    = HELLO_IND(pAnswer, msg.dwMsgID, msg.nSenderID, 0, msg.nSenderID, DeviceConfig.dwDeviceID, szName, (int)bDevType, fLocN, fLocE, szLocOrientation[0], msg.nHopCount + 1);

                  //forward to LORA nodes
                  #if LORALINK_HARDWARE_LORA == 1
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsgForType(0, msg.dwMsgID, pAnswer, nAnswerLen, false, SKYNET_CONN_TYPE_LORA, this->getTaskID());
                  #endif

                  //forward to other IP Nodes
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsgForType(0, msg.dwMsgID, pAnswer, nAnswerLen, false, SKYNET_CONN_TYPE_IP_CLIENT, this->getTaskID());
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsgForType(0, msg.dwMsgID, pAnswer, nAnswerLen, false, SKYNET_CONN_TYPE_IP_SERVER, this->getTaskID());
                };
  
                
                //message for local devices or for me
                if((msg.nReceiverID == 0) || (msg.nReceiverID == DeviceConfig.dwDeviceID))
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.print(F("[CHandler] send (local) HELLO_CONF to: "));
                    Serial.print(msg.nOriginID);
                    Serial.print(F(" type: "));
                    Serial.print(this->m_nConnType);
                    Serial.print(F(" Hops: "));
                    Serial.println(msg.nHopCount);
                  #endif
      
                  //answer 
                  nAnswerLen = HELLO_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, SKYNET_RESPONSE_OK);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                }
                else
                {
                  if(msg.nViaID == DeviceConfig.dwDeviceID)
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] forward msg to: "));
                      Serial.println(msg.nReceiverID);
                    #endif
                    
                    if(this->forwardMessage((_sSkyNetProtocolMessage*)&msg, false) == false)
                    {
                      #if SKYNET_CONN_DEBUG == 1
                        Serial.print(F("[CHandler] unable forward msg to: "));
                        Serial.println(msg.nReceiverID);
                      #endif
                      
                      //send a query request...
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueQueryRequest(msg.nReceiverID, (char*)&szName, false, 0);
    
                      //send response
                      nAnswerLen = HELLO_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, SKYNET_RESPONSE_NO_ROUTE);
                      ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                    };    
                  }
                  else
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] ignore msg via: "));
                      Serial.println(msg.nViaID);
                    #endif
                  };
                };
              };
            };
            break;
          };
        };
        break;
  
        case SKYNET_SUBCMD_CONF:
        {
          switch(msg.bMsgType & 0x0F)
          {   
            case SKYNET_CMD_DATA:
            {
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] DATA_CONF: "));
                Serial.print(F("Origin: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" Sender: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" Receiver: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" MsgID: "));
                Serial.print(msg.dwMsgID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
              
              ((CSkyNetConnection*)this->m_pSkyNetConnection)->removeConfirmedMessage(msg.nSenderID, msg.dwMsgID);

              if(msg.nDataLen > 0)
              {
                #if SKYNET_CONN_INFO == 1
                  Serial.print(F("[CHandler] DATA_CONF: state: "));
                  Serial.println(msg.pData[0]);
                #endif

                if(msg.pData[0] == SKYNET_RESPONSE_NO_ROUTE)
                {
                  #if SKYNET_CONN_INFO == 1
                    Serial.print(F(" Receiver: "));
                    Serial.print(msg.nReceiverID);
                    Serial.print(F(" not reachable via: "));
                    Serial.println(msg.nViaID);
                  #endif

                  //remove the routing entry (L1 upwards)
                  RemoveRoutingEntry(msg.nReceiverID, msg.nViaID);
                };
              };
            };
            break;
  
            
            case SKYNET_CMD_PROTOCOL_MSG:
            {
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] PROTOCOL_MSG_CONF: "));
                Serial.print(F("Origin: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" Sender: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" Receiver: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" MsgID: "));
                Serial.print(msg.dwMsgID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
              
              if(msg.nReceiverID == DeviceConfig.dwDeviceID)
              {
                ((CSkyNetConnection*)this->m_pSkyNetConnection)->removeConfirmedMessage(msg.nSenderID, msg.dwMsgID);   
  
                if(msg.pData != NULL)
                {
                  if((bool)msg.pData[0] == true)
                  {
                    #if SKYNET_CONN_INFO == 1
                      Serial.println(F("[CHandler] PROTOCOL_MSG_CONF: failed"));
                    #endif
  
                    RemoveRoutingEntry(msg.nReceiverID, msg.nSenderID);
                  };
                };
              };
            };
            break;
  
            
            case SKYNET_CMD_HELLO:
            {
              //variables
              ///////////
              CWSFFileDBRecordset rs(g_pNodeTable);
              bool bSendReq = false;
              uint32_t dwMsgID;
              
              #if SKYNET_CONN_INFO == 1
                Serial.print(F("[CHandler] HELLO_CONF: "));
                Serial.print(F("Origin: "));
                Serial.print(msg.nOriginID);
                Serial.print(F(" Sender: "));
                Serial.print(msg.nSenderID);
                Serial.print(F(" Receiver: "));
                Serial.print(msg.nReceiverID);
                Serial.print(F(" via: "));
                Serial.print(msg.nViaID);
                Serial.print(F(" Hops: "));
                Serial.println(msg.nHopCount);
              #endif
  
              //for me?
              if(msg.nReceiverID == DeviceConfig.dwDeviceID)
              {
                #if SKYNET_CONN_DEBUG == 1
                  Serial.println(F("[CHandler] CONF for me... New State: Connected"));
                #endif
  
                this->m_nState          = SKYNET_CONN_STATE_CONNECTED;
                this->m_lKeepAliveTimer = millis() + SKYNET_CONN_TIMEOUT_KEEPALIVE;
  
                //check if in node table (has not send a HELLO_IND)
                if(this->isKnownNode(msg.nSenderID) == false)
                { 
                  bSendReq = true;
                };
  
                if(bSendReq == false)
                {
                  if((msg.nOriginID == msg.nSenderID) || (msg.nOriginID == 0))
                  {
                    //if the sender is not in the routing table (L0),
                    //send a request to send a HELLO_IND
                    if(FindRoutingEntry(msg.nSenderID, 0) == NULL)
                    {
                      bSendReq = true;
                    };

                    //set origin = sender,
                    //when orig is 0, avoid sending the 
                    //request to 0
                    msg.nOriginID = msg.nSenderID;
                  }
                  else
                  {
                    //if the sender is not in the routing database (L1),
                    //send a request to send a HELLO_IND
                    bSendReq = !isInRoutingDB(msg.nSenderID, msg.nOriginID);
                  };
                };
  
                if(bSendReq == true)
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.print(F("[CHandler] Type: "));
                    Serial.print(this->m_nConnType);
                    Serial.print(F(" TaskID: "));
                    Serial.print(this->getTaskID());
                    Serial.println(F(" - Node not known or in routing, send HELLO_REQ"));
                  #endif
  
                  dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                  nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, msg.nOriginID, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nOriginID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
              }
              else
              {
                //check conf for other devices, so that we can send a HELLO_REQ for new devices
                if(msg.nReceiverID != 0)
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.println(F("[CHandler] CONF for other dev..."));
                  #endif
  
                  if(this->isKnownNode(msg.nReceiverID) == false)
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] CONF for other unknown RX dev "));
                      Serial.print(msg.nReceiverID);
                      Serial.print(F(", request dev data over: "));
                      Serial.println(msg.nSenderID);
                    #endif
  
                    dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                    nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, msg.nReceiverID, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                    
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nReceiverID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                  };
                };
  
                if(DeviceConfig.dwDeviceID != msg.nSenderID)
                {
                  if(this->isKnownNode(msg.nSenderID) == false)
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] CONF for other unknown TX dev "));
                      Serial.print(msg.nReceiverID);
                      Serial.print(F(", request dev data over: "));
                      Serial.println(msg.nSenderID);
                    #endif
    
                    dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                    nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                    
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nReceiverID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                  };
                };
  
                if((DeviceConfig.dwDeviceID != msg.nViaID) && (msg.nSenderID != msg.nViaID) && (msg.nViaID != 0))
                {
                  if(this->isKnownNode(msg.nViaID) == false)
                  {
                    #if SKYNET_CONN_DEBUG == 1
                      Serial.print(F("[CHandler] CONF for other unknown VIA dev "));
                      Serial.print(msg.nViaID);
                      Serial.print(F(", request dev data over: "));
                      Serial.println(msg.nSenderID);
                    #endif
    
                    dwMsgID    = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
                    nAnswerLen = HELLO_REQ(pAnswer, DeviceConfig.dwDeviceID, msg.nViaID, DeviceConfig.dwDeviceID, msg.nSenderID, dwMsgID);
                    
                    ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nViaID, dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                  };
                };
  
                //check if we need to forward the conf
                if((msg.nViaID == DeviceConfig.dwDeviceID) && (msg.nViaID != msg.nReceiverID))
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.print(F("[CHandler] forward msg to: "));
                    Serial.println(msg.nReceiverID);
                  #endif

                  nAnswerLen = PROTOCOL_MSG_CONF(pAnswer, DeviceConfig.dwDeviceID, msg.nSenderID, msg.dwMsgID, !this->forwardMessage((_sSkyNetProtocolMessage*)&msg, true));
                  ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(msg.nSenderID, msg.dwMsgID, this->getTaskID(), this->m_nConnType, pAnswer, nAnswerLen, false);
                };
  
  
                if((this->m_nState == SKYNET_CONN_STATE_INIT) || (this->m_nState == SKYNET_CONN_STATE_CONNECTING))
                {
                  #if SKYNET_CONN_DEBUG == 1
                    Serial.println(F("[CHandler] CONF for dev, my State is still disconnected, send hello_ind"));
                  #endif
  
                  this->m_lStateTimer = millis();
                  this->m_nState      = SKYNET_CONN_STATE_INIT;
                };
              };
            };
            break;
          };
        };
        break;
      };
    }
    else
    {
      //sender not configured yet, ignore...
      #if SKYNET_CONN_DEBUG == 1
        Serial.print(F("[CHandler] Ignore unconfigured dev or own ID (RX = TX): "));
        Serial.print(msg.nOriginID);
        Serial.print(F(" sender ID: "));
        Serial.print(msg.nSenderID);
        Serial.print(F(" type: "));
        Serial.print(this->m_nConnType);
        Serial.print(F(" Hops: "));
        Serial.println(msg.nHopCount);
      #endif
  
      //in case of own id, I don't know, if this is a real reception
      //or a bug inside the library or modem...
    };
  }
  else
  {
    #if SKYNET_CONN_INFO == 1
      Serial.print(F("[CHandler] Type: "));
      Serial.print(this->m_nConnType);
      Serial.print(F(" TaskID: "));
      Serial.print(this->getTaskID());
      Serial.println(F(" - Error decode data"));
    #endif
  };

  
  if(msg.pData != NULL)
  {
    delete msg.pData;
  };

  delete pAnswer;

  #if SKYNET_CONN_DEBUG == 1
    Serial.print(F("<-- handleConnData("));
    Serial.print(this->getTaskID());
    Serial.println(F(")"));
  #endif
};


void CSkyNetConnectionHandler::handleTask()
{
  switch(this->m_nState)
  {
    case SKYNET_CONN_STATE_INIT:
    {
      //variables
      ///////////
      byte      bData[100];
      int       nLen;
      uint32_t  dwMsgID;

      if(this->m_lStateTimer < millis())
      {
        #if SKYNET_CONN_INFO == 1
          Serial.print(F("[CHandler]  Type: "));
          Serial.print(this->m_nConnType);
          Serial.print(F(" TaskID: "));
          Serial.print(this->getTaskID());
          Serial.print(F(", local time: "));
          Serial.print(ClockPtr->GetTimeString());
          Serial.println(F(" - State: INIT"));
        #endif

        //due to multithreading, avoid sending data, when the connection handler
        //is not populated to the connection...
        if((DeviceConfig.dwDeviceID > 0) && (((CSkyNetConnection*)this->m_pSkyNetConnection)->getHandlerByTaskID(this->getTaskID()) != NULL))
        {
          //send hello to everyone
          dwMsgID = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
          nLen    = HELLO_IND((byte*)&bData, dwMsgID, DeviceConfig.dwDeviceID, 0, DeviceConfig.dwDeviceID, DeviceConfig.dwDeviceID, DeviceConfig.szDevName, DeviceConfig.nDeviceType & 0xFF, DeviceConfig.fLocN, DeviceConfig.fLocE, DeviceConfig.cPosOrientation, 0);
    
          ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(0, dwMsgID, this->getTaskID(), this->m_nConnType, (byte*)&bData, nLen, false);

          //reset timers after HELLO
          this->m_nState          = SKYNET_CONN_STATE_CONNECTING;
          this->m_lSilenceTimer   = millis() + SKYNET_CONN_TIMEOUT_SILENCE; 
          this->m_lKeepAliveTimer = millis() + SKYNET_CONN_TIMEOUT_KEEPALIVE;
        };

        this->m_lStateTimer = millis() + SKYNET_CONN_TIMEOUT_CONNECT;
      };
    };
    break;


    case SKYNET_CONN_STATE_CONNECTING:
    {
      if(this->m_lStateTimer < millis())
      {
        #if SKYNET_CONN_INFO == 1
          Serial.print(F("[CHandler] Type: "));
          Serial.print(this->m_nConnType);
          Serial.print(F(" TaskID: "));
          Serial.print(this->getTaskID());
          Serial.println(F(" - CONNECT Timed out..."));
        #endif
  
        if(this->m_nConnType != SKYNET_CONN_TYPE_LORA)
        {
          if(++this->m_nReconnectCount >= 3)
          {
            #if SKYNET_CONN_INFO == 1
              Serial.println(F("[CHandler] close ip connection"));
            #endif
            
            ((CTCPClient*)this->m_pModemConnection)->closeConnection();
          }
          else
          {
            #if SKYNET_CONN_INFO == 1
              Serial.println(F("[CHandler] re-transmit..."));
            #endif

            this->m_nState      = SKYNET_CONN_STATE_INIT;
            this->m_lStateTimer = millis() + 1000;
          };
        }
        else
        {
          #if SKYNET_CONN_INFO == 1
            Serial.println(F("[CHandler] reconnect via LoRa"));
          #endif
          
          this->m_nState      = SKYNET_CONN_STATE_INIT;
          this->m_lStateTimer = millis();
        };
      };
    };
    break;


    case SKYNET_CONN_STATE_CONNECTED:
    {
      //variables
      ///////////
      byte                bData[100];
      int                 nLen;
      uint32_t            dwMsgID;
    
      if(this->m_lKeepAliveTimer < millis())
      {
        #if SKYNET_CONN_DEBUG == 1
          Serial.print(F("[CHandler] Type: "));
          Serial.print(this->m_nConnType);
          Serial.print(F(" TaskID: "));
          Serial.print(this->getTaskID());
          Serial.println(F(" - Send Keep alive..."));
        #endif

        dwMsgID = ((CSkyNetConnection*)this->m_pSkyNetConnection)->getMessageID();
        nLen    = KEEPALIVE_IND((byte*)&bData, dwMsgID, DeviceConfig.dwDeviceID);
  
        ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(0, dwMsgID, this->getTaskID(), this->m_nConnType, (byte*)&bData, nLen, false);

        this->m_lKeepAliveTimer = millis() + SKYNET_CONN_TIMEOUT_KEEPALIVE;
      };


      if(this->m_lSilenceTimer <= millis())
      {
        if(this->m_nConnType == SKYNET_CONN_TYPE_LORA)
        {
          #if SKYNET_CONN_DEBUG == 1
            Serial.print(F("[CHandler] Type: "));
            Serial.print(this->m_nConnType);
            Serial.print(F(" TaskID: "));
            Serial.print(this->getTaskID());
            Serial.println(F(" - No LoRa Data (INIT)"));
          #endif
          
          this->m_nState      = SKYNET_CONN_STATE_INIT;
          this->m_lStateTimer = 0;

          //remove lora entries
          RemoveRoutingEntriesByTaskID(this->getTaskID());
        }
        else
        {
          #if SKYNET_CONN_DEBUG == 1
            Serial.print(F("[CHandler] Type: "));
            Serial.print(this->m_nConnType);
            Serial.print(F(" TaskID: "));
            Serial.print(this->getTaskID());
            Serial.println(F(" - No Data (close IP conn)"));
          #endif

          ((CTCPClient*)this->m_pModemConnection)->closeConnection();

          this->m_nState      = SKYNET_CONN_STATE_INIT;
          this->m_lStateTimer = 0;
        };
      };
    };
    break;
  };
};







bool CSkyNetConnectionHandler::sendData(byte *pData, int nDataLength)
{
  #if SKYNET_CONN_INFO == 1
    Serial.print(F("[CHandler] Type: "));
    Serial.print(this->m_nConnType);
    Serial.print(F(" TaskID: "));
    Serial.print(this->getTaskID());
    Serial.println(F(" - send Data"));
  #endif
  
  return this->m_pModemConnection->sendData(pData, nDataLength);
};



bool CSkyNetConnectionHandler::isKnownNode(uint32_t dwNodeID)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(g_pNodeTable);
  
  //check if in node table
  return FindDeviceByNodeID(&rs, dwNodeID);
};







bool CSkyNetConnectionHandler::forwardMessage(void *pMessage, bool bConfirm)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage *pMsg = new _sSkyNetProtocolMessage;
  _sSkyNetRoutingEntry    route;
  byte                    *pData = new byte[SKYNET_PROTO_MAX_PACKET_LEN + 1];
  int                     nLen;
  
  //create a copy, so we don't mod the pointer, which will be used 
  //later...
  memcpy(pMsg, pMessage, sizeof(_sSkyNetProtocolMessage));

  //only when I am not sender, reciever, or origin
  if((DeviceConfig.dwDeviceID != pMsg->nOriginID) && (pMsg->nReceiverID != DeviceConfig.dwDeviceID) && (pMsg->nReceiverID != pMsg->nSenderID) && (pMsg->nReceiverID > 0))
  { 
    if(SearchBestMatchingRoute(pMsg->nReceiverID, (_sSkyNetRoutingEntry*)&route) == true)      
    {
      #if SKYNET_CONN_INFO == 1
        Serial.print(F("[CHandler] ForwardMessage: Type: "));
        Serial.print(this->m_nConnType);
        Serial.print(F(" TaskID: "));
        Serial.print(this->getTaskID());
        Serial.print(F(" - found route via: "));
        Serial.print(route.dwViaNode);
        Serial.print(F(" hop count "));
        Serial.print(route.dwHopCount);
        Serial.print(F(" for msg to: "));
        Serial.println(pMsg->nReceiverID);
      #endif

      //if the connection is not lora allow forwarding
      //only to other connections. LoRa connections
      //are store & forward, so we need to send messages
      //over the same radio connection.
      if(this->m_nConnType != SKYNET_CONN_TYPE_LORA)
      {
        //route over IP connection: check if data will be 
        //sent to the receiver over the same taskID, which is 
        //the same connection
        if(route.pConnHandler->getConnectionType() == this->m_nConnType)
        {
          if(route.pConnHandler->getTaskID() == this->getTaskID())
          {
            #if SKYNET_CONN_ERROR == 1
              Serial.println(F("[CHandler] Route error: routing loop detected (remove route)!"));
            #endif

            RemoveRoutingEntry(pMsg->nReceiverID, route.dwViaNode);

            delete pMsg;
            delete pData;

            //try again after removing the route...
            return this->forwardMessage(pMessage, bConfirm);
          };
        };
      };

     
      if(route.dwViaNode != pMsg->nSenderID)
      {
        pMsg->nHopCount  += 1;
        pMsg->nSenderID   = DeviceConfig.dwDeviceID;
        pMsg->nViaID      = route.dwViaNode;
        
        nLen = encodeSkyNetProtocolMessage(pMsg, pData);
        ((CSkyNetConnection*)this->m_pSkyNetConnection)->enqueueMsg(route.dwViaNode, pMsg->dwMsgID, route.pConnHandler->getTaskID(), route.pConnHandler->getConnectionType(), pData, nLen, bConfirm);

        delete pMsg;
        delete pData;
  
        return true;
      }
      else 
      {
        #if SKYNET_CONN_ERROR == 1
          Serial.println(F("[CHandler] Route error: new Via == Sender - failed to route packet"));
        #endif
      };
    };

    #if SKYNET_CONN_ERROR == 1
      Serial.print(F("[CHandler] Type: "));
      Serial.print(this->m_nConnType);
      Serial.print(F(" TaskID: "));
      Serial.print(this->getTaskID());
      Serial.println(F(" - failed to route packet, no data"));
    #endif
  }
  else
  {
    #if SKYNET_CONN_ERROR == 1
      Serial.print(F("[CHandler] Type: "));
      Serial.print(this->m_nConnType);
      Serial.print(F(" MsgID: "));
      Serial.print(pMsg->dwMsgID);
      Serial.print(F(" TaskID: "));
      Serial.print(this->getTaskID());
      Serial.println(F(" - failed to route packet, Orig = This Dev / TX = RX / RX = 0"));
    #endif
  };

  delete pMsg;
  delete pData;
  
  return false;
};





bool CSkyNetConnectionHandler::insertOrUpdateIntoKnownNodes(uint32_t dwNodeID, char *szName, byte *pDevType, float fLocN, float fLocE, char *szLocOrientation, int nRSSI, float fPacketSNR)
{
  //variables
  ///////////
  CWSFFileDBRecordset *rs;
  void *pInsert[NODETABLE_SIZE + 1];
  DateTime dtTime;
  uint32_t dwTime;
  bool bRes = false;

  if(dwNodeID != DeviceConfig.dwDeviceID)
  {
    rs = new CWSFFileDBRecordset(g_pNodeTable);
    
    //check if in node table
    if(FindDeviceByNodeID(rs, dwNodeID) == false)
    {
      //actual time
      dwTime = ClockPtr->getUnixTimestamp();

      //insert
      pInsert[0] = (void*)&dwNodeID;
      pInsert[1] = (void*)szName;
      pInsert[2] = (void*)pDevType;
      pInsert[3] = (void*)&fLocN;
      pInsert[4] = (void*)&fLocE;
      pInsert[5] = (void*)szLocOrientation;
      pInsert[6] = (void*)&dwTime;
      pInsert[7] = (void*)&nRSSI;
      pInsert[8] = (void*)&fPacketSNR;
      
      g_pNodeTable->insertData(pInsert);
  
      bRes = true;
    }
    else
    {
      //actual time
      dwTime = ClockPtr->getUnixTimestamp();
      
      //update last heard & name
      rs->setData(1, (void*)szName, strlen(szName) + 1);
      rs->setData(2, (void*)pDevType, 1);
      rs->setData(3, (void*)&fLocN, sizeof(float));
      rs->setData(4, (void*)&fLocE, sizeof(float));
      rs->setData(5, (void*)szLocOrientation, 1);
      rs->setData(6, (void*)&dwTime, sizeof(dwTime));
      rs->setData(7, (void*)&nRSSI, sizeof(nRSSI));
      rs->setData(8, (void*)&fPacketSNR, sizeof(fPacketSNR));
    };
  
    delete rs;
  };
  
  return bRes;
};
