//includes
//////////
#include "CLoRaLinkProtocol.h"
#include "HardwareConfig.h"
#include "LoRaLinkConfig.h"
#include "helper.h"
#include "CWSFClockHelper.h"
#include "CLoRaLinkDatabase.h"
#include "SystemState.h"
#include "CDBTaskQuery.h"
#include "SkyNetProtocol.h"
#include "CWebEvent.h"
#include "CSkyNetRouting.h"



int LLPROTO_Create(byte *pResult, byte *pPayload, int nPayloadLen, int nProtocolType, uint32_t dwSenderUserID)
{
  //variables
  ///////////
  int nPos = 0;

  pResult[nPos++] = nProtocolType;

  nPos            += WriteDWORD(pResult + nPos, dwSenderUserID);

  pResult[nPos++] = (byte)nPayloadLen & 0xFF;

  if(nPayloadLen > 0)
  {
    memcpy(pResult + nPos, pPayload, nPayloadLen);
    nPos += nPayloadLen;
  };
  
  return nPos;
};



bool LLPROTO_Decode(byte *pData, int nDataLen, byte *pPayload, int *pnPayloadLen, int *pnProtocolType, uint32_t *pdwSenderUserID)
{
  //variables
  ///////////
  int nPos = 0;

  if(nDataLen >= LLProtoMinHeaderLength)
  {
    *pnProtocolType                   = pData[nPos++];
        
    *pdwSenderUserID                  = ReadDWORD(pData + nPos);
    nPos += sizeof(uint32_t);
    
    *pnPayloadLen                     = pData[nPos++];

    if(*pnPayloadLen > 0)
    {
      memcpy(pPayload, pData + nPos, *pnPayloadLen);
      nPos += *pnPayloadLen;
    };
    
    return true;
  }
  else
  {
    #if CLRP_ERROR == 1
      Serial.println(F("LLPROTO_Decode: ERR: len < header size"));
    #endif
    
    return false;
  };
};


int  LLPROTO_CreateQueryUser(byte *pResult, char *szUser, uint32_t dwDeviceToQuery, uint32_t dwContactID, uint32_t dwTaskID)
{
  //variables
  ///////////
  int nPos = 0;

  pResult[nPos++] = (byte)strlen(szUser) & 0xFF;

  memcpy(pResult + nPos, szUser, pResult[0]);
  nPos += pResult[0];

  nPos += WriteDWORD(pResult + nPos, dwDeviceToQuery); 
  nPos += WriteDWORD(pResult + nPos, dwContactID); 
  nPos += WriteDWORD(pResult + nPos, dwTaskID); 

  #if CLRP_DEBUG == 1
    Serial.print(F("LLPROTO_CreateQueryUser: data len: "));
    Serial.print(nPos);
    Serial.print(F(" user: "));
    Serial.println(szUser);
  #endif
  
  return nPos;
};


bool LLPROTO_DecodeQueryUser(byte *pData, int nDataLen, char *szUser, uint32_t *dwDeviceToQuery, uint32_t *dwContactID, uint32_t *dwTaskID)
{
  //variables
  ///////////
  int nPos = 0;
  int nUserToQueryLen;
  
  if(nDataLen > LLProtoMinHeaderLength)
  { 
    nUserToQueryLen = pData[nPos++];

    #if CLRP_DEBUG == 1
      Serial.print(F("LLPROTO_DecodeQueryUser: user len: "));
      Serial.println(nUserToQueryLen);
    #endif
    
    memcpy(szUser, pData + nPos, nUserToQueryLen);
    nPos += nUserToQueryLen;
    
    *dwDeviceToQuery = ReadDWORD(pData + nPos);
    nPos += sizeof(uint32_t);
  
    *dwContactID = ReadDWORD(pData + nPos);
    nPos += sizeof(uint32_t);
    
    *dwTaskID = ReadDWORD(pData + nPos);
    nPos += sizeof(uint32_t);
  
    return true;
  }
  else
  {
    Serial.print(F("LLPROTO_DecodeQueryUser failed: len: "));
    Serial.println(nDataLen);

    return false;
  };
};




int  LLPROTO_CreateQueryUserResp(byte *pResult, uint32_t dwContactID, uint32_t dwLocalContactID, uint32_t dwTaskID, int nResult)
{
  //variables
  ///////////
  int nPos = 0;
  
  nPos += WriteDWORD(pResult + nPos, dwContactID); 
  nPos += WriteDWORD(pResult + nPos, dwLocalContactID); 
  nPos += WriteDWORD(pResult + nPos, dwTaskID); 
  nPos += WriteINT(pResult + nPos, nResult); 

  return nPos;
};


bool LLPROTO_DecodeQueryUserResp(byte *pData, int nDataLen, uint32_t *pdwContactID, uint32_t *pdwLocalContactID, uint32_t *pdwTaskID, int *pnResult)
{
  //variables
  ///////////
  int nPos = 0;

  *pdwContactID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);
  
  *pdwLocalContactID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pnResult = ReadINT(pData + nPos);
  nPos += sizeof(int);

  return true;
};



int  LLPROTO_CreateDataHeaderIndication(byte *pResult, char *szFilename, uint32_t dwLocalFileID, char *szSender, uint32_t dwSize, int nDataType, int nBlockSize, uint32_t dwTransferTaskID)
{
  //variables
  ///////////
  int nPos = 0;

  pResult[nPos++] = strlen(szFilename);
  memcpy(pResult + nPos, szFilename, pResult[nPos - 1]);
  nPos += strlen(szFilename);
  
  nPos += WriteDWORD(pResult + nPos, dwLocalFileID); 

  pResult[nPos++] = strlen(szSender);
  memcpy(pResult + nPos, szSender, pResult[nPos - 1]);
  nPos += strlen(szSender);

  nPos += WriteDWORD(pResult + nPos, dwSize); 
  nPos += WriteINT(pResult + nPos, nDataType); 
  nPos += WriteINT(pResult + nPos, nBlockSize); 
  nPos += WriteDWORD(pResult + nPos, dwTransferTaskID); 

  return nPos;
};



bool LLPROTO_DecodeDataHeaderIndication(byte *pData, int nDataLen, char *szFilename, uint32_t *pdwLocalFileID, char *szSender, uint32_t *pdwSize, int *pnDataType, int *pnBlockSize, uint32_t *pdwTransferTaskID)
{
  //variables
  ///////////
  int nPos = 0;
  int nLen = 0;
  
  memset(szSender, 0, 32);
  memset(szFilename, 0, 32);

  nLen = pData[nPos++];
  memcpy(szFilename, pData + nPos, nLen);
  nPos += nLen;

  *pdwLocalFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  nLen = pData[nPos++];
  memcpy(szSender, pData + nPos, nLen);
  nPos += nLen;

  *pdwSize = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pnDataType = ReadINT(pData + nPos);
  nPos += sizeof(int);

  *pnBlockSize = ReadINT(pData + nPos);
  nPos += sizeof(int);

  *pdwTransferTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  return true;
};


int  LLPROTO_CreateDataHeaderResp(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nRejected)
{
  //variables
  ///////////
  int nPos = 0; 

  nPos += WriteDWORD(pResult + nPos, dwLocalFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteTaskID); 
  nPos += WriteINT(pResult + nPos, nRejected); 

  return nPos;
};



bool LLPROTO_DecodeDataHeaderResp(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnRejected)
{
  //variables
  ///////////
  int nPos = 0;

  *pdwLocalFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pnRejected = ReadINT(pData + nPos);
  nPos += sizeof(int);

  return true;
};


int  LLPROTO_CreateDataReq(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nBlockNumber)
{
  //variables
  ///////////
  int nPos = 0; 

  nPos += WriteDWORD(pResult + nPos, dwLocalFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteTaskID); 
  nPos += WriteINT(pResult + nPos, nBlockNumber); 

  return nPos;
};


bool  LLPROTO_DecodeDataReq(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnBlockNumber)
{
  //variables
  ///////////
  int nPos = 0;

  *pdwLocalFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pnBlockNumber = ReadINT(pData + nPos);
  nPos += sizeof(int);

  return true;
};



int  LLPROTO_CreateDataResp(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nBlockNumber, byte *pData, int nDataLen, int nSuccess)
{
  //variables
  ///////////
  int nPos = 0; 

  nPos += WriteDWORD(pResult + nPos, dwLocalFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteTaskID); 
  nPos += WriteINT(pResult + nPos, nBlockNumber); 
  nPos += WriteINT(pResult + nPos, nSuccess);
  nPos += WriteINT(pResult + nPos, nDataLen);

  if(nDataLen > 0)
  {
    memcpy(pResult + nPos, pData, nDataLen);
    nPos += nDataLen;
  };

  return nPos;
};



bool LLPROTO_DecodeDataResp(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnBlockNumber, byte *pResultData, int *pnResultDataLen, int *pnSuccess)
{
  //variables
  ///////////
  int nPos = 0;

  *pdwLocalFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pnBlockNumber = ReadINT(pData + nPos);
  nPos += sizeof(int);

  *pnSuccess = ReadINT(pData + nPos);
  nPos += sizeof(int);

  *pnResultDataLen = ReadINT(pData + nPos);
  nPos += sizeof(int);

  if(*pnResultDataLen > 0)
  {
    memcpy(pResultData, pData + nPos, *pnResultDataLen);
  };

  return true;
};



