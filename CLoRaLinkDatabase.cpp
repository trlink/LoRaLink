
//include
/////////
#include <FS.h>
#include <string.h>
#include "CLoRaLinkDatabase.h"
#include "CWSFClockHelper.h"
#include "SystemState.h"
#include "HardwareConfig.h"



//globals
/////////
CWSFFileDB *g_pUserTable        = NULL;
CWSFFileDB *g_pNodeTable        = NULL;
CWSFFileDB *g_pShoutOutTable    = NULL;
CWSFFileDB *g_pDataTable        = NULL;
CWSFFileDB *g_pDataHeaderTable  = NULL;
CWSFFileDB *g_pTrackDataTable   = NULL;


//user related 
//////////////

bool CheckApiUserPassword(uint32_t dwUserPos, const char *szPwdHash)
{
  //variables
  ///////////
  CWSFFileDBRecordset rsUser(g_pUserTable, dwUserPos);
  char *szHash = new char[50];

  if(rsUser.haveValidEntry() == true)
  {
    memset(szHash, 0, 50);
    rsUser.getData(5, (void*)szHash, 50);
    
    if(strcasecmp(szHash, szPwdHash) == 0)
    {
      delete szHash;
      
      return true;
    };
  };

  delete szHash;

  return false;
};


bool CheckApiAdminPassword(const char *szUser, const char *szPwdHash)
{
  if((strcasecmp(szUser, AdminCfg.szUser) == 0) && (strcasecmp(szPwdHash, AdminCfg.szPassword) == 0))
  {
    return true;
  };
  
  return false;
};



uint32_t  GetUserIdByName(char *szUserName)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(g_pUserTable);
  char                szTemp[40];
  
  
  while(rs.haveValidEntry() == true)
  {
    memset(szTemp, 0, sizeof(szTemp));
    
    rs.getData(0, (byte*)&szTemp, sizeof(szTemp));
    
    if(strcasecmp(szTemp, szUserName) == 0)
    {
      Serial.print(F("GetUserIdByName: found: "));
      Serial.println(szTemp);
      
      return rs.getRecordPos();
    };

    ResetWatchDog();
    rs.moveNext();
  };

  Serial.print(F("GetUserIdByName: not found: "));
  Serial.println(szUserName);
      
  return 0;
};



bool FindUserByName(CWSFFileDBRecordset *pRS, const char *szUserName)
{
  //variables
  ///////////
  byte szData[30];
  
  Serial.print("FindUserByName: Search for ");
  Serial.println(szUserName);
  
  if(pRS->moveFirst() == true)
  {
    do 
    {
      if(pRS->haveValidEntry() == true)
      {
        memset(szData, 0, sizeof(szData));
        
        if(pRS->getData(0, (void*)&szData, sizeof(szData)) == true)
        {         
          if(strcasecmp((char*)&szData, szUserName) == 0)
          {
            Serial.print("FindUserByName: found ");
            Serial.println(szUserName);
    
            return true;
          };
        };
      };
      
      ResetWatchDog();
    }
    while(pRS->moveNext() == true);
  };

  Serial.print("FindUserByName: not found ");
  Serial.println(szUserName);
  
  return false;
};


bool CreateUser(char *szUserName, char *szPwdHash, char *szEmail)
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRS = new CWSFFileDBRecordset(g_pUserTable);
  void                *pInsert[USERTABLE_SIZE + 1];
  uint32_t            dwLastLogin = 0;
  float               fPos = 0.0;
  byte                bFwd = 0;
  byte                bShow = 1;
  uint32_t            dwCreated = ClockPtr->GetCurrentTime().unixtime();

  
  if(FindUserByName(pRS, szUserName) == false)
  {
    pInsert[0] = (void*)szUserName;
    pInsert[1] = (void*)&dwLastLogin;
    pInsert[2] = (void*)&dwLastLogin;
    pInsert[3] = (void*)szEmail;
    pInsert[4] = (void*)&bFwd;
    pInsert[5] = (void*)szPwdHash;
    pInsert[6] = (void*)&bShow;
    pInsert[7] = (void*)&dwCreated;
    pInsert[8] = (void*)&bFwd;

    g_pUserTable->insertData(pInsert);

    delete pRS;

    return true;
  };

  delete pRS;

  return false;
};



