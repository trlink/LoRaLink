//includes
//////////
#include "SkyNetProtocol.h"
#include <Arduino_CRC32.h>
#include <string.h>
#include "CWSFClockHelper.h"


int WriteINT(byte *pData, int nValue)
{
  memcpy(pData, (int*)&nValue, sizeof(int));

  return sizeof(int);
};



int ReadINT(byte *pData)
{
  //variables
  ///////////
  int  nTemp = 0;

  memcpy(&nTemp, pData, sizeof(int));

  return nTemp;
};



int WriteDWORD(byte *pData, uint32_t dwValue)
{
  memcpy(pData, (uint32_t*)&dwValue, sizeof(uint32_t));

  return sizeof(uint32_t);
};



uint32_t ReadDWORD(byte *pData)
{
  //variables
  ///////////
  uint32_t  dwTemp = 0;

  memcpy(&dwTemp, pData, sizeof(uint32_t));

  return dwTemp;
};



int WriteFloat(byte *pData, float fValue)
{
  memcpy(pData, (float*)&fValue, sizeof(float));

  return sizeof(float);
};



float ReadFloat(byte *pData)
{
  //variables
  ///////////
  float  fTemp = 0;

  memcpy(&fTemp, pData, sizeof(float));

  return fTemp;
};







bool decodeSkyNetProtocolMessage(byte *pData, int nDataLen, _sSkyNetProtocolMessage *pMsg)
{
  //variables
  ///////////
  int       nPos = 0;
  int       nLen;
  uint32_t  nTemp = 0;
  Arduino_CRC32 crc32;

  pMsg->nDataLen  = 0;
  pMsg->pData     = NULL;
      
  if(nDataLen >= SKYNET_HEADER_SIZE)
  {
    pMsg->bMsgLength     = pData[nPos++];

    #if SKYNET_PROTO_DEBUG == 1
      Serial.print(F("[SKYNETP] decodeSkyNetProtocolMessage: rx data len: "));
      Serial.print(nDataLen);
      Serial.print(F(" decoded packet len: "));
      Serial.println(pMsg->bMsgLength);
    #endif

    if(pMsg->bMsgLength != nDataLen)
    {
      #if SKYNET_PROTO_DEBUG == 1
        Serial.println(F("[SKYNETP] decodeSkyNetProtocolMessage: packet data len missmatch!"));
      #endif

      return false;
    };
    
    pMsg->dwMsgID        = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);
    pMsg->bMsgType       = pData[nPos++];

    pMsg->nHopCount      = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);

    pMsg->nSenderID      = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);
  
    pMsg->nReceiverID    = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);
  
    pMsg->nOriginID      = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);

    pMsg->nViaID         = ReadDWORD(pData + nPos);
    nPos                += sizeof(uint32_t);

    //check if msg contains data
    nLen = pMsg->bMsgLength - SKYNET_HEADER_SIZE;

    if(nLen > 0)
    {
      #if SKYNET_PROTO_DEBUG == 1
        Serial.print(F("[SKYNETP] decodeSkyNetProtocolMessage: decoded payload len: "));
        Serial.println(nLen);
      #endif
      
      pMsg->nDataLen = nLen;
      pMsg->pData    = new byte[nLen + 1];
      
      memcpy(pMsg->pData, pData + nPos, pMsg->nDataLen); 
      nPos          += pMsg->nDataLen;
    };

    pMsg->nCheckSum   = ReadDWORD(pData + nPos);

    nTemp = crc32.calc(pData, nPos);

    if(nTemp == pMsg->nCheckSum)
    {
      return true;
    }
    else
    {
      #if SKYNET_PROTO_DEBUG == 1
        Serial.println(F("[SKYNETP] decodeSkyNetProtocolMessage: checksum missmatch..."));
      #endif
    };
  }
  else 
  {
    #if SKYNET_PROTO_DEBUG == 1
      Serial.println(F("decodeSkyNetProtocolMessage: Header missmatch..."));
    #endif
  };

  return false;
};