int  LLPROTO_CreateDataComplete(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID)
{
  //variables
  ///////////
  int nPos = 0; 

  nPos += WriteDWORD(pResult + nPos, dwLocalFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteFileID); 
  nPos += WriteDWORD(pResult + nPos, dwRemoteTaskID);

  return nPos;
};


bool LLPROTO_DecodeDataComplete(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID)
{
  //variables
  ///////////
  int nPos = 0;

  *pdwLocalFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteFileID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  *pdwRemoteTaskID = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  return true;
};



int  LLPROTO_CreateShoutOutInfo(byte *pResult, char *szUser, uint32_t dwTimeSent, char *szMsg)
{
  //variables
  ///////////
  int nPos = 0; 
  
  pResult[nPos++] = strlen(szUser);

  memcpy(pResult + nPos, szUser, strlen(szUser));
  nPos += strlen(szUser);

  nPos += WriteDWORD(pResult + nPos, dwTimeSent);

  pResult[nPos++] = strlen(szMsg);
  
  memcpy(pResult + nPos, szMsg, strlen(szMsg));
  nPos += strlen(szMsg);

  return nPos;
};


bool LLPROTO_DecodeShoutOutInfo(byte *pData, int nDataLen, char *szUser, uint32_t *pdwTimeSent, char *szMsg)
{
  //variables
  ///////////
  int nPos = 1; 

  memcpy(szUser, pData + nPos, pData[nPos - 1]);
  nPos += pData[nPos - 1];

  *pdwTimeSent = ReadDWORD(pData + nPos);
  nPos += sizeof(uint32_t);

  memcpy(szMsg, pData + nPos + 1, pData[nPos]);
  nPos += pData[nPos] + 1;

  return true;
};


#if LORALINK_HARDWARE_GPS == 1
      
  int  LLPROTO_CreateUserPositionInd(byte *pResult, float fCourse, float fSpeed, int nHDOP, int nNumSat, uint32_t dwLastValid, float fLatitude, float fLongitude, float fAltitude, bool bValidSignal, int nPosType)
  {
    //variables
    ///////////
    int nPos    = 0; 
    int nValid  = (int)bValidSignal;
  
    #ifdef CLRP_DEBUG
      Serial.print(F("Create POS_IND: age: "));
      Serial.println(dwLastValid);
    #endif
    
    nPos += WriteFloat(pResult + nPos, fCourse);
    nPos += WriteFloat(pResult + nPos, fSpeed);
    nPos += WriteINT(pResult + nPos, nHDOP);
    nPos += WriteINT(pResult + nPos, nNumSat);
    nPos += WriteDWORD(pResult + nPos, dwLastValid);
    nPos += WriteFloat(pResult + nPos, fLatitude);
    nPos += WriteFloat(pResult + nPos, fLongitude);
    nPos += WriteFloat(pResult + nPos, fAltitude);
    nPos += WriteINT(pResult + nPos, nValid);
    nPos += WriteINT(pResult + nPos, nPosType);
  
    return nPos;
  };
  
  bool LLPROTO_DecodeUserPositionInd(byte *pData, int nDataLen, float *pfCourse, float *pfSpeed, int *pnHDOP, int *pnNumSat, uint32_t *pdwLastValid, float *pfLatitude, float *pfLongitude, float *pfAltitude, bool *pbValidSignal, int *pnPosType)
  {
    //variables
    ///////////
    int nPos = 0; 
  
    
    *pfCourse = ReadFloat(pData + nPos);
    nPos += sizeof(float);
  
    *pfSpeed = ReadFloat(pData + nPos);
    nPos += sizeof(float);
  
    *pnHDOP = ReadINT(pData + nPos);
    nPos += sizeof(int);
  
    *pnNumSat = ReadINT(pData + nPos);
    nPos += sizeof(int);
  
    *pdwLastValid = ReadDWORD(pData + nPos);
    nPos += sizeof(uint32_t);
  
    *pfLatitude = ReadFloat(pData + nPos);
    nPos += sizeof(float);
  
    *pfLongitude = ReadFloat(pData + nPos);
    nPos += sizeof(float);
  
    *pfAltitude = ReadFloat(pData + nPos);
    nPos += sizeof(float);
  
    *pbValidSignal = (bool)ReadINT(pData + nPos);
    nPos += sizeof(int);
  
    *pnPosType = (bool)ReadINT(pData + nPos);
    nPos += sizeof(int);
  
  
    #ifdef CLRP_DEBUG
      Serial.print(F("Decode POS_IND: age: "));
      Serial.println(*pdwLastValid);
    #endif
    
    return true;
  };


#endif


CLoRaLinkProtocol::CLoRaLinkProtocol(CSkyNetConnection *pSkyNetConnection, CDBTaskScheduler *pdbTaskScheduler)
{
  this->m_pSkyNetConnection = pSkyNetConnection;
  this->m_pdbTaskScheduler  = pdbTaskScheduler;
};


CLoRaLinkProtocol::~CLoRaLinkProtocol()
{
  
};