bool GetUserNameByID(uint32_t dwUserID, char *szUserName, int nMaxLen)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(g_pUserTable, dwUserID);

  if(rs.haveValidEntry() == true)
  {
    rs.getData(0, (byte*)szUserName, nMaxLen);
    
    return true;
  };
    
  return false;
};


uint32_t  GetNodeIdByName(char *szNodeName)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(g_pNodeTable);
  char                szTemp[30];
  uint32_t            dwTemp;
  
  while(rs.haveValidEntry() == true)
  {
    memset(szTemp, 0, sizeof(szTemp));
    
    rs.getData(1, (byte*)&szTemp, sizeof(szTemp));
    
    if(strcasecmp(szTemp, szNodeName) == 0)
    {
      rs.getData(0, (byte*)&dwTemp, sizeof(dwTemp));
      
      return dwTemp;
    };

    ResetWatchDog();
    rs.moveNext();
  };
    
  return 0;  
};


bool FindDeviceByNodeName(CWSFFileDBRecordset *pRS, char *szNodeName)
{
  //variables
  ///////////
  char szTemp[30];
  
  Serial.print("FindDeviceByNodeName: Search for ");
  Serial.println(szNodeName);
  
  if(pRS->moveFirst() == true)
  {
    do 
    {
      if(pRS->haveValidEntry() == true)
      {
        if(pRS->getData(1, (void*)&szTemp, sizeof(szTemp)) == true)
        {
          if(strcasecmp(szTemp, szNodeName) == 0)
          {
            Serial.print("FindDeviceByNodeName: found ");
            Serial.println(szNodeName);
    
            return true;
          };
        };
      };
      
      ResetWatchDog();
    }
    while(pRS->moveNext() == true);
  };

  Serial.print("FindDeviceByNodeName: not found ");
  Serial.println(szNodeName);
  
  return false;  
};


bool updateLastHeard(uint32_t dwNodeID, uint32_t dwTime)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(g_pNodeTable);

  if(FindDeviceByNodeID(&rs, dwNodeID) == true)
  {
    rs.setData(6, (void*)&dwTime, sizeof(dwTime));

    return true;
  };

  return false;
};


bool FindDeviceByNodeID(CWSFFileDBRecordset *pRS, uint32_t dwNodeID)
{
  //variables
  ///////////
  uint32_t dwTemp;
  
  Serial.print("FindDeviceByNodeID: Search for ");
  Serial.println(dwNodeID);
  
  if(pRS->moveFirst() == true)
  {
    while(pRS->haveValidEntry() == true) 
    {
      if(pRS->getData(0, (void*)&dwTemp, sizeof(dwTemp)) == true)
      {
        if(dwTemp == dwNodeID)
        {
          Serial.print("FindDeviceByNodeID: found ");
          Serial.println(dwNodeID);
  
          return true;
        };
      };
      
      ResetWatchDog();
      pRS->moveNext();
    };
  };

  Serial.print("FindDeviceByNodeID: not found ");
  Serial.println(dwNodeID);
  
  return false;
};










bool FindContactByName(CWSFFileDBRecordset *pRS, char *szUserName, char *szDeviceName)
{
  //variables
  ///////////
  char szTemp[30];

  while(pRS->haveValidEntry() == true)
  {
    memset(szTemp, 0, sizeof(szTemp));
    
    if(pRS->getData(2, (byte*)&szTemp, sizeof(szTemp)) == true)
    {
      if(strcasecmp((char*)szTemp, szUserName) == 0)
      {
        memset(szTemp, 0, sizeof(szTemp));
        
        if(pRS->getData(3, (byte*)&szTemp, sizeof(szTemp)) == true)
        {
          if(strcasecmp((char*)szTemp, szDeviceName) == 0)
          {
            return true;
          };
        };  
      };
    };

    ResetWatchDog();
    pRS->moveNext();
  };

  return false;
};