int  encodeSkyNetProtocolMessage(_sSkyNetProtocolMessage *pMsg, byte *pData)
{
  //variables
  ///////////
  int           nPos = 0;
  Arduino_CRC32 crc32;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] encodeSkyNetProtocolMessage: message len: "));
    Serial.print(SKYNET_HEADER_SIZE + pMsg->nDataLen);
    Serial.print(F(" payload len: "));
    Serial.println(pMsg->nDataLen);
  #endif
  
  pData[nPos++] = SKYNET_HEADER_SIZE + pMsg->nDataLen;
  nPos += WriteDWORD(pData + nPos, pMsg->dwMsgID);
  pData[nPos++] = pMsg->bMsgType;

  nPos += WriteDWORD(pData + nPos, pMsg->nHopCount);

  nPos += WriteDWORD(pData + nPos, pMsg->nSenderID);

  nPos += WriteDWORD(pData + nPos, pMsg->nReceiverID);

  nPos += WriteDWORD(pData + nPos, pMsg->nOriginID);

  nPos += WriteDWORD(pData + nPos, pMsg->nViaID);

  if(pMsg->nDataLen > 0)
  {
    memcpy(pData + nPos, pMsg->pData, pMsg->nDataLen);
    nPos += pMsg->nDataLen;
  };

  pMsg->nCheckSum = crc32.calc(pData, nPos);
  nPos += WriteDWORD(pData + nPos, pMsg->nCheckSum);

  return nPos;
};



void prepareHeader(_sSkyNetProtocolMessage *pMsg, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t dwViaID, byte bMsgType, uint32_t dwHopCount)
{
  pMsg->nDataLen    = 0;
  pMsg->pData       = NULL;
  pMsg->dwMsgID     = dwMsgID;
  pMsg->bMsgType    = bMsgType;
  pMsg->nHopCount   = dwHopCount;
  pMsg->nSenderID   = dwSenderID;
  pMsg->nOriginID   = dwOriginID;
  pMsg->nReceiverID = dwReceiverID;
  pMsg->nViaID      = dwViaID; 
};


int  KEEPALIVE_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create KEEPALIVE_IND --> Sender: "));
    Serial.println(dwSenderID);
  #endif
  

  prepareHeader(&msg, dwMsgID, dwSenderID, 0, dwSenderID, dwSenderID, (SKYNET_SUBCMD_IND << 4) + SKYNET_CMD_KEEPALIVE, 0);
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};


int  HELLO_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t dwViaID, char *szDevName, byte bDevType, float fLocN, float fLocE, char szLocOrientation, uint32_t dwHopCount)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szData[100];
  int  nPos = 0;
  uint32_t dwTimeStamp = (ClockPtr->timeSet() == true ? ClockPtr->GetCurrentTime().unixtime() : 0);

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create HELLO_IND --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif

  memset(szData, 0, sizeof(szData));

  szData[nPos++] = bDevType;
  szData[nPos++] = strlen(szDevName);

  strcpy(((char*)&szData) + nPos, szDevName);
  nPos += strlen(szDevName);

  nPos += WriteFloat(((byte*)&szData) + nPos, fLocN);
  nPos += WriteFloat(((byte*)&szData) + nPos, fLocE);
  szData[nPos++] = szLocOrientation;
  nPos += WriteDWORD(((byte*)&szData) + nPos, dwTimeStamp);

  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_IND << 4) + SKYNET_CMD_HELLO, dwHopCount);
  
  msg.nDataLen  = nPos;
  msg.pData     = (byte*)&szData;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};



int HELLO_CONF(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, uint32_t dwMsgID, int nResponse)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szData[10];
  int  nPos = 0;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create HELLO_CONF --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif

  memset(szData, 0, sizeof(szData));
  szData[nPos++] = nResponse;
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_CONF << 4) + SKYNET_CMD_HELLO, 0);

  msg.nDataLen  = nPos;
  msg.pData     = (byte*)&szData;
   
  return encodeSkyNetProtocolMessage(&msg, pResult);
};


int HELLO_REQ(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, uint32_t dwMsgID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create HELLO_REQ --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_REQ << 4) + SKYNET_CMD_HELLO, 0);
 
  return encodeSkyNetProtocolMessage(&msg, pResult);
};