bool CLoRaLinkProtocol::handleLoRaLinkProtocolData(_sSkyNetProtocolMessage *pProtocolMsg, byte *pData, int nLength)
{
  //variables
  ///////////
  byte      *pPayload       = new byte[nLength + 1];
  int       nPayloadLen     = 0;
  int       nProtocolType   = 0;
  uint32_t  dwSenderUserID  = 0;
  bool      bResult         = false;
  

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] decode ProtoData: len: "));
    Serial.println(nLength);
  #endif
  
  if(LLPROTO_Decode(pData, nLength, pPayload, (int*)&nPayloadLen, (int*)&nProtocolType, (uint32_t*)&dwSenderUserID) == true)
  {
    #if CLRP_DEBUG == 1
      Serial.print(F("[CLRLP] received Data from: "));
      Serial.print(dwSenderUserID);
      Serial.print(F("@"));
      Serial.print(pProtocolMsg->nOriginID); 
      Serial.print(F(" proto type: "));
      Serial.println(nProtocolType); 
    #endif
    
    switch(nProtocolType)
    {
      #if LORALINK_HARDWARE_GPS == 1
      
        case LORA_PROTO_TYPE_USER_POSITION_IND:
        {
          //variables
          ///////////
          float               fCourse;
          float               fSpeed;
          int                 nHDOP;
          int                 nNumSat;
          uint32_t            dwLastValid;
          float               fLatitude;
          float               fLongitude;
          float               fAltitude;
          bool                bValidSignal;
          int                 nPosType;
          char                *szData;
          float               fDst = 0;
          float               fCourse2 = 0;
          uint32_t            dwTime = ClockPtr->getUnixTimestamp();
          CWSFFileDBRecordset *pRS;
                
          if(LLPROTO_DecodeUserPositionInd(pPayload, nPayloadLen, (float*)&fCourse, (float*)&fSpeed, (int*)&nHDOP, (int*)&nNumSat, (uint32_t*)&dwLastValid, (float*)&fLatitude, (float*)&fLongitude, (float*)&fAltitude, (bool*)&bValidSignal, (int*)&nPosType) == true)
          {
            #if CLRP_DEBUG == 1
              Serial.print(F("[CLRLP] POSITION_IND: from dev: "));
              Serial.print(pProtocolMsg->nOriginID);
              Serial.print(F(" type: "));
              Serial.println(nPosType);            
            #endif                    
  
            szData = new char[WEB_API_MAX_STRING_SIZE + 1];
            memset(szData, 0, WEB_API_MAX_STRING_SIZE);
  
            if(LLSystemState.bValidSignal == true)
            {
              fDst     = TinyGPS::distance_between(LLSystemState.fLatitude, LLSystemState.fLongitude, fLatitude, fLongitude);
              fCourse2 = TinyGPS::course_to(LLSystemState.fLatitude, LLSystemState.fLongitude, fLatitude, fLongitude);
            };
  
            sprintf_P(szData, PSTR("{\\\"Sender\\\": %lu, \\\"Course\\\": %f, \\\"Speed\\\": %f, \\\"HDOP\\\": %i, \\\"Sat\\\": %i, \\\"Age\\\": %lu, \\\"Lat\\\": %f, \\\"Lon\\\": %f, \\\"Alt\\\": %f, \\\"Valid\\\": %i, \\\"Type\\\": %i, \\\"Dst\\\": %f, \\\"Course2\\\": %f}"), pProtocolMsg->nOriginID, fCourse, fSpeed, nHDOP, nNumSat, dwLastValid, fLatitude, fLongitude, fAltitude, bValidSignal, nPosType, fDst, fCourse2);  
  
            #if CLRP_DEBUG == 1
              Serial.println(szData);
            #endif
  
            g_pWebEvent->addEventString(0, 7, szData);
  
            delete szData;
  
            //update position in node table
            if(bValidSignal == true)
            {
              pRS = new CWSFFileDBRecordset(g_pNodeTable);
              
              if(FindDeviceByNodeID(pRS, pProtocolMsg->nOriginID) == true)
              {
                pRS->setData(3, (void*)&fLatitude, sizeof(fLatitude));
                pRS->setData(4, (void*)&fLongitude, sizeof(fLongitude));
                pRS->setData(6, (void*)&dwTime, sizeof(dwTime));
              };

              delete pRS;
            };
          };
        };
        break;

      #endif
      
      case LORA_PROTO_TYPE_SHOUTOUT_IND:
      {
        //variables
        ///////////
        void               *pInsert[SHOUTOUTTABLE_SIZE + 1];
        char                szUser[70], szTemp[70];
        uint32_t            dwTime, dwTemp, dwReceiver;
        char                *szMsg = new char[301];
        CWSFFileDBRecordset *pRecordset;
        bool                bFound = false;
        int                 nSuccess;

        memset(szMsg, 0, 300);
        memset(szUser, 0, sizeof(szUser));
        
        if(LLPROTO_DecodeShoutOutInfo(pPayload, nPayloadLen, (char*)&szUser, (uint32_t*)&dwTime, szMsg) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] SHOUTOUT_IND: from dev: "));
            Serial.print(pProtocolMsg->nOriginID);
            Serial.print(F(" User: "));
            Serial.print(szUser);
            Serial.print(F(" Msg: "));
            Serial.print(szMsg);
            Serial.print(F(" time: "));
            Serial.println(dwTime);
          #endif
          

          limitShoutEntrys(DeviceConfig.nMaxShoutOutEntries);

          pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable);

          while(pRecordset->haveValidEntry() == true)
          {
            memset(szTemp, 0, sizeof(szTemp));
            
            pRecordset->getData(0, (void*)&szTemp, sizeof(szTemp));
            pRecordset->getData(1, (void*)&dwTemp, sizeof(dwTemp));

            if((strcasecmp(szTemp, szUser) == 0) && (dwTemp == dwTime))
            {
              bFound = true;
              break;
            };

            pRecordset->moveNext();
          };


          if(bFound == false)
          {
            nSuccess   = (this->enqueueShoutOut(pProtocolMsg->nOriginID, dwSenderUserID, (char*)&szUser, szMsg, dwTime) == true ? 1 : 0);
            
            pInsert[0] = (void*)&szUser;
            pInsert[1] = (void*)&dwTime;
            pInsert[2] = (void*)szMsg;
            pInsert[3] = (void*)&nSuccess;
            pInsert[4] = (void*)&pProtocolMsg->nOriginID;

            g_pShoutOutTable->insertData(pInsert);

            g_pWebEvent->addEvent(0, 5, g_pShoutOutTable->getLastInsertPos());
          };

          delete pRecordset;
        };

        delete szMsg;
      };
      break;

      case LORA_PROTO_TYPE_DATA_REQ:
      {
        //variables
        ///////////
        CWSFFileDB          *pDatabase;
        CWSFFileDBRecordset *pRecordset;
        byte                bData[DATATABLE_BLOCKSIZE + 1];
        int                 nBlockSize = DATATABLE_BLOCKSIZE;
        char                szDatabaseFile[55];
        uint32_t            dwLocalFileID;
        uint32_t            dwRemoteFileID; 
        uint32_t            dwRemoteTaskID;
        int                 nBlockNumber;
        int                 nSuccess;
        byte                *pDataAnswerPayload  = new byte[256];
        byte                *pDataAnswer         = new byte[256];
        int                 nPayloadLenAnswer    = 0;

        memset(pDataAnswerPayload, 0, 255);
        memset(pDataAnswer, 0, 255);
        
        if(LLPROTO_DecodeDataReq(pPayload, nPayloadLen, &dwLocalFileID, &dwRemoteFileID, &dwRemoteTaskID, &nBlockNumber) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] DATA_REQ: from dev: "));
            Serial.print(pProtocolMsg->nOriginID);
            Serial.print(F(" file: "));
            Serial.print(dwLocalFileID);
            Serial.print(F(" remote file: "));
            Serial.print(dwRemoteFileID);
            Serial.print(F(" block: "));
            Serial.println(nBlockNumber);
          #endif
          
          if(g_pDataTable->isOpen() == true)
          { 
            pRecordset = new CWSFFileDBRecordset(g_pDataTable);
            
            if(FindEntryForFileIdAndBlockNumber(pRecordset, dwRemoteFileID, nBlockNumber) == true)
            {
              pRecordset->getData(4, (void*)&bData, sizeof(bData));
              pRecordset->getData(3, (void*)&nBlockSize, sizeof(nBlockSize));

              //set transferred
              nPayloadLenAnswer = 1;
              pRecordset->setData(2, (void*)&nPayloadLenAnswer, sizeof(nPayloadLenAnswer));

              nSuccess          = 1;
              nPayloadLenAnswer = LLPROTO_CreateDataResp(pDataAnswerPayload, dwLocalFileID, dwRemoteFileID, dwRemoteTaskID, nBlockNumber, (byte*)&bData, nBlockSize, nSuccess);
            }
            else
            {
              nSuccess          = 0;
              nPayloadLenAnswer = LLPROTO_CreateDataResp(pDataAnswerPayload, dwLocalFileID, dwRemoteFileID, dwRemoteTaskID, nBlockNumber, NULL, nSuccess, nSuccess);
            };

            delete pRecordset;
          };

          //answer
          nSuccess          = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_DATA_RESP, dwSenderUserID);
          bResult           = this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nSuccess);
        }
        else
        {
          #if CLRP_ERROR == 1
            Serial.println(F("Unable to decode data req"));
          #endif
        };

        delete pDataAnswerPayload;
        delete pDataAnswer;
      };
      break;


      
      case LORA_PROTO_TYPE_DATA_RESP:
      {
        //variables
        ///////////
        CWSFFileDB          *pDatabase;
        CWSFFileDBRecordset *pRecordset;
        byte                *pData = new byte[DATATABLE_BLOCKSIZE + 1];
        int                 nBlockSize;
        char                szDatabaseFile[55];
        uint32_t            dwLocalFileID;
        uint32_t            dwRemoteFileID; 
        uint32_t            dwRemoteTaskID;
        int                 nBlockNumber;
        int                 nSuccess;       

        memset(pData, 0, DATATABLE_BLOCKSIZE);
        
        memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
        sprintf((char*)&szDatabaseFile, DATATABLE_FILE);

        if(LLPROTO_DecodeDataResp(pPayload, nPayloadLen, &dwLocalFileID, &dwRemoteFileID, &dwRemoteTaskID, &nBlockNumber, pData, &nBlockSize, &nSuccess) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] DATA_RESP: from dev: "));
            Serial.print(pProtocolMsg->nOriginID);
            Serial.print(F(" file: "));
            Serial.print(dwLocalFileID);
            Serial.print(F(" remote file: "));
            Serial.print(dwRemoteFileID);
            Serial.print(F(" block: "));
            Serial.print(nBlockNumber);
            Serial.print(F(" size: "));
            Serial.print(nBlockSize);
            Serial.print(F(" success: "));
            Serial.println(nSuccess);
          #endif
          
          if(nSuccess == 1)
          {
            if(g_pDataTable->isOpen() == true)
            { 
              pRecordset = new CWSFFileDBRecordset(g_pDataTable);
              
              if(FindEntryForFileIdAndBlockNumber(pRecordset, dwLocalFileID, nBlockNumber) == true)
              {
                pRecordset->setData(4, (void*)pData, DATATABLE_BLOCKSIZE);
                pRecordset->setData(3, (void*)&nBlockSize, sizeof(nBlockSize));
                pRecordset->setData(2, (void*)&nSuccess, sizeof(nSuccess));

                //manually invoke task to request next block
                this->m_pdbTaskScheduler->invokeTask(dwRemoteTaskID);
              }
              else
              {
                nSuccess = 0;
              };

              delete pRecordset;
            };
          };

          
          if(nSuccess == 0)
          {
            //data does not longer exist, or something else is missing
            //remove from database
            #if CLRP_ERROR == 1
              Serial.print(F("Failed to save block, success: "));
              Serial.println(nSuccess);
            #endif
            
            this->m_pdbTaskScheduler->removeSchedule(dwRemoteTaskID, false);

            removeDataTransferTablesHeader(dwLocalFileID);
          };
        };

        delete pData;
      };
      break;



      case LORA_PROTO_TYPE_DATA_COMPLETE_IND:
      {
        //variables
        ///////////
        CWSFFileDB          *pDatabase;
        CWSFFileDBRecordset *pRecordset;
        char                szDatabaseFile[55];
        uint32_t            dwReceiverDeviceID;
        uint32_t            dwLocalFileID;
        uint32_t            dwRemoteFileID;
        uint32_t            dwRemoteTaskID;
        uint32_t            dwLocalUserID        = 0;
        byte                *pDataAnswerPayload  = new byte[256];
        byte                *pDataAnswer         = new byte[256];
        int                 nPayloadLenAnswer    = 0;
        int                 nDataType            = 0;
        uint32_t            dwMessageID          = 0;
        int                 nRes;

        if(LLPROTO_DecodeDataComplete(pPayload, nPayloadLen, &dwLocalFileID, &dwRemoteFileID, &dwRemoteTaskID) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] DATA_COMPLETE_IND: from dev: "));
            Serial.print(pProtocolMsg->nOriginID);
            Serial.print(F(" file: "));
            Serial.print(dwLocalFileID);
            Serial.print(F(" remote file: "));
            Serial.print(dwRemoteFileID);
            Serial.print(F(" task: "));
            Serial.println(dwRemoteTaskID);
          #endif
    
          if(g_pDataHeaderTable->isOpen() == true)
          {
            pRecordset = new CWSFFileDBRecordset(g_pDataHeaderTable, dwRemoteFileID);

            if(pRecordset->haveValidEntry() == true)
            {
              //get corresponding message
              pRecordset->getData(7, (void*)&nDataType, sizeof(nDataType));
              pRecordset->getData(14, (void*)&dwMessageID, sizeof(dwMessageID));
              pRecordset->getData(15, (void*)&dwLocalUserID, sizeof(dwLocalUserID));
            };

            delete pRecordset;
          };

          if(nDataType == DATATABLE_DATATYPE_MSG)
          {
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, dwLocalUserID);
          
            pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, true, DBRESERVE_CHATMSG_COUNT);  
            pDatabase->open();
              
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase, dwMessageID);

              if(pRecordset->haveValidEntry() == true)
              {
                nDataType = 1;
                
                pRecordset->setData(2, (void*)&nDataType, sizeof(nDataType));

                #if CLRP_DEBUG == 1
                  Serial.print(F("[CLRLP] set complete, MsgID: "));
                  Serial.print(dwMessageID);
                  Serial.print(F(" user: "));
                  Serial.println(dwLocalUserID);
                #endif
              };

              delete pRecordset;
            };

            delete pDatabase;
          };

          nPayloadLenAnswer       = LLPROTO_CreateDataComplete(pDataAnswerPayload, dwLocalFileID, dwRemoteFileID, dwRemoteTaskID);
          nRes                    = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_DATA_COMPLETE_CONF, dwSenderUserID);
    
          this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nRes);

          removeDataTransferTablesHeader(dwRemoteFileID);
        };
        
        delete pDataAnswerPayload;
        delete pDataAnswer;
      };
      break;



      case LORA_PROTO_TYPE_DATA_COMPLETE_CONF:
      {
        //variables
        ///////////
        CWSFFileDB          *pDatabase;
        CWSFFileDBRecordset *pRecordset;
        char                szDatabaseFile[55];
        uint32_t            dwReceiverDeviceID;
        uint32_t            dwLocalFileID;
        uint32_t            dwRemoteFileID;
        uint32_t            dwRemoteTaskID;
        uint32_t            dwLocalUserID        = 0;
        int                 nDataType;
        uint32_t            dwMessageID          = 0;
        int                 nRes;

        memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
        sprintf((char*)&szDatabaseFile, DATAHEADERTABLE_FILE);

        
        if(LLPROTO_DecodeDataComplete(pPayload, nPayloadLen, &dwLocalFileID, &dwRemoteFileID, &dwRemoteTaskID) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] DATA_COMPLETE_CONF: from dev: "));
            Serial.print(pProtocolMsg->nOriginID);
            Serial.print(F(" file: "));
            Serial.print(dwLocalFileID);
            Serial.print(F(" remote file: "));
            Serial.print(dwRemoteFileID);
            Serial.print(F(" task: "));
            Serial.println(dwRemoteTaskID);
          #endif

          removeDataTransferTablesHeader(dwLocalFileID);

          this->m_pdbTaskScheduler->removeSchedule(dwRemoteTaskID, true);
        };
      };
      break;

      


      case LORA_PROTO_TYPE_DATA_HEADER:
      {
        //variables
        ///////////
        CWSFFileDB          *pDatabase;
        CWSFFileDBRecordset *pRecordset;
        uint32_t            dwTime                = ClockPtr->getUnixTimestamp();
        char                *szDatabaseFile       = new char[201];
        void                *pInsert[DATAHEADERTABLE_SIZE + 1];
        int                 nFileComplete = 0;
        int                 nBlock = 0;
        int                 nPos = 0;
        int                 nBlockSize            = DATATABLE_BLOCKSIZE;
        char                szFilename[34];
        char                szSender[34];
        uint32_t            dwRemoteFileID;
        uint32_t            dwFileSize;
        int                 nDataType             = DATATABLE_DATATYPE_MSG;
        int                 nTransferEnabled      = 1;
        int                 nTransferDirection    = DATATABLE_DATADIR_IN;
        int                 nHeaderConfirmed      = 1;
        uint32_t            dwDataHeadID          = 0;
        byte                *pDataAnswerPayload   = new byte[50];
        byte                *pDataAnswer          = new byte[80];
        int                 nPayloadLenAnswer     = 0;
        sDBTaskSendFile     task;
        uint32_t            dwRemoteTask;
        uint32_t            dwTransferID          = 0;
        uint32_t            dwLocalUserID         = 0;

        memset(szFilename, 0, sizeof(szFilename));
        memset(szSender, 0, sizeof(szSender));

        if(LLPROTO_DecodeDataHeaderIndication(pPayload, nPayloadLen, (char*)&szFilename, &dwRemoteFileID, (char*)&szSender, &dwFileSize, &nDataType, &nBlockSize, &dwRemoteTask) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] DATA_HEADER: from: "));
            Serial.print(szSender);
            Serial.print(F(" to/file "));
            Serial.print(szFilename);
            Serial.print(F(" size: "));
            Serial.println(dwFileSize);
          #endif

          if(nDataType == DATATABLE_DATATYPE_MSG)
          {
            dwLocalUserID = GetUserIdByName(szFilename);

            if(isContactBlocked(dwLocalUserID, dwSenderUserID, pProtocolMsg->nOriginID) == false)
            {
              //write header to db, request file
              if(g_pDataHeaderTable->isOpen() == true)
              {
                dwDataHeadID = getDataHeadIdBySourceAndFileId(g_pDataHeaderTable, pProtocolMsg->nOriginID, dwRemoteFileID);
              
                if(dwDataHeadID == 0)
                {
                  pInsert[0] = (void*)&szFilename;
                  pInsert[1] = (void*)&szSender;
                  pInsert[2] = (void*)&dwTime;
                  pInsert[3] = (void*)&dwFileSize;
                  pInsert[4] = (void*)&pProtocolMsg->nOriginID;
                  pInsert[5] = (void*)&pProtocolMsg->nOriginID;
                  pInsert[6] = (void*)&dwTime;
                  pInsert[7] = (void*)&nDataType;
                  pInsert[8] = (void*)&nFileComplete;
                  pInsert[9] = (void*)&dwRemoteFileID;
                  pInsert[10] = (void*)&nTransferEnabled;
                  pInsert[11] = (void*)&nTransferDirection;
                  pInsert[12] = (void*)&nHeaderConfirmed;
                  pInsert[13] = (void*)&nBlockSize;
                  pInsert[14] = (void*)&dwTransferID;
                  pInsert[15] = (void*)&dwSenderUserID;
                  
            
                  g_pDataHeaderTable->insertData(pInsert);
                  
                  dwDataHeadID = g_pDataHeaderTable->getLastInsertPos();

                  #if CLRP_DEBUG == 1
                    Serial.print(F("[CLRLP] inserted new file: "));
                    Serial.println(dwDataHeadID);
                  #endif
            
                  //insert empty data blocks  
                  if(g_pDataTable->isOpen() == true)
                  { 
                    while(nPos < dwFileSize)
                    {    
                      if((dwFileSize - nPos) < nBlockSize)
                      {
                        nBlockSize = dwFileSize - nPos;  
                      };
              
                      nFileComplete = 0;
              
                      pInsert[0] = (void*)&dwDataHeadID;
                      pInsert[1] = (void*)&nBlock;
                      pInsert[2] = (void*)&nFileComplete;
                      pInsert[3] = (void*)&nBlockSize;
                      pInsert[4] = NULL;
              
                      g_pDataTable->insertData(pInsert);

                      #if CLRP_DEBUG == 1
                        Serial.print(F("empty block saved to database: "));
                        Serial.print(nBlock);
                        Serial.print(F(" pos: "));
                        Serial.print(nPos);
                        Serial.print(F(" size: "));
                        Serial.println(nBlockSize);
                      #endif
                      
                      nPos   += nBlockSize;
                      nBlock += 1;
                    };

                    #if CLRP_DEBUG == 1
                      Serial.print(F("Blocks saved to database: "));
                      Serial.println(nBlock);
                    #endif
              
                    //answer
                    nPayloadLenAnswer = LLPROTO_CreateDataHeaderResp(pDataAnswerPayload, dwDataHeadID, dwRemoteFileID, dwRemoteTask, 0);
                    nPos              = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_DATA_HEADER_RESP, dwSenderUserID);
                    bResult           = this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nPos);
          
    
                    //create transfer complete task
                    task.dwFileID       = dwDataHeadID;
                    task.dwReleaseTask  = 0;
    
                    memset(szDatabaseFile, 0, 200);
                    memcpy(szDatabaseFile, &task, sizeof(sDBTaskSendFile));
    
                    //create transfer task
                    task.dwReleaseTask  = this->m_pdbTaskScheduler->addSchedule(DBTASK_CONFIRMTRANSFER, 60, 600, (byte*)szDatabaseFile, false);
      
                    memset(szDatabaseFile, 0, 200);
                    memcpy(szDatabaseFile, &task, sizeof(sDBTaskSendFile));
      
                    this->m_pdbTaskScheduler->addSchedule(DBTASK_REQUESTFILEDATA, 600, 600, (byte*)szDatabaseFile, true);
                  }
                  else
                  {
                    #if CLRP_ERROR == 1
                      Serial.println(F("Unable to open data db"));
                    #endif
              
                    removeDataTransferTablesHeader(dwDataHeadID);
              
                    dwDataHeadID = 0;
                  };
                }
                else
                {
                  #if CLRP_DEBUG == 1
                    Serial.println(F("Retransmit HEADERRESP"));
                  #endif
                  
                  //answer
                  nPayloadLenAnswer = LLPROTO_CreateDataHeaderResp(pDataAnswerPayload, dwDataHeadID, dwRemoteFileID, dwRemoteTask, 0);
                  nPos              = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_DATA_HEADER_RESP, dwSenderUserID);
                  bResult           = this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nPos);
                };
              }
              else
              {
                #if CLRP_ERROR == 1
                  Serial.println(F("Unable to open data header db"));
                #endif
            
                delete pDatabase;
              };
            }
            else 
            {
              #if CLRP_INFO == 1
                Serial.println(F("Reject file, sender blocked..."));
              #endif
              
              //answer (reject file!)
              nPayloadLenAnswer = LLPROTO_CreateDataHeaderResp(pDataAnswerPayload, dwDataHeadID, dwRemoteFileID, dwRemoteTask, 1);
              nPos              = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_DATA_HEADER_RESP, dwSenderUserID);
              bResult           = this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nPos);
            };
          }
          else
          {
            //not implemented yet
          };
        }
        else
        {
          #if CLRP_ERROR == 1
            Serial.println(F("Unable to decode data"));
          #endif
        };

        delete pDataAnswerPayload;
        delete pDataAnswer;
        delete szDatabaseFile;
      };
      break;


      case LORA_PROTO_TYPE_DATA_HEADER_RESP:
      {
        //variables
        ///////////
        CWSFFileDBRecordset *pRecordset;
        uint32_t            dwLocalFileID = 0; 
        uint32_t            dwRemoteFileID = 0;
        char                szDatabaseFile[50];
        uint32_t            dwTaskID;
        int                 nTransferred = 1;
        int                 nRejected = 0;
        
        if(LLPROTO_DecodeDataHeaderResp(pPayload, nPayloadLen, (uint32_t*)&dwLocalFileID, (uint32_t*)&dwRemoteFileID, (uint32_t*)&dwTaskID, (int*)&nRejected) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] received DATA_HEADER_RESP: remote file: "));
            Serial.print(dwLocalFileID);
            Serial.print(F(" local file: "));
            Serial.print(dwRemoteFileID);
            Serial.print(F(" rejected: "));
            Serial.print(nRejected);
            Serial.print(F(" Task ID: "));
            Serial.println(dwTaskID);
          #endif

          if(nRejected == 0)
          {
            if(g_pDataHeaderTable->isOpen() == true)
            { 
              pRecordset = new CWSFFileDBRecordset(g_pDataHeaderTable, dwRemoteFileID);
  
              if(pRecordset->haveValidEntry() == true)
              {
                //set header as transferred
                pRecordset->setData(12, (void*)&nTransferred, sizeof(nTransferred));
  
                if(dwTaskID > 0)
                {
                  //remove transmission schedule and wait for requests
                  this->m_pdbTaskScheduler->removeSchedule(dwTaskID, true);
                };
              }
              else
              {
                #if CLRP_ERROR == 1
                  Serial.println(F("Failed to set Header Transferred"));
                #endif
              };
  
              delete pRecordset;
            };
          }
          else
          {
            #if CLRP_INFO == 1
              Serial.println(F("Message rejected by receiver!"));
            #endif

            //remove transmission schedule (will also remove file data)
            this->m_pdbTaskScheduler->removeSchedule(dwTaskID, false);
          };
        };
      };
      break;
      
  
      case LORA_PROTO_TYPE_USERQ_REQ:
      {
        //variables
        ///////////
        char                   szUser[26];
        uint32_t               dwDeviceToQuery      = 0;
        uint32_t               dwContactID          = 0;
        uint32_t               dwTaskID             = 0;
        CWSFFileDBRecordset    rs(g_pUserTable);
        int                    nRes                 = 1;
        uint32_t               dwRespID             = 0;
        byte                   *pDataAnswerPayload  = new byte[50];
        byte                   *pDataAnswer         = new byte[80];
        int                    nPayloadLenAnswer    = 0;
        
        memset(szUser, 0, sizeof(szUser));
  
        if(LLPROTO_DecodeQueryUser(pPayload, nPayloadLen, (char*)&szUser, (uint32_t*)&dwDeviceToQuery, (uint32_t*)&dwContactID, (uint32_t*)&dwTaskID) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[LRLP] received UserQueryReq for User: "));
            Serial.print(szUser);
            Serial.print(F(", remote contact: "));
            Serial.print(dwContactID);
            Serial.print(F(" from "));
            Serial.print(dwSenderUserID);
            Serial.print(F("@"));
            Serial.println(pProtocolMsg->nOriginID); 
          #endif
        
          if(FindUserByName(&rs, szUser) == true)
          {
            dwRespID  = rs.getRecordPos();
            nRes      = 2;
          };
    
          nPayloadLenAnswer       = LLPROTO_CreateQueryUserResp(pDataAnswerPayload, dwRespID, dwContactID, dwTaskID, nRes);
          nRes                    = LLPROTO_Create(pDataAnswer, pDataAnswerPayload, nPayloadLenAnswer, LORA_PROTO_TYPE_USERQ_RESP, dwSenderUserID);
    
          bResult                 = this->enqueueLoRaLinkMessage(pProtocolMsg->nOriginID, pDataAnswer, nRes);
        };
        
        delete pDataAnswerPayload;
        delete pDataAnswer;
      };
      break;
  
  
      case LORA_PROTO_TYPE_USERQ_RESP:
      {
        //variables
        ///////////
        uint32_t                  dwContactID;
        uint32_t                  dwLocalContactID;
        uint32_t                  dwTaskID; 
        int                       nResult;
        CWSFFileDB                *pDatabase;
        CWSFFileDBRecordset       *pRecordset;
        char                      szDatabaseFile[55];

        if(LLPROTO_DecodeQueryUserResp(pPayload, nPayloadLen, (uint32_t*)&dwContactID, (uint32_t*)&dwLocalContactID, (uint32_t*)&dwTaskID, (int*)&nResult) == true)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] USERQ_RESP: result: "));
            Serial.print(nResult);
            Serial.print(F(" for contact: "));
            Serial.print(dwLocalContactID);
            Serial.print(F(" for local user: "));
            Serial.print(dwSenderUserID);
            Serial.print(F(" Task: "));
            Serial.print(dwTaskID);
            Serial.print(F(" Result: "));
            Serial.print(nResult);
          #endif
    
          memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
          sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, dwSenderUserID);
    
          pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);  
          pDatabase->open();
            
          if(pDatabase->isOpen() == true)
          {   
            //success
            #if CLRP_DEBUG == 1
              Serial.print(F("[CLRLP] USERQ_RESP: opened user contacts db: "));
              Serial.println(szDatabaseFile);
            #endif
            
            pRecordset = new CWSFFileDBRecordset(pDatabase, dwLocalContactID);
    
            if(pRecordset->haveValidEntry() == true)
            {     
              if(nResult != 2)
              {
                //failed
                #if CLRP_ERROR == 1
                  Serial.println(F("[CLRLP] USERQ_RESP: query failed, user does not exist"));
                #endif
    
                pRecordset->setData(4, (void*)&nResult, sizeof(nResult));
              }
              else
              {
                #if CLRP_DEBUG == 1
                  Serial.print(F("[CLRLP] USERQ_RESP: query OK, update contact: "));
                  Serial.println(dwContactID);
                #endif
            
                pRecordset->setData(0, (void*)&dwContactID, sizeof(dwContactID));
                pRecordset->setData(4, (void*)&nResult, sizeof(nResult));
              };
            };
    
            delete pRecordset;
            bResult = true;
          }
          else
          {
            bResult = false;
          };
    
          delete pDatabase;
          
          //remove the task
          if(dwTaskID > 0)
          {
            this->m_pdbTaskScheduler->removeSchedule(dwTaskID, bResult);
          };
        }
        else
        {
          #if CLRP_ERROR == 1
            Serial.println(F("[CLRLP] USERQ_RESP: ERROR decode"));
          #endif
        };
      };
      break;
    };

    delete pPayload;
    
    return bResult;
  }
  else
  {
    #if CLRP_ERROR == 1
      Serial.println(F("[CLRLP] ERR: unable to decode protocol data"));
    #endif

    delete pPayload;
    
    return false;
  };
};



