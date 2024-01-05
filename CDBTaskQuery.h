#ifndef __DBTQUERYNODE__
#define __DBTQUERYNODE__

//includes
//////////
#include <Arduino.h>


#define DBTASK_QUERYNODE        1
#define DBTASK_QUERYUSER        2
#define DBTASK_SENDMESSAGE      3
#define DBTASK_RSTFILETRANSFER  4
#define DBTASK_REQUESTFILEDATA  5
#define DBTASK_CONFIRMTRANSFER  6
#define DBTASK_CHECKWEBSERVER   7
#define DBTASK_SEND_POSITION    8
#define DBTASK_RECORD_TRACK     9


//an alwys running task, which restarts file transfer after device reset
struct sDBTaskRestartFileTransfer
{
  uint32_t dwLastFile;
  uint32_t dwLastRequest;
};



struct sDBTaskQueryNode 
{
  uint32_t dwNodeToQuery;   //id of the node to query  
  uint32_t dwUserToInform;  //if > 0, an answer would update the user contacts table
  uint32_t dwContactID;     //contact id in table to update
  uint32_t dwFileID;        //file id in table to update
  uint32_t dwReleaseTask;   //when > 0 the task will be enabled on answer
};




struct sDBTaskQueryUser 
{
  uint32_t dwDeviceID;      //id of the dev to query
  uint32_t dwUserToInform;  //if > 0, an answer would update the user contacts table
  uint32_t dwContactID;     //contact id in table to update  
  uint32_t dwReleaseTask;   //when > 0 the task will be enabled on answer
};


struct sDBTaskSendFile 
{
  uint32_t dwFileID;        //msg id in data header table  
  uint32_t dwReleaseTask;   //when > 0 the task will be enabled on answer
};


//compare function which can be used to check if a specific node
//will always be queried...
bool compareTaskQueryNode(byte *pTaskData, int nType, void *pStruct);




#endif