bool     FindContactByID(CWSFFileDBRecordset *pRS, uint32_t dwUserID, uint32_t dwDeviceID)
{
  //variables
  ///////////
  uint32_t dwTemp = 0, dwTemp1 = 0;

  while(pRS->haveValidEntry() == true)
  {
    pRS->getData(0, (byte*)&dwTemp, sizeof(dwTemp));
    pRS->getData(1, (byte*)&dwTemp1, sizeof(dwTemp1));
  
    if((dwTemp == dwUserID) && (dwTemp1 == dwDeviceID))
    {
      return true;
    };
       
    ResetWatchDog();
    pRS->moveNext();
  };

  return false; 
};


bool     isContactBlocked(uint32_t dwLocalUserID, uint32_t dwRemoteUserID, uint32_t dwRemoteDeviceID)
{
  //variables
  ///////////
  CWSFFileDB          *pDatabase;
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  int                 nBlocked = 0;

  memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
  sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, dwLocalUserID);

  Serial.print(F("open user contacts db: "));
  Serial.println(szDatabaseFile);
        
  pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);  
  pDatabase->open();
        
  if(pDatabase->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(pDatabase);

    if(FindContactByID(pRecordset, dwRemoteUserID, dwRemoteDeviceID) == true)
    {
      pRecordset->getData(5, (void*)&nBlocked, sizeof(nBlocked));
    };

    delete pRecordset;
  };

  delete pDatabase;

  return (nBlocked == 0 ? false : true);
};



uint32_t InsertContactForUser(uint32_t dwForUser, uint32_t dwUserID, uint32_t dwDeviceID, char *szUserName, char *szDevName, int nContactState)
{
  //variables
  ///////////
  CWSFFileDB          *pDatabase;
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  uint32_t            dwContID = 0;
  void                *pInsert[CONTACTSTABLE_SIZE + 1];
  int                 nBlocked = 0;
  
  memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
  sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, dwForUser);

  Serial.print(F("open user contacts db: "));
  Serial.println(szDatabaseFile);
        
  pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);  
  pDatabase->open();
        
  if(pDatabase->isOpen() == true)
  {
    pInsert[0] = (void*)&dwUserID;
    pInsert[1] = (void*)&dwDeviceID;
    pInsert[2] = (void*)szUserName;
    pInsert[3] = (void*)szDevName;
    pInsert[4] = (void*)&nContactState;
    pInsert[5] = (void*)&nBlocked;
    pInsert[6] = (void*)&nBlocked;

    pDatabase->insertData(pInsert);
    dwContID = pDatabase->getLastInsertPos();

    Serial.print(F("inserted new contact: "));
    Serial.println(dwContID);
  }
  else
  {
    Serial.println(F("Unable to open user contacts db"));
  };
  
  delete pDatabase;

  return dwContID;
};



bool initDatabase(fs::FS &fs)
{
  g_pUserTable = new CWSFFileDB((fs::FS*)&fs, USERTABLE_FILE, (int*)&nUserTableDef, USERTABLE_SIZE, true, DBRESERVE_USER_COUNT);

  if(g_pUserTable->open() == true)
  {
    Serial.println(F("UserTable: DB Open!"));
    g_pUserTable->check();
  }
  else
  {
    return false;
  };


  g_pNodeTable = new CWSFFileDB((fs::FS*)&fs, NODETABLE_FILE, (int*)&nNodeTableDef, NODETABLE_SIZE, true, DBRESERVE_NODE_COUNT);

  if(g_pNodeTable->open() == true)
  {
    Serial.println(F("NodeTable: DB Open!"));
    g_pNodeTable->check();
  }
  else
  {
    return false;
  };


  g_pShoutOutTable = new CWSFFileDB((fs::FS*)&fs, SHOUTOUTTABLE_FILE, (int*)&nShoutOutTableDef, SHOUTOUTTABLE_SIZE, true, DBRESERVE_SHOUT_COUNT);

  if(g_pShoutOutTable->open() == true)
  {
    Serial.println(F("ShoutOutTable: DB Open!"));
    g_pShoutOutTable->check();
  }
  else
  {
    return false;
  };


  g_pDataHeaderTable = new CWSFFileDB(&LORALINK_DATA_FS, DATAHEADERTABLE_FILE, (int*)&nDataHeaderTableDef, DATAHEADERTABLE_SIZE, true, DBRESERVE_DATA_COUNT);

  if(g_pDataHeaderTable->open() == true)
  {
    Serial.println(F("DataHeader: DB Open!"));
    g_pDataHeaderTable->check();
  }
  else
  {
    return false;
  };

  g_pDataTable = new CWSFFileDB(&LORALINK_DATA_FS, DATATABLE_FILE, (int*)&nDataTableDef, DATATABLE_SIZE, true, DBRESERVE_DDATA_COUNT);

  if(g_pDataTable->open() == true)
  {
    Serial.println(F("DataData: DB Open!"));
    g_pDataTable->check();
  }
  else
  {
    return false;
  };



  return true;
};