void CLoRaLinkProtocol::handleTask()
{
  //nothing to do here yet...
};


void CLoRaLinkProtocol::enqueueQueryRequest(uint32_t dwNodeID, char *szNodeName, bool bForward, uint32_t dwScheduleID)
{
  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] query dev: "));
    Serial.println(szNodeName);
  #endif
  
  this->m_pSkyNetConnection->enqueueQueryRequest(dwNodeID, szNodeName, bForward, dwScheduleID);
};



bool CLoRaLinkProtocol::addShoutOut(uint32_t dwSenderNodeID, uint32_t dwUserID, String strUser, String strDev, char *szMsg)
{
  //variables
  ///////////
  void                *pInsert[SHOUTOUTTABLE_SIZE + 1];
  char                szUser[70];
  uint32_t            dwTime               = ClockPtr->getUnixTimestamp();
  uint32_t            dwRes;
  bool                bSend;
  int                 nTransmitted;
  
  memset(szUser, 0, sizeof(szUser));
  sprintf_P(szUser, PSTR("%s@%s"), (char*)strUser.c_str(), (char*)strDev.c_str());

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] addShoutOut() from: "));
    Serial.println(szUser);
  #endif

  bSend = this->enqueueShoutOut(dwSenderNodeID, dwUserID, szUser, szMsg, dwTime);

  //the message will be retransmitted
  //later if not forwared to at least 1 node
  nTransmitted = (bSend == true ? 1 : 0);

  pInsert[0] = (void*)&szUser;
  pInsert[1] = (void*)&dwTime;
  pInsert[2] = (void*)szMsg;
  pInsert[3] = (void*)&nTransmitted;
  pInsert[4] = (void*)&dwSenderNodeID;

  g_pShoutOutTable->insertData(pInsert);
  dwRes      = g_pShoutOutTable->getLastInsertPos();

  g_pWebEvent->addEvent(dwUserID, 6, dwRes);

  return true;
};


