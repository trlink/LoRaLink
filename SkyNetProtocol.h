#ifndef __SKYNET_PROTOCOL__
#define __SKYNET_PROTOCOL__


//includes
//////////
#include <Arduino.h>

//defines
/////////
#define SKYNET_PROTO_DEBUG              1
#define SKYNET_PROTO_RETRIES            3
#define SKYNET_PROTO_MSG_TIMEOUT        ((60 * 1000) * 3)
#define SKYNET_PROTO_MAX_PACKET_LEN     250
#define SKYNET_PROTO_MAX_DATA_LEN       (SKYNET_PROTO_MAX_PACKET_LEN - SKYNET_PROTO_MAX_PACKET_LEN)
#define SKYNET_HEADER_SIZE              (2 + (7 * sizeof(uint32_t)))



//response codes
#define SKYNET_RESPONSE_OK              0
#define SKYNET_RESPONSE_NO_ROUTE        1

//skynet protocol sub commands
#define SKYNET_SUBCMD_REQ               1
#define SKYNET_SUBCMD_RESP              2
#define SKYNET_SUBCMD_IND               3
#define SKYNET_SUBCMD_CONF              4

//skynet protocol commands for handling 
//local connections (own "bubble")
#define SKYNET_CMD_HELLO                1
#define SKYNET_CMD_KEEPALIVE            2
#define SKYNET_CMD_QUERY                3
#define SKYNET_CMD_DATA                 4
#define SKYNET_CMD_PROTOCOL_MSG         6


/*
TR-Link v2 Protocol Msg Structure      
Name        Type  Size  Desc
Msg Length    BYTE  1     Length of message
Msg ID        DWORD 4     ID of Message from sender
Msg Type      BYTE  1     Type of Message (COMMAND & SUB_COMMAND)
Hop Count     DWORD 4     Number of Hops passed
Sender ID     DWORD 4     Sender Device ID
Receiver ID   DWORD 4     Receiver Device ID
Origin ID     DWORD 4     Originating Device ID
Via ID        DWORD 4     Via Device ID
Data          BYTE  50    Data
CheckSum      DWORD 4     CheckSum of message
*/


struct _sSkyNetProtocolMessage
{
  byte      bMsgLength;
  uint32_t  dwMsgID;
  byte      bMsgType;
  uint32_t  nHopCount;
  uint32_t  nSenderID;
  uint32_t  nReceiverID;
  uint32_t  nOriginID;
  uint32_t  nViaID;
  int       nDataLen;
  byte      *pData;
  uint32_t  nCheckSum;
};

int       WriteINT(byte *pData, int nValue);
int       ReadINT(byte *pData);
int       WriteDWORD(byte *pData, uint32_t dwValue);
uint32_t  ReadDWORD(byte *pData);
int       WriteFloat(byte *pData, float fValue);
float     ReadFloat(byte *pData);


/**
 * this function decodes a byte stream into a _sSkyNetProtocolMessage struct. If the function returns true
 * the decoding was successful otherwise there was an error decoding the packet (CRC missmatch or logocal failures)
 */
bool decodeSkyNetProtocolMessage(byte *pData, int nDataLen, _sSkyNetProtocolMessage *pMsg);


/*
 * this function turns the _sSkyNetProtocolMessage struct into a byte buffer containing the data to send.
 * it returns the length
 */
int  encodeSkyNetProtocolMessage(_sSkyNetProtocolMessage *pMsg, byte *pData);


//send a local keep alive
int KEEPALIVE_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID);


//this command confirmes a forwarded message, which needs a confirmation
//that it was transferred to another node
int PROTOCOL_MSG_CONF(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwMsgID, bool bFailed);


//this commands register a device locally
int HELLO_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  nViaID, char *szDevName, byte bDevType, float fLocN, float fLocE, char szLocOrientation, uint32_t dwHopCount);
int HELLO_CONF(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, uint32_t dwMsgID, int nResponse);
int HELLO_REQ(byte *pResult, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, uint32_t dwMsgID);

//this method asks locally for a specific node, if bForward = false, if bForward = true, the request will be 
//sent into the whole network... when the devId is set and szDevName not, the receiving systems check their db
//for the data by the id of the system and return it... if the name is set, the dev searches for the name...
int QUERY_REQ(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t dwViaID, bool bForward, char *szDevName, uint32_t dwDeviceID, uint32_t dwScheduleID);
int QUERY_RESP(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, char *szDevName, uint32_t dwDeviceID, uint32_t dwScheduleID, int nDevType);


//methods to send data through the net
int DATA_IND(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, byte *pData, int nDataLength);
int DATA_CONF(byte *pResult, uint32_t dwMsgID, uint32_t dwSenderID, uint32_t dwReceiverID, uint32_t dwOriginID, uint32_t  dwViaID, int nResult);

#endif