//chat related functions
////////////////////////
bool FindChatForContactID(CWSFFileDBRecordset *pRS, uint32_t dwContactID)
{
  //variables
  ///////////
  uint32_t dwTemp;

  
  while(pRS->haveValidEntry() == true)
  {
    pRS->getData(6, (void*)&dwTemp, sizeof(dwTemp));

    if(dwTemp == dwContactID)
    {
      return true;
    };
      
    pRS->moveNext();
    ResetWatchDog();
  };

  return false;
};



bool      deleteChatByID(uint32_t dwMsgHeadID, uint32_t dwUserID)
{
  //variables
  ///////////
  CWSFFileDB          *pDatabase;
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  bool                bRes = false;
  uint32_t            dwTemp;
  
  
  memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
  sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, dwUserID);

  pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, false, DBRESERVE_CHAT_COUNT);  
  pDatabase->open();
    
  if(pDatabase->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(pDatabase, dwMsgHeadID);

    if(pRecordset->haveValidEntry() == true)
    {
      pRecordset->remove();
      bRes = true;
    };

    delete pRecordset;
  };

  delete pDatabase;

  if(bRes == true)
  {
    deleteChatMessagesByID(dwMsgHeadID, dwUserID, false);
  };

  return bRes;
};




bool      deleteChatMessagesByID(uint32_t dwMsgHeadID, uint32_t dwUserID, bool bUpdate)
{
  //variables
  ///////////
  CWSFFileDB          *pDatabase;
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  bool                bRes = false;
  uint32_t            dwTemp;
  int                 nTemp;
  
  //remove from msgs table
  memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
  sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, dwUserID);

  pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, false, DBRESERVE_CHATMSG_COUNT);  
  pDatabase->open();
    
  if(pDatabase->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(pDatabase);

    while(pRecordset->haveValidEntry() == true)
    {
      pRecordset->getData(0, (void*)&dwTemp, sizeof(dwTemp));
  
      if(dwTemp == dwMsgHeadID)
      {
        pRecordset->remove();
      };
        
      pRecordset->moveNext();
      ResetWatchDog();
    };

    delete pRecordset;
  };

  delete pDatabase;

  if(bUpdate == true)
  {
    //check if we need to reset last msg and number of new messages
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, dwUserID);
  
    pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, false, DBRESERVE_CHAT_COUNT);  
    pDatabase->open();
      
    if(pDatabase->isOpen() == true)
    {
      pRecordset = new CWSFFileDBRecordset(pDatabase, dwMsgHeadID);
  
      if(pRecordset->haveValidEntry() == true)
      {
        dwTemp = 0;
        nTemp  = 0;
  
        pRecordset->setData(3, (void*)&dwTemp, sizeof(dwTemp));
        pRecordset->setData(5, (void*)&nTemp, sizeof(nTemp));
      };
  
      delete pRecordset;
    };
  
    delete pDatabase;
  };


  return true;
};










//data transfer
///////////////


bool isDataTransferHeaderConfirmed(uint32_t dwHeaderID)
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  bool                bRes = false;
  int                 nTemp;
          
  if(g_pDataHeaderTable->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(g_pDataHeaderTable, dwHeaderID);

    if(pRecordset->haveValidEntry() == true)
    {
      pRecordset->getData(12, (void*)&nTemp, sizeof(nTemp));
      
      bRes = (nTemp == 1 ? true : false);
    };

    delete pRecordset;
  };

  return bRes;
};