#if LORALINK_HARDWARE_GPS == 1

  bool CLoRaLinkProtocol::addPosition(uint32_t dwRemoteDevID, uint32_t dwLocalUserID, int nPositionType)
  {
      //variables
    ///////////
    byte                  *pDataAnswer         = new byte[256];
    byte                  *pPayload            = new byte[256];
    int                   nPayloadLen          = 0;
    int                   nSuccess;
    uint32_t              dwMsgID              = this->m_pSkyNetConnection->getMessageID();
    
  
    nPayloadLen       = LLPROTO_CreateUserPositionInd(pPayload, LLSystemState.fCourse, LLSystemState.fSpeed, LLSystemState.nHDOP, LLSystemState.nNumSat, (LLSystemState.dwLastValid > 0 ? (millis() - LLSystemState.dwLastValid) / 1000 : 0), LLSystemState.fLatitude, LLSystemState.fLongitude, LLSystemState.fAltitude, LLSystemState.bValidSignal, nPositionType);
    nSuccess          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_USER_POSITION_IND, dwLocalUserID);
  
  
    nPayloadLen       = DATA_IND(pPayload, dwMsgID, DeviceConfig.dwDeviceID, dwRemoteDevID, DeviceConfig.dwDeviceID, 0, pDataAnswer, nSuccess);
    
    //enqueue only via LoRa, the device must be able to communicate directly
    //this is a privacy / security restriction, since (except EMERG) positions
    //are not routed through the entire network
    this->m_pSkyNetConnection->enqueueMsgForType(dwRemoteDevID, dwMsgID, pPayload, nPayloadLen, false, SKYNET_CONN_TYPE_LORA, 0); 
  
  
    delete pDataAnswer;
    delete pPayload;
  
    return true;
  };