int QUERY_REQ(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, bool bForward, char *szDevName, uint32_t dwDeviceID, uint32_t dwScheduleID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szData[100];
  int  nPos = 0;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create QUERY_REQ --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
 
  memset(szData, 0, sizeof(szData));

  szData[nPos++] = (bForward == true ? 1 : 0);
  szData[nPos++] = strlen(szDevName);

  strcpy(((char*)&szData) + nPos, szDevName);
  nPos += strlen(szDevName);

  nPos += WriteDWORD(((byte*)&szData) + nPos, dwDeviceID);
  nPos += WriteDWORD(((byte*)&szData) + nPos, dwScheduleID);

  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_REQ << 4) + SKYNET_CMD_QUERY, 0);

  msg.nDataLen  = nPos;
  msg.pData     = (byte*)&szData;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);  
};



int QUERY_RESP(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, char *szDevName, uint32_t dwDeviceID, uint32_t dwScheduleID, int nDevType)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szData[100];
  int  nPos = 0;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create QUERY_RESP --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
 
  memset(szData, 0, sizeof(szData));

  szData[nPos++] = strlen(szDevName);

  strcpy(((char*)&szData) + nPos, szDevName);
  nPos += strlen(szDevName);

  nPos += WriteDWORD(((byte*)&szData) + nPos, dwDeviceID);
  nPos += WriteDWORD(((byte*)&szData) + nPos, dwScheduleID);
  nPos += WriteINT(((byte*)&szData) + nPos, nDevType);


  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_RESP << 4) + SKYNET_CMD_QUERY, 0);

  msg.nDataLen  = nPos;
  msg.pData     = (byte*)&szData;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};



int DATA_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, byte *pData, int nDataLength)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create DATA_IND --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.print(dwViaID);
    Serial.print(F(", MsgID: "));
    Serial.print(dwMsgID);
    Serial.print(F(", len: "));
    Serial.println(nDataLength);
  #endif
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_IND << 4) + SKYNET_CMD_DATA, 0);

  msg.nDataLen  = nDataLength;
  msg.pData     = pData;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};



int PROTOCOL_MSG_CONF(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwMsgID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create PROTOCOL_MSG_CONF --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", MsgID: "));
    Serial.println(dwMsgID);
  #endif
   
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwSenderID, 0, (SKYNET_SUBCMD_CONF << 4) + SKYNET_CMD_PROTOCOL_MSG, 0);

  msg.nDataLen  = 0;
  msg.pData     = NULL;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};


int DATA_CONF(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, int nResult)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;
  char szData[10];
  int  nPos = 0;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create DATA_CONF --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
 
  memset(szData, 0, sizeof(szData));
  szData[nPos++] = (byte)(nResult & 0xFF);
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_CONF << 4) + SKYNET_CMD_DATA, 0);

  msg.nDataLen  = nPos;
  msg.pData     = (byte*)&szData;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};



int PING_REQ(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create PING_REQ --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_REQ << 4) + SKYNET_CMD_PING, 0);

  msg.nDataLen  = 0;
  msg.pData     = NULL;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};


int PING_RESP(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID)
{
  //variables
  ///////////
  _sSkyNetProtocolMessage msg;

  #if SKYNET_PROTO_DEBUG == 1
    Serial.print(F("[SKYNETP] create PING_RESP --> Sender: "));
    Serial.print(dwSenderID);
    Serial.print(F(", Receiver: "));
    Serial.print(dwReceiverID);
    Serial.print(F(", Origin: "));
    Serial.print(dwOriginID);
    Serial.print(F(", Via: "));
    Serial.println(dwViaID);
  #endif
  
  prepareHeader(&msg, dwMsgID, dwSenderID, dwReceiverID, dwOriginID, dwViaID, (SKYNET_SUBCMD_RESP << 4) + SKYNET_CMD_PING, 0);

  msg.nDataLen  = 0;
  msg.pData     = NULL;
  
  return encodeSkyNetProtocolMessage(&msg, pResult);
};