bool removeDataTransferTablesHeader(uint32_t dwHeaderID)
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  bool                bRes = false;
  uint32_t            dwTemp;
  
  if(g_pDataHeaderTable->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(g_pDataHeaderTable, dwHeaderID);

    if(pRecordset->haveValidEntry() == true)
    {
      pRecordset->remove();
      bRes = true;
    };

    delete pRecordset;
  };

  if(bRes == true)
  {
    //remove data
    if(g_pDataTable->isOpen() == true)
    {
      pRecordset = new CWSFFileDBRecordset(g_pDataTable);
  
      while(pRecordset->haveValidEntry() == true)
      {
        dwTemp = 0;

        pRecordset->getData(0, (void*)&dwTemp, sizeof(dwTemp));

        if(dwTemp == dwHeaderID)
        {
          pRecordset->remove();
          LLSystemState.lBlocksToTransfer -= 1;
        };

        pRecordset->moveNext();
        ResetWatchDog();
      };
  
      delete pRecordset;
    };
  };

  return bRes;
};


uint32_t getDataHeadIdBySourceAndFileId(CWSFFileDB *pDB, uint32_t dwSourceID, uint32_t dwFileID)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(pDB);
  uint32_t            dwTmpSrcID;
  uint32_t            dwTmpFileID;
  
  while(rs.haveValidEntry() == true)
  {
    rs.getData(4, (void*)&dwTmpSrcID, sizeof(dwTmpSrcID));
    rs.getData(9, (void*)&dwTmpFileID, sizeof(dwTmpFileID));

    if((dwTmpSrcID == dwSourceID) && (dwFileID == dwTmpFileID))
    {
      return rs.getRecordPos();
    };

    rs.moveNext();
    ResetWatchDog();
  };

  return 0;
};


bool     FindEntryForFileIdAndBlockNumber(CWSFFileDBRecordset *pRS, uint32_t dwFileID, int nBlockNumber)
{
  //variables
  ///////////
  int       nTempBlockNumber;
  uint32_t  dwTempID;
  
  while(pRS->haveValidEntry() == true)
  {
    pRS->getData(0, (void*)&dwTempID, sizeof(dwTempID));
    pRS->getData(1, (void*)&nTempBlockNumber, sizeof(nTempBlockNumber));

    if((dwTempID == dwFileID) && (nBlockNumber == nTempBlockNumber))
    {
      return true;
    };
                
    pRS->moveNext();
    ResetWatchDog();
  };


  return false;
};



int      assembleMessageData(byte *pData, int nMaxLen, uint32_t dwFileID)
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  char                szDatabaseFile[55];
  int                 nBlock = 0;
  int                 nComplete;
  int                 nTempBlock;
  uint32_t            dwTempID;
  int                 nBlockSize;
  int                 nPos = 0;
  
          
  if(g_pDataTable->isOpen() == true)
  { 
    pRecordset = new CWSFFileDBRecordset(g_pDataTable);

    while(pRecordset->haveValidEntry() == true)
    {
      pRecordset->getData(0, (void*)&dwTempID, sizeof(dwTempID));
      pRecordset->getData(1, (void*)&nTempBlock, sizeof(nTempBlock));
      pRecordset->getData(2, (void*)&nComplete, sizeof(nComplete));
      pRecordset->getData(3, (void*)&nBlockSize, sizeof(nBlockSize));

      if((nTempBlock == nBlock) && (dwTempID == dwFileID))
      {
        if(nComplete == 1)
        {
          pRecordset->getData(4, (void*)pData + nPos, DATATABLE_BLOCKSIZE);
  
          nBlock += 1;
          nPos   += nBlockSize;
        }
        else
        {
          nPos = 0;
        };
      };

      pRecordset->moveNext();
      ResetWatchDog();
    };
   
    delete pRecordset;
  };

  return nPos;
};