#endif




bool CLoRaLinkProtocol::enqueueShoutOut(uint32_t dwSenderNodeID, uint32_t dwSenderUserID, char *szUser, char *szMsg, uint32_t dwTime)
{
  //variables
  ///////////
  byte                  *pDataAnswer         = new byte[256];
  byte                  *pPayload            = new byte[256];
  int                   nPayloadLen          = 0;
  int                   nSuccess;
  bool                  bForwarded           = false;
  _sListItem            *item                = g_pRoutingEntries->getList();
  _sSkyNetRoutingEntry  *routing; 
  
  nPayloadLen       = LLPROTO_CreateShoutOutInfo(pPayload, szUser, dwTime, szMsg);
  nSuccess          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_SHOUTOUT_IND, dwSenderUserID);

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if(routing->pConnHandler->getConnectionType() == SKYNET_CONN_TYPE_LORA)
        {
          //avoid sending back and nodes which have a transport node set
          if((dwSenderNodeID != routing->dwDeviceID) && (routing->dwViaNode != dwSenderNodeID) && (routing->dwViaNode == 0))
          {
            #if CLRP_DEBUG == 1
              Serial.print(F("[CLRLP] SHOUTOUT_IND: TX to: "));
              Serial.print(routing->dwDeviceID);
              Serial.print(F(" from: "));
              Serial.print(szUser);
              Serial.print(F(" msg: "));
              Serial.println(szMsg);
            #endif
            
            if(this->enqueueLoRaLinkMessage(routing->dwDeviceID, pDataAnswer, nSuccess) == true)
            {
              bForwarded = true;
            };
          };
        }
        else
        {
          //mark as forwarded, when route follows back
          if((dwSenderNodeID == routing->dwDeviceID) || (routing->dwViaNode == dwSenderNodeID))
          {
            #if CLRP_DEBUG == 1
              Serial.println(F("[CLRLP] SHOUTOUT over IP, dont fwd over IP"));
            #endif
    
            bForwarded = true;
          }
          else
          {
            if(routing->dwViaNode == 0)
            {
              #if CLRP_DEBUG == 1
                Serial.print(F("[CLRLP] SHOUTOUT_IND: TX to: "));
                Serial.print(routing->dwDeviceID);
                Serial.print(F(" from: "));
                Serial.print(szUser);
                Serial.print(F(" msg: "));
                Serial.println(szMsg);
              #endif
              
              if(this->enqueueLoRaLinkMessage(routing->dwDeviceID, pDataAnswer, nSuccess) == true)
              {
                bForwarded = true;
              };
            };
          };
        };
      };
      
      item = item->pNext;
    };
  };

  delete pDataAnswer;
  delete pPayload;

  return bForwarded;
};


