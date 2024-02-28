#ifndef __LORALINKDB__
#define __LORALINKDB__

//include
/////////
#include <Arduino.h>
#include "HardwareConfig.h"
#include <CWSFFileDB.h>
#include <FS.h>
#include "LoRaLinkConfig.h"


//number of entries to reserve memory for
//I ran into write problems on SPIFFS during my test,
//so i reserve min free space on the FS...
#define DBRESERVE_USER_COUNT    10
#define DBRESERVE_NODE_COUNT    10
#define DBRESERVE_DATA_COUNT    10
#define DBRESERVE_DDATA_COUNT   20
#define DBRESERVE_SCHED_COUNT   10
#define DBRESERVE_SHOUT_COUNT   100
#define DBRESERVE_CHAT_COUNT    10
#define DBRESERVE_CHATMSG_COUNT 10
#define DBRESERVE_CONTACT_COUNT 10




//user-table definition
//
//this table stores the user data
//
/////////////////////////////////////////////////
#define USERTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/user.tbl")).c_str()
#define USERTABLE_SIZE 9


const int nUserTableDef[] = 
{
  25,                 //user name
  sizeof(uint32_t),   //last login
  sizeof(uint32_t),   //last heard
  100,                //email
  sizeof(byte),       //forward messages to email
  40,                 //hash of password
  sizeof(byte),       //show date & time in interface
  sizeof(uint32_t),   //user created
  sizeof(byte)        //blocked from login / system
};



//node-table definition
//
//this table stores the nodes data (no routing 
//information). The data is persistant, it will
//not be removed, when the node is not longer
//reachable.
//
/////////////////////////////////////////////////
#define NODETABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/nodes.tbl")).c_str()
#define NODETABLE_SIZE 9


const int nNodeTableDef[] =
{
  sizeof(uint32_t),                 //node id
  sizeof(DeviceConfig.szDevName),   //dev name
  sizeof(int),                      //dev type
  sizeof(float),                    //dev location Lat
  sizeof(float),                    //dev location Lon
  sizeof(char),                     //location orientation (unused)
  sizeof(uint32_t),                 //last heard
  sizeof(int),                      //RSSI
  sizeof(float)                     //SNR
};



//scheduler-table definition
//
//this table stores the schedules or tasks which
//should be executed in fixed intervals.
//
/////////////////////////////////////////////////
#define SCHEDULERTABLE_FILE       (char*)(String(LORALINK_DATA_ROOT) + String("/schedule.tbl")).c_str()
#define SCHEDULERTABLE_SIZE       7
#define SCHEDULERTABLE_DATA_SIZE  200

const int nSchedulerTableDef[] =
{
  sizeof(int),                      //Schedule type
  sizeof(uint32_t),                 //last exec
  sizeof(int),                      //executions
  sizeof(int),                      //max tries
  sizeof(int),                      //re-schedule time in sec
  SCHEDULERTABLE_DATA_SIZE,         //Schedule Data
  sizeof(int)                       //enabled
};



#define DATAHEADERTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/datah.tbl")).c_str()
#define DATAHEADERTABLE_SIZE      16
#define DATATABLE_DATATYPE_MSG    1
#define DATATABLE_DATATYPE_SHOUT  2
#define DATATABLE_DATATYPE_FILE   3
#define DATATABLE_DATADIR_OUT     1
#define DATATABLE_DATADIR_IN      2

const int nDataHeaderTableDef[] =
{
  33,                               //File- / UserName (RcptTo)
  33,                               //UserName (Sender)
  sizeof(uint32_t),                 //file created time
  sizeof(uint32_t),                 //Size
  sizeof(uint32_t),                 //source device
  sizeof(uint32_t),                 //destination device
  sizeof(uint32_t),                 //last transmission time
  sizeof(int),                      //file type / data type
  sizeof(int),                      //file complete 
  sizeof(uint32_t),                 //remote file id    
  sizeof(int),                      //transfer enabled
  sizeof(int),                      //transfer direction
  sizeof(int),                      //header confirmed
  sizeof(int),                      //data max blocksize
  sizeof(uint32_t),                 //message id this transfer belongs to
  sizeof(uint32_t)                  //id of the user this transfer belongs to
};




#define DATATABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/datad.tbl")).c_str()
#define DATATABLE_SIZE            5
#define DATATABLE_BLOCKSIZE       128


const int nDataTableDef[] =
{
  sizeof(uint32_t),                 //file ID
  sizeof(int),                      //block number
  sizeof(int),                      //block transferred?
  sizeof(int),                      //size
  DATATABLE_BLOCKSIZE               //data
};


#define CONTACTSTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/%u.cnt")).c_str()
#define CONTACTSTABLE_SIZE 7

const int nContactsTableDef[] =
{
  sizeof(uint32_t),                 //UserID
  sizeof(uint32_t),                 //Device ID
  25,                               //User Name
  25,                               //device name
  sizeof(int),                      //contact state
  sizeof(int),                      //user blocked
  sizeof(int)                       //allow GPS tracking
};



#define CHATHEADTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/%u.chh")).c_str()
#define CHATHEADTABLE_SIZE 7

const int nChatHeadTableDef[] =
{
  sizeof(int),                      //chat type
  52,                               //FullUserName, group- or shout out name 
  sizeof(uint32_t),                 //Chat started time
  sizeof(uint32_t),                 //Last Msg ID
  sizeof(uint32_t),                 //Last Msg Time
  sizeof(int),                      //Number of unread msgs
  sizeof(uint32_t)                  //contact-, group- or shout out id
};