uint32_t addDataToTransferTables(byte *pData, int nDataLen, int nFileType, uint32_t dwDestinationDev, uint32_t dwSourceDev, char *szSender, char *szRcptTo, uint32_t dwRemoteFileID, int nTransferEnabled, int nDirection, uint32_t dwLocalMessageID, uint32_t dwResponsibleUserID)
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  uint32_t            dwTime = ClockPtr->GetCurrentTime().unixtime();
  char                szDatabaseFile[55];
  uint32_t            dwDataHeadID = 0;
  void                *pInsert[11];
  int                 nFileComplete = 0;
  int                 nBlock = 0;
  int                 nPos = 0;
  byte                bData[DATATABLE_BLOCKSIZE + 1];
  int                 nBlockSize = DATATABLE_BLOCKSIZE;
        
  if(g_pDataHeaderTable->isOpen() == true)
  {
    pInsert[0] = (void*)szRcptTo;
    pInsert[1] = (void*)szSender;
    pInsert[2] = (void*)&dwTime;
    pInsert[3] = (void*)&nDataLen;
    pInsert[4] = (void*)&dwSourceDev;
    pInsert[5] = (void*)&dwDestinationDev;
    pInsert[6] = (void*)&dwTime;
    pInsert[7] = (void*)&nFileType;
    pInsert[8] = (void*)&nFileComplete;
    pInsert[9] = (void*)&dwRemoteFileID;
    pInsert[10] = (void*)&nTransferEnabled;
    pInsert[11] = (void*)&nDirection;
    pInsert[12] = (void*)&nBlock;
    pInsert[13] = (void*)&nBlockSize;
    pInsert[14] = (void*)&dwLocalMessageID;
    pInsert[15] = (void*)&dwResponsibleUserID;

    if(g_pDataHeaderTable->insertData(pInsert) == true)
    {
      dwDataHeadID = g_pDataHeaderTable->getLastInsertPos();
      
      Serial.print(F("inserted new file: "));
      Serial.println(dwDataHeadID);
  
      //insert data blocks            
      if(g_pDataTable->isOpen() == true)
      { 
        while(nPos < nDataLen)
        {
          if((nDataLen - nPos) < nBlockSize)
          {
            nBlockSize = nDataLen - nPos;  
          };
  
          memcpy(&bData, pData + nPos, nBlockSize); 
  
          pInsert[0] = (void*)&dwDataHeadID;
          pInsert[1] = (void*)&nBlock;
          pInsert[2] = (void*)&nFileComplete;
          pInsert[3] = (void*)&nBlockSize;
          pInsert[4] = (void*)&bData;
  
          g_pDataTable->insertData(pInsert);
  
          Serial.print(F("block saved to database: "));
          Serial.print(nBlock);
          Serial.print(F(" pos: "));
          Serial.print(nPos);
          Serial.print(F(" size: "));
          Serial.println(nBlockSize);
          
          nPos                             += nBlockSize;
          nBlock                           += 1;
          LLSystemState.lBlocksToTransfer  += 1;
          
          ResetWatchDog();
        };
  
        Serial.print(F("Data saved to database: "));
        Serial.println(nBlock);
      }
      else
      {
        Serial.println(F("Unable to open data db"));
    
        removeDataTransferTablesHeader(dwDataHeadID);
  
        dwDataHeadID = 0;
      };  
    }
    else
    {
      Serial.println(F("Unable to write data header"));
  
      removeDataTransferTablesHeader(dwDataHeadID);

      dwDataHeadID = 0;
    };
  }
  else
  {
    Serial.println(F("Unable to open data header db"));
  };

  return dwDataHeadID;
};





void     limitShoutEntrys(int nLimit)
{
  //variables
  ///////////
  uint32_t             dwTime = millis(), dwTemp = 0, dwPos = 0;
  CWSFFileDBRecordset *pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable);

  if(g_pShoutOutTable->getRecordCount() >= nLimit)
  {
    while(pRecordset->haveValidEntry() == true)
    {
      pRecordset->getData(1, (void*)&dwTemp, sizeof(dwTemp));
  
      if(dwTemp <= dwTime)
      {
        dwTime = dwTemp;
        dwPos  = pRecordset->getRecordPos();
      };

      pRecordset->moveNext();
    };

    if(dwPos > 0)
    {
      Serial.println(F("DB: Remove SO (limit)"));
      
      delete pRecordset;

      pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable, dwPos);
      pRecordset->remove();

      return;
    };
  };

  delete pRecordset;
};




void    openTrackDataDB() 
{
  //variables
  ///////////
  char szDatabaseFile[50];
  
  if(g_pTrackDataTable == NULL)
  {
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, TRACKWAYPOINTTABLE_FILE);
  
    g_pTrackDataTable = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackWaypointTableDef, TRACKWAYPOINTTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
    g_pTrackDataTable->open();
  };
};