bool CLoRaLinkProtocol::addMessage(uint32_t dwLocalUserID, String strUser, String strDev, char *szMsg, uint32_t dwDevID, uint32_t dwUsrID, uint32_t dwContID, int nDirection)
{
  //variables
  ///////////
  uint32_t            dwTemp, dwTime;
  CWSFFileDB          *pDatabase;
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[201];
  bool                bContactFound = false;
  bool                bSearchNodes = false;
  void                *pInsert[CHATHEADTABLE_SIZE + 1];
  uint32_t            dwChatHeadID;
  uint32_t            dwMessageID;
  int                 nChatType = CHAT_TYPE_MESSAGE;
  int                 nTemp;
  int                 nPos;
  int                 nBlock;
  int                 nUnreadMsgs;
  char                szUser[26];
  char                szDevice[26];
  char                szSender[26];
  bool                bIsLocal = false;
  uint32_t            dwMsgTask = 0;
  int                 nMsgSent = 0;
  int                 nMsgRead = 0;
  uint32_t            dwTransferTaskID = 0;
  uint32_t            dwTransferID = 0;
  bool                bMsgCreated = false;

  memset(szUser, 0, sizeof(szUser));
  memset(szDevice, 0, sizeof(szDevice));
  memset(szSender, 0, sizeof(szSender));

  sprintf(szUser, "%s\0", strUser.c_str());
  sprintf(szDevice, "%s\0", strDev.c_str());
  GetUserNameByID(dwLocalUserID, (char*)&szSender, sizeof(szSender));

  #if CLRP_DEBUG == 1
    Serial.print(F("Start Msg to: "));
    Serial.print(strUser);
    Serial.print(F("@"));
    Serial.print(strDev);
    Serial.print(F(" from: "));
    Serial.print(szSender);
    Serial.print(F(" - dir: "));
    Serial.print(nDirection);
    Serial.print(F(" size: "));
    Serial.println(strlen(szMsg));
  #endif

  if((strUser.length() > 0) && (strDev.length() > 0))
  {
    //check if local dev
    if(strcasecmp((char*)strDev.c_str(), DeviceConfig.szDevName) == 0)
    {
      bIsLocal = true;
      dwDevID  = DeviceConfig.dwDeviceID;
      dwUsrID  = GetUserIdByName((char*)strUser.c_str());

      if(dwUsrID == 0)
      {
        #if CLRP_ERROR == 1
          Serial.print(F("Failed to lookup local user: "));
          Serial.println(strUser);
        #endif
        
        return false;
      };
    };

    //check if dev is known remote node
    if(dwDevID == 0)
    {
      dwDevID = GetNodeIdByName((char*)strDev.c_str());

      if(dwDevID > 0)
      {
        #if CLRP_DEBUG == 1
          Serial.print(F("Found Dev in nodes, set to: "));
          Serial.println(dwDevID);
        #endif
      };
    };

    ResetWatchDog();

    //check if transmission to known contact
    //if not, create contact
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, dwLocalUserID);
    
    pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);  
    pDatabase->open();
    
    if(pDatabase->isOpen() == true)
    {
      #if CLRP_DEBUG == 1
        Serial.print(F("Lookup contact: "));
      #endif
      
      pRecordset = new CWSFFileDBRecordset(pDatabase);

      //check personal contacts
      if(FindContactByName(pRecordset, (char*)strUser.c_str(), (char*)strDev.c_str()) == true)
      {
        #if CLRP_DEBUG == 1
          Serial.print(F(" Found, "));
        #endif
        
        bContactFound = true;

        //check if devid is unknown
        if(dwDevID == 0)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F(" DevID: 0, "));
          #endif
          
          pRecordset->getData(1, (void*)&dwDevID, sizeof(dwDevID));

          if(dwDevID == 0)
          {
            //send node query
            bSearchNodes = true;
          };
        }
        else
        {
          //check if we can update contact
          pRecordset->getData(1, (void*)&dwTemp, sizeof(dwTemp));

          if(dwTemp == 0)
          {
            #if CLRP_DEBUG == 1
              Serial.print(F(" update dev ID, "));
            #endif
            
            pRecordset->setData(1, (void*)&dwDevID, sizeof(dwDevID));
          };
        };

        //check if usrID == 0
        if(dwUsrID == 0)
        {
          #if CLRP_DEBUG == 1
            Serial.print(F(" UsrID: 0, "));
          #endif
          
          pRecordset->getData(0, (void*)&dwUsrID, sizeof(dwUsrID));
        }
        else
        {
          pRecordset->getData(0, (void*)&dwTemp, sizeof(dwTemp));
          
          if(dwTemp == 0)
          {
            #if CLRP_DEBUG == 1
              Serial.print(F(" set user ID, "));
            #endif
                        
            pRecordset->setData(0, (void*)&dwUsrID, sizeof(dwUsrID));
          };
        };

        if(dwContID == 0)
        {
          dwContID = pRecordset->getRecordPos();

          #if CLRP_DEBUG == 1
            Serial.print(F(" - ContactPos: "));
            Serial.print(dwContID);
          #endif
        };
      }
      else
      {
        bSearchNodes = true;
        #if CLRP_DEBUG == 1
          Serial.print(F(" not found, "));
        #endif
      };

      delete pRecordset;
    }
    else
    {
      #if CLRP_DEBUG == 1
        Serial.print(F(" not found (DB empty), "));
      #endif
      
      bSearchNodes = true;
    };

    delete pDatabase;

    Serial.println();

    ResetWatchDog();

    //search node table
    if((bSearchNodes == true) && (dwDevID == 0))
    {
      dwDevID = GetNodeIdByName((char*)strDev.c_str());
    };

    if(bContactFound == false)
    {
      //insert contact (if local, set state to 2
      dwContID = InsertContactForUser(dwLocalUserID, dwUsrID, dwDevID, (char*)&szUser, (char*)&szDevice, (dwUsrID > 0 && dwDevID > 0 ? 2 : 0));
    };


    //at this point we should have everything which can be 
    //aquirred without the network, write data...
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, dwLocalUserID);
  
    pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, true, DBRESERVE_CHAT_COUNT);  
    pDatabase->open();
      
    if(pDatabase->isOpen() == true)
    {
      pRecordset = new CWSFFileDBRecordset(pDatabase);
      
      if(FindChatForContactID(pRecordset, dwContID) == true)
      {
        dwChatHeadID = pRecordset->getRecordPos();
        pRecordset->getData(5, (void*)&nUnreadMsgs, sizeof(nUnreadMsgs));

        #if CLRP_DEBUG == 1
          Serial.print(F("append to chat: "));
          Serial.print(dwChatHeadID);
          Serial.print(F(" to: "));
          Serial.println(szDatabaseFile);
        #endif
      }
      else
      {
        dwTime      = ClockPtr->getUnixTimestamp();
        dwMessageID = 0;
        nUnreadMsgs = 0;

        memset(szDatabaseFile, 0, sizeof(szDatabaseFile));
        sprintf(szDatabaseFile, "%s@%s\0", strUser.c_str(), strDev.c_str());


        pInsert[0] = (void*)&nChatType;
        pInsert[1] = (void*)&szDatabaseFile;
        pInsert[2] = (void*)&dwTime; 
        pInsert[3] = (void*)&dwMessageID;
        pInsert[4] = (void*)&dwTime; 
        pInsert[5] = (void*)&nUnreadMsgs; 
        pInsert[6] = (void*)&dwContID;

        pDatabase->insertData(pInsert);
        dwChatHeadID = pDatabase->getLastInsertPos();

        #if CLRP_DEBUG == 1
          Serial.print(F("inserted new chat: "));
          Serial.print(dwChatHeadID);
          Serial.print(F(" to: "));
          Serial.println(szDatabaseFile);
        #endif

        bMsgCreated = true;
      };  
    }
    else
    {
      #if CLRP_ERROR == 1
        Serial.println(F("Unable to open chat head db"));
      #endif

      delete pRecordset;
      delete pDatabase;

      return false;
    };

    delete pRecordset;
    delete pDatabase;


    ResetWatchDog();

    //create chat message entry for sender
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, dwLocalUserID);
  
    pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, true, DBRESERVE_CHATMSG_COUNT);  
    pDatabase->open();
      
    if(pDatabase->isOpen() == true)
    {
      nPos        = strlen(szMsg);
      nTemp       = (bIsLocal == true ? 1 : 0); //tx complete
      dwTime      = ClockPtr->getUnixTimestamp();

      #if CLRP_DEBUG == 1
        Serial.print(F("Insert Msg to "));
        Serial.print(szDatabaseFile);
        Serial.print(F(" chID: "));
        Serial.print(dwChatHeadID);
        Serial.print(F(" size: "));
        Serial.print(nPos);
        Serial.print(F(" local msg: "));
        Serial.print(bIsLocal);
        Serial.print(F(" from: "));
        Serial.print(dwLocalUserID);
        Serial.print(F(" to: "));
        Serial.print(strUser);
        Serial.print(F(" dir: "));
        Serial.println(nDirection);
      #endif

      nMsgSent = (bIsLocal == true ? 1 : 0);
  
      pInsert[0] = (void*)&dwChatHeadID;
      pInsert[1] = (void*)&nPos;
      pInsert[2] = (void*)&nMsgSent;
      pInsert[3] = (void*)&nDirection;
      pInsert[4] = (void*)&dwTime;
      pInsert[5] = (void*)&dwContID;
      pInsert[6] = (void*)szMsg;
      pInsert[7] = (void*)&nMsgRead;

      pDatabase->insertData(pInsert);
      dwMessageID = pDatabase->getLastInsertPos();

      #if CLRP_DEBUG == 1
        Serial.print(F("inserted new chat message: "));
        Serial.println(dwMessageID);
      #endif
    }
    else
    {
      #if CLRP_ERROR == 1
        Serial.println(F("Unable to open chat msg db"));
      #endif
      
      delete pDatabase;

      if(bMsgCreated == true)
      {
        deleteChatByID(dwChatHeadID, dwLocalUserID);
      };

      return false;
    };

    delete pDatabase;


    ResetWatchDog();

    //set this message as latest
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, dwLocalUserID);

    if(nDirection == CHAT_DIRECTION_INCOMING)
    {
      nUnreadMsgs+= 1;
    };
  
    pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, true, DBRESERVE_DATA_COUNT);  
    pDatabase->open();
      
    if(pDatabase->isOpen() == true)
    {
      pRecordset = new CWSFFileDBRecordset(pDatabase, dwChatHeadID);

      if(pRecordset->haveValidEntry() == true)
      {
        pRecordset->setData(3, (void*)&dwMessageID, sizeof(dwMessageID));
        pRecordset->setData(4, (void*)&dwTime, sizeof(dwTime));
        pRecordset->setData(5, (void*)&nUnreadMsgs, sizeof(nUnreadMsgs));
      };

      delete pRecordset;
    };

    delete pDatabase;

    //create a task object to transfer the message to the other 
    //device (when not local) and get missing information...
    if(bIsLocal == false)
    {
      if(nDirection == CHAT_DIRECTION_OUTGOING)
      { 
        //create msg transfer task, enable when userid and 
        //dev id are known...
        dwTransferID = addDataToTransferTables((byte*)szMsg, strlen(szMsg), DATATABLE_DATATYPE_MSG, dwDevID, DeviceConfig.dwDeviceID, szSender, (char*)strUser.c_str(), 0, 1, DATATABLE_DATADIR_OUT, dwMessageID, dwLocalUserID);

        sDBTaskSendFile sendTask;

        sendTask.dwReleaseTask  = 0;
        sendTask.dwFileID       = dwTransferID;

        memset(szDatabaseFile, 0, sizeof(szDatabaseFile));
        memcpy(szDatabaseFile, &sendTask, sizeof(sendTask));

        dwTransferTaskID        = this->m_pdbTaskScheduler->addSchedule(DBTASK_SENDMESSAGE, 60, 3600, (byte*)&szDatabaseFile, (dwUsrID == 0 || dwDevID == 0 ? false : true));

        #if CLRP_DEBUG == 1
          Serial.print(F("[CLRLP] Create transfer task: file: "));
          Serial.print(dwTransferID);
          Serial.print(F(" task: "));
          Serial.println(dwTransferTaskID);
        #endif

        ResetWatchDog();

        //query user ID after dev ID
        if(dwUsrID == 0)
        {
          sDBTaskQueryUser task;

          task.dwReleaseTask    = dwTransferTaskID;
          task.dwUserToInform   = dwLocalUserID;
          task.dwDeviceID       = dwDevID;
          task.dwContactID      = dwContID;

          memset(szDatabaseFile, 0, sizeof(szDatabaseFile));
          memcpy(szDatabaseFile, &task, sizeof(task));

          #if CLRP_DEBUG == 1
            Serial.print(F("[CLRLP] Create User query task: for "));
            Serial.print(szUser);
            Serial.print(F(" remote device: "));
            Serial.println(dwDevID);
          #endif

          dwMsgTask = this->m_pdbTaskScheduler->addSchedule(DBTASK_QUERYUSER, 60, 3600, (byte*)&szDatabaseFile, (dwDevID > 0 ? true : false));
        };
        
        
        //transfer chat message
        if(dwDevID == 0)
        {
          #if CLRP_DEBUG == 1
            Serial.println(F("Create Node query task"));
          #endif
          
          sDBTaskQueryNode task;

          task.dwReleaseTask    = dwMsgTask;
          task.dwUserToInform   = dwLocalUserID;
          task.dwNodeToQuery    = 0;
          task.dwContactID      = dwContID;
          task.dwFileID         = dwTransferID;

          memset(szDatabaseFile, 0, sizeof(szDatabaseFile));
          memcpy(szDatabaseFile, &task, sizeof(task));

          this->m_pdbTaskScheduler->addSchedule(DBTASK_QUERYNODE, 60, 3600, (byte*)&szDatabaseFile, true);
        };
      }
      else
      {
        //create web event
        g_pWebEvent->addEvent(dwLocalUserID, 1);
      };
    }
    else
    {
      //create web event
      g_pWebEvent->addEvent(dwLocalUserID, 1);
    };
    
    return true;
  };
  
  return false;
};