#define CHATMESSAGETABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/%u.chm")).c_str()
#define CHATMESSAGETABLE_SIZE 8

const int nChatMessageTableDef[] = 
{
  sizeof(uint32_t),                 //chat head id
  sizeof(int),                      //msg size
  sizeof(int),                      //msg transfer complete
  sizeof(int),                      //msg direction         
  sizeof(uint32_t),                 //msg sent / received time
  sizeof(uint32_t),                 //msg contact id
  LORALINK_MAX_MESSAGE_SIZE + 1,    //the msg
  sizeof(int)                       //msg read
};



#define SHOUTOUTTABLE_FILE            (char*)(String(LORALINK_DATA_ROOT) + String("/shoutout.tbl")).c_str()
#define SHOUTOUTTABLE_SIZE            5
#define SHOUTOUTTABLE_BLOCKSIZE       151


const int nShoutOutTableDef[] =
{
  70,                               //sender
  sizeof(uint32_t),                 //sent time
  SHOUTOUTTABLE_BLOCKSIZE,          //data
  sizeof(int),                      //transmitted (0/1)
  sizeof(uint32_t)                  //received by node id
};



#define TRACKHEADTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/trackh.tbl")).c_str()
#define TRACKHEADTABLE_SIZE 3


const int nTrackHeadTableDef[] =
{
  sizeof(uint32_t),                 //user id
  sizeof(uint32_t),                 //start time
  255                               //track name
};


#define TRACKWAYPOINTTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/trackhwp.tbl")).c_str()
#define TRACKWAYPOINTTABLE_SIZE 5


const int nTrackWaypointTableDef[] =
{
  sizeof(uint32_t),                 //track head id
  sizeof(float),                    //lat
  sizeof(float),                    //long
  sizeof(float),                    //elevation
  sizeof(uint32_t)                  //timestamp
};


//definitions
/////////////
extern CWSFFileDB *g_pUserTable;
extern CWSFFileDB *g_pNodeTable;
extern CWSFFileDB *g_pShoutOutTable;
extern CWSFFileDB *g_pDataTable;
extern CWSFFileDB *g_pDataHeaderTable;
extern CWSFFileDB *g_pTrackDataTable;



//usertable functions
/////////////////////
bool      FindUserByName(CWSFFileDBRecordset *pRS, const char *szUserName);         //this function searches for the first entry matching username
bool      GetUserNameByID(uint32_t dwUserID, char *szUserName, int nMaxLen);        //returns the username by id
uint32_t  GetUserIdByName(char *szUserName);
bool      CreateUser(char *szUserName, char *szPwdHash, char *szEmail);




//node table
////////////
bool      FindDeviceByNodeID(CWSFFileDBRecordset *pRS, uint32_t dwNodeID);
bool      FindDeviceByNodeName(CWSFFileDBRecordset *pRS, char *szNodeName);
uint32_t  GetNodeIdByName(char *szNodeName);
bool      updateLastHeard(uint32_t dwNodeID, uint32_t dwTime);


//user contact table
////////////////////
bool      CheckApiAdminPassword(const char *szUser, const char *szPwdHash);
bool      CheckApiUserPassword(uint32_t dwUserPos, const char *szPwdHash);
bool      FindContactByID(CWSFFileDBRecordset *pRS, uint32_t dwUserID, uint32_t dwDeviceID);
bool      FindContactByName(CWSFFileDBRecordset *pRS, char *szUserName, char *szDeviceName);
uint32_t  InsertContactForUser(uint32_t dwForUser, uint32_t dwUserID, uint32_t dwDeviceID, char *szUserName, char *szDevName, int nContactState);
bool      isContactBlocked(uint32_t dwLocalUserID, uint32_t dwRemoteUserID, uint32_t dwRemoteDeviceID);


//chat related functions
////////////////////////
bool      FindChatForContactID(CWSFFileDBRecordset *pRS, uint32_t dwContactID);
bool      deleteChatByID(uint32_t dwMsgHeadID, uint32_t dwUserID);
bool      deleteChatMessagesByID(uint32_t dwMsgHeadID, uint32_t dwUserID, bool bUpdate);


//data transfer functions
/////////////////////////
uint32_t addDataToTransferTables(byte *pData, int nDataLen, int nFileType, uint32_t dwDestinationDev, uint32_t dwSourceDev, char *szSender, char *szRcptTo, uint32_t dwRemoteFileID, int nTransferEnabled, int nDirection, uint32_t dwLocalMessageID, uint32_t dwResponsibleUserID);
bool     removeDataTransferTablesHeader(uint32_t dwHeaderID);
bool     isDataTransferHeaderConfirmed(uint32_t dwHeaderID);
uint32_t getDataHeadIdBySourceAndFileId(CWSFFileDB *pDB, uint32_t dwSourceID, uint32_t dwFileID);
bool     FindEntryForFileIdAndBlockNumber(CWSFFileDBRecordset *pRS, uint32_t dwFileID, int nBlockNumber);
int      assembleMessageData(byte *pData, int nMaxLen, uint32_t dwFileID);



//shout out db functions
////////////////////////
void     limitShoutEntrys(int nLimit);



//track db functions
void    openTrackDataDB();





bool initDatabase(fs::FS &fs);



#endif
