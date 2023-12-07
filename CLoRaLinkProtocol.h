#ifndef __LORALINKPROTO__
#define __LORALINKPROTO__

//includes
//////////
#include <Arduino.h>
#include "CSkyNetConnection.h"
#include "CDBTaskScheduler.h"
#include "SkyNetProtocol.h"
#include "CLoRaLinkDatabase.h"


#define CLRP_INFO                           1
#define CLRP_DEBUG                          1
#define CLRP_ERROR                          1



#define LLProtoMinHeaderLength              (1 + (2 * sizeof(uint32_t)))
#define LLProtoBaseDebug                    1

//message types
#define CHAT_TYPE_MESSAGE                   1
#define CHAT_TYPE_SHOUTOUT                  2

//chat directions
#define CHAT_DIRECTION_OUTGOING             0
#define CHAT_DIRECTION_INCOMING             1

//position types
#define GPS_POSITION_TYPE_NORMAL            0
#define GPS_POSITION_TYPE_EMERGENCY         1



#define LORA_PROTO_TYPE_DATA_HEADER         1
#define LORA_PROTO_TYPE_DATA                2
#define LORA_PROTO_TYPE_DATA_REQ            3
#define LORA_PROTO_TYPE_USERQ_REQ           4
#define LORA_PROTO_TYPE_USERQ_RESP          5
#define LORA_PROTO_TYPE_DATA_HEADER_RESP    6
#define LORA_PROTO_TYPE_DATA_RESP           7
#define LORA_PROTO_TYPE_DATA_COMPLETE_IND   8
#define LORA_PROTO_TYPE_DATA_COMPLETE_CONF  9
#define LORA_PROTO_TYPE_SHOUTOUT_IND        10
#define LORA_PROTO_TYPE_SHOUTOUT_CONF       11
#define LORA_PROTO_TYPE_USER_LOCATE_REQ     12
#define LORA_PROTO_TYPE_USER_LOCATE_RESP    13
#define LORA_PROTO_TYPE_USER_POSITION_IND   14







struct _sNewMessage
{
  int           nMsgType;
  String        strUser;
  String        strDev;
  String        strRcptTo;
  char          szMsg[LORALINK_MAX_MESSAGE_SIZE + 1];
  uint32_t      dwUserID;
  char          szSender[30];
  uint32_t      dwRcptDevID;
  uint32_t      dwRcptUserID;
  uint32_t      dwRcptContactID;
};






//create header
int  LLPROTO_Create(byte *pResult, byte *pPayload, int nPayloadLen, int nProtocolType, uint32_t dwSenderDevice, uint32_t dwSenderUserID);
bool LLPROTO_Decode(byte *pData, int nDataLen, byte *pPayload, int *pnPayloadLen, int *pnProtocolType, uint32_t *pdwSenderDevice, uint32_t *pdwSenderUserID);
  


int  LLPROTO_CreateQueryUser(byte *pResult, char *szUser, uint32_t dwDeviceToQuery, uint32_t dwContactID, uint32_t dwTaskID);
bool LLPROTO_DecodeQueryUser(byte *pData, int nDataLen, char *szUser, uint32_t *dwDeviceToQuery, uint32_t *dwContactID, uint32_t *dwTaskID);



int  LLPROTO_CreateQueryUserResp(byte *pResult, uint32_t dwContactID, uint32_t dwLocalContactID, uint32_t dwTaskID, int nResult);
bool LLPROTO_DecodeQueryUserResp(byte *pData, int nDataLen, uint32_t *pdwContactID, uint32_t *pdwLocalContactID, uint32_t *pdwTaskID, int *pnResult);



int  LLPROTO_CreateDataHeaderIndication(byte *pResult, char *szFilename, uint32_t dwLocalFileID, char *szSender, uint32_t dwSize, int nDataType, int nBlockSize, uint32_t dwTransferTaskID);
bool LLPROTO_DecodeDataHeaderIndication(byte *pData, int nDataLen, char *szFilename, uint32_t *pdwLocalFileID, char *szSender, uint32_t *pdwSize, int *pnDataType, int *pnBlockSize, uint32_t *pdwTransferTaskID);



int  LLPROTO_CreateDataHeaderResp(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nRejected); 
bool LLPROTO_DecodeDataHeaderResp(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnRejected); 



int  LLPROTO_CreateDataReq(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nBlockNumber);
bool LLPROTO_DecodeDataReq(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnBlockNumber);



int  LLPROTO_CreateDataResp(byte *pResult, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID, int nBlockNumber, byte *pData, int nDataLen, int nSuccess); 
bool LLPROTO_DecodeDataResp(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID, uint32_t *pdwRemoteTaskID, int *pnBlockNumber, byte *pResultData, int nResultDataLen);