bool CLoRaLinkProtocol::enqueueLoRaLinkMessage(uint32_t dwReceiverID, byte *pData, int nDataLen)
{
  //variables
  ///////////
  bool                  bRes       = true;
  _sSkyNetRoutingEntry  route;
  byte                  *pDataAnswer = new byte[SKYNET_PROTO_MAX_PACKET_LEN + 1];
  uint32_t              dwMsgID = this->m_pSkyNetConnection->getMessageID();
  int                   nLen;

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] enqueueLoRaLinkMessage: TX to: "));
    Serial.println(dwReceiverID);
  #endif
  
  if(SearchBestMatchingRoute(dwReceiverID, (_sSkyNetRoutingEntry*)&route) == true)
  {
    nLen = DATA_IND(pDataAnswer, dwMsgID, DeviceConfig.dwDeviceID, dwReceiverID, DeviceConfig.dwDeviceID, route.dwViaNode, pData, nDataLen);

    this->m_pSkyNetConnection->enqueueMsg(dwReceiverID, dwMsgID, route.pConnHandler->getTaskID(), route.pConnHandler->getConnectionType(), pDataAnswer, nLen, true);

    #if CLRP_DEBUG == 1
      Serial.print(F("[CLRLP] startTx: send DATA via: "));
      Serial.print(route->dwViaNode);  
      Serial.print(F(" to: "));
      Serial.print(dwReceiverID); 
      Serial.print(F(" size: "));
      Serial.println(nLen); 
    #endif
    
    bRes = true;
  }
  else
  {
    #if CLRP_ERROR == 1
      Serial.print(F("[CLRLP] startTx: no route for: "));
      Serial.println(dwReceiverID);  
    #endif
  };

  delete pDataAnswer;
  
  return bRes;
};



bool CLoRaLinkProtocol::enqueueRequestFileData(uint32_t dwReceiverDeviceID, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, int nBlockNumber, uint32_t dwRemoteTaskID, uint32_t dwLocalUserID)
{
  //variables
  ///////////
  int                   nLen          = 0;
  int                   nPayloadLen   = 0;
  bool                  bResult       = false;
  byte                  *pPayload     = new byte[60];
  byte                  *pDataAnswer  = new byte[100];  

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] enqueueRequestFileData: req from: "));
    Serial.print(dwReceiverDeviceID);
    Serial.print(F(", file ID: "));
    Serial.print(dwLocalFileID);
    Serial.print(F(", rem file ID: "));
    Serial.print(dwRemoteFileID);
    Serial.print(F(", Blk: "));
    Serial.println(nBlockNumber);
  #endif

  nPayloadLen   = LLPROTO_CreateDataReq(pPayload, dwLocalFileID, dwRemoteFileID, dwRemoteTaskID, nBlockNumber);
  nLen          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_DATA_REQ, dwLocalUserID);
  
  bResult       = this->enqueueLoRaLinkMessage(dwReceiverDeviceID, pDataAnswer, nLen);

  delete pDataAnswer;
  delete pPayload;
  
  return bResult;
};



bool CLoRaLinkProtocol::enqueueFileComplete(uint32_t dwReceiverDeviceID, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID)
{
  //variables
  ///////////
  int                   nLen          = 0;
  int                   nPayloadLen   = 0;
  bool                  bResult       = false;
  byte                  *pPayload     = new byte[60];
  byte                  *pDataAnswer  = new byte[100];  

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] enqueueFileComplete: devID: "));
    Serial.print(dwReceiverDeviceID);
    Serial.print(F(", LocalFileID: "));
    Serial.print(dwLocalFileID);
    Serial.print(F(", RemoteFileID: "));
    Serial.print(dwRemoteFileID);
    Serial.print(F(", TaskID: "));
    Serial.println(dwRemoteTaskID);
  #endif

  nPayloadLen   = LLPROTO_CreateDataComplete(pPayload, dwLocalFileID, dwRemoteFileID, dwRemoteTaskID);
  nLen          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_DATA_COMPLETE_IND, 0);
  
  bResult       = this->enqueueLoRaLinkMessage(dwReceiverDeviceID, pDataAnswer, nLen);

  delete pDataAnswer;
  delete pPayload;
  
  return bResult;
};



bool CLoRaLinkProtocol::enqueueFileTransfer(uint32_t dwReceiverDeviceID, uint32_t dwLocalUserID, char *szFilename, uint32_t dwLocalFileID, char *szSender, uint32_t dwSize, int nDataType, int nBlockSize, uint32_t dwTaskID)
{
  //variables
  ///////////
  int                   nLen          = 0;
  int                   nPayloadLen   = 0;
  bool                  bResult       = false;
  byte                  *pPayload     = new byte[60];
  byte                  *pDataAnswer  = new byte[100];  

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] enqueueFileTransfer: user/file: "));
    Serial.print(szFilename);
    Serial.print(F(", len: "));
    Serial.println(dwSize);
  #endif

  nPayloadLen   = LLPROTO_CreateDataHeaderIndication(pPayload, szFilename, dwLocalFileID, szSender, dwSize, nDataType, nBlockSize, dwTaskID);
  nLen          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_DATA_HEADER, dwLocalUserID);
  
  bResult       = this->enqueueLoRaLinkMessage(dwReceiverDeviceID, pDataAnswer, nLen);

  delete pDataAnswer;
  delete pPayload;
  
  return bResult;
};


bool CLoRaLinkProtocol::enqueueUserQuery(char *szUser, uint32_t dwDeviceID, uint32_t dwLocalUserID, uint32_t dwContactID, uint32_t dwTaskID)
{
  //variables
  ///////////
  int                   nLen          = 0;
  int                   nPayloadLen   = 0;
  bool                  bResult       = false;
  byte                  *pPayload     = new byte[50];
  byte                  *pDataAnswer  = new byte[80];

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] enqueueUserQuery: user: "));
    Serial.print(szUser);
    Serial.print(F(", len: "));
    Serial.println(strlen(szUser));
  #endif

  nPayloadLen   = LLPROTO_CreateQueryUser(pPayload, szUser, dwDeviceID, dwContactID, dwTaskID);
  nLen          = LLPROTO_Create(pDataAnswer, pPayload, nPayloadLen, LORA_PROTO_TYPE_USERQ_REQ, dwLocalUserID);

  #if CLRP_DEBUG == 1
    Serial.print(F("[CLRLP] Send UserQuery to "));
    Serial.print(dwDeviceID);
    Serial.print(F(" for User: "));
    Serial.print(szUser);
    Serial.print(F(" from loc usr: "));
    Serial.print(dwLocalUserID);
    Serial.print(F(" payload len: "));
    Serial.print(nPayloadLen);
    Serial.print(F(" data len: "));
    Serial.println(nLen);
  #endif
  
  bResult = this->enqueueLoRaLinkMessage(dwDeviceID, pDataAnswer, nLen);

  delete pDataAnswer;
  delete pPayload;
  
  return bResult;
};