int  LLPROTO_CreateDataComplete(byte *pResult, uint32_t dwLocalFileID, uint32_t *pdwRemoteFileID);
bool LLPROTO_DecodeDataComplete(byte *pData, int nDataLen, uint32_t *pdwLocalFileID, uint32_t *pdwRemoteFileID);


int  LLPROTO_CreateShoutOutInfo(byte *pResult, char *szUser, uint32_t dwTimeSent, char *szMsg);
bool LLPROTO_DecodeShoutOutInfo(byte *pData, int nDataLen, char *szUser, uint32_t *pdwTimeSent, char *szMsg);

#if LORALINK_HARDWARE_GPS == 1

  int  LLPROTO_CreateUserLocateReq(byte *pResult, uint32_t dwUntil, uint32_t dwRemoteUserID);
  bool LLPROTO_DecodeUserLocateReq(byte *pData, int nDataLen, uint32_t *pdwUntil, uint32_t *pdwRemoteUserID);
  
  int  LLPROTO_CreateUserLocateResp(byte *pResult, int nResult);
  bool LLPROTO_DecodeUserLocateResp(byte *pData, int nDataLen, int *pnResult);
      
  int  LLPROTO_CreateUserPositionInd(byte *pResult, float fCourse, float fSpeed, int nHDOP, int nNumSat, uint32_t dwLastValid, float fLatitude, float fLongitude, float fAltitude, bool bValidSignal, int nPosType);
  bool LLPROTO_DecodeUserPositionInd(byte *pData, int nDataLen, float *pfCourse, float *pfSpeed, int *pnHDOP, int *pnNumSat, uint32_t *pdwLastValid, float *pfLatitude, float *pfLongitude, float *pfAltitude, bool *pbValidSignal, int *pnPosType);

#endif


class CLoRaLinkProtocol : public CTaskIF
{
  public:
  
    CLoRaLinkProtocol(CSkyNetConnection *pSkyNetConnection, CDBTaskScheduler *pdbTaskScheduler);
    ~CLoRaLinkProtocol();

    bool handleLoRaLinkProtocolData(byte *pData, int nLength);
    void handleTask();

    //functions
    ///////////

    //this method requests block data
    bool enqueueRequestFileData(uint32_t dwReceiverDeviceID, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, int nBlockNumber, uint32_t dwRemoteTaskID, uint32_t dwLocalUserID);
    
    //this method confirms the transfer
    bool enqueueFileComplete(uint32_t dwReceiverDeviceID, uint32_t dwLocalFileID, uint32_t dwRemoteFileID, uint32_t dwRemoteTaskID);

    //this method enques a file transfer and informs the remote device
    //over a file for him... 
    bool enqueueFileTransfer(uint32_t dwReceiverDeviceID, uint32_t dwLocalUserID, char *szFilename, uint32_t dwLocalFileID, char *szSender, uint32_t dwSize, int nDataType, int nBlockSize, uint32_t dwTaskID);
    

    //this method querys the id of a remote user
    bool enqueueUserQuery(char *szUser, uint32_t dwDeviceID, uint32_t dwLocalUserID, uint32_t dwContactID, uint32_t dwTaskID);

    //this method adds a new message to a new or existing chat, unlinks the contact- / device
    //and fill the user contacts table...
    bool addMessage(uint32_t dwLocalUserID, String strUser, String strDev, char *szMsg, uint32_t dwDevID, uint32_t dwUsrID, uint32_t dwContID, int nDirection);

    #if LORALINK_HARDWARE_GPS == 1
    
      //this send a pos tracking request to a remote device. if tracking was allowed by the other user
      //the other device creates a task sending it's position. position tracking only works over LoRa,
      //for "normal" positions. if an emergency pos is received, this position will be forwarded to all
      //known devices!
      bool addPositionTrackingReq(uint32_t dwLocalUserID, uint32_t dwRemoteDevID, uint32_t dwRemoteUsrID, int nDurationMin);
  
      
      bool addPosition(uint32_t dwRemoteDevID, uint32_t dwLocalUserID, int nPositionType);
      
    #endif
    
    bool addShoutOut(uint32_t dwSenderNodeID, uint32_t dwUserID, String strUser, String strDev, char *szMsg);  


    //skynet pass through
    /////////////////////
    void enqueueQueryRequest(uint32_t dwNodeID, char *szNodeName, bool bForward, uint32_t dwScheduleID);


    //this method send the shout out to all known nodes
    //except the sender node
    bool enqueueShoutOut(uint32_t dwSenderNodeID, char *szUser, char *szMsg, uint32_t dwTime);

  private:

    //variables
    ///////////
    CSkyNetConnection *m_pSkyNetConnection;
    CDBTaskScheduler  *m_pdbTaskScheduler;

    //this method searches a route and puts the data into the message queue
    bool enqueueLoRaLinkMessage(uint32_t dwReceiverID, byte *pData, int nDataLen); 

    
};



#endif
