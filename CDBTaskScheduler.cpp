//includes
//////////
#include "CDBTaskScheduler.h"



CDBTaskScheduler::CDBTaskScheduler(tOnTaskSchedule pScheduleCallback, tOnTaskScheduleRemove pScheduleRemoveCallback)
{
  this->m_pScheduleTable          = NULL;
  this->m_pScheduleCallback       = pScheduleCallback;
  this->m_pScheduleRemoveCallback = pScheduleRemoveCallback;
  this->m_bSchedulerHalted        = true;
  this->m_nLastSource             = UPDATE_SOURCE_NONE;
  this->m_lNextTaskCheck          = 0;
};


CDBTaskScheduler::~CDBTaskScheduler()
{
  delete this->m_pScheduleTable;
};





uint32_t CDBTaskScheduler::addSchedule(int nScheduleType, int nReScheduleTime, int nMaxTries, byte *pData, bool bEnabled)
{
  //variables
  ///////////
  void *pInsert[SCHEDULERTABLE_SIZE + 1];
  uint32_t dwLastExec = 0;
  int nTries = 0;
  int nEnabled = (bEnabled == true ? 1 : 0);

  if(this->m_pScheduleTable != NULL)
  {
    if(this->m_pScheduleTable->isOpen() == true)
    {
      pInsert[0] = (void*)&nScheduleType;
      pInsert[1] = (void*)&dwLastExec;
      pInsert[2] = (void*)&nTries;
      pInsert[3] = (void*)&nMaxTries;
      pInsert[4] = (void*)&nReScheduleTime;
      pInsert[5] = (void*)pData;
      pInsert[6] = (void*)&nEnabled;
      
      this->m_pScheduleTable->insertData(pInsert);
    
      #ifdef SCHEDULER_DEBUG
        Serial.print(F("[Sched] addSchedule() type: "));
        Serial.print(nScheduleType);
        Serial.print(F(", enabled: "));
        Serial.print(bEnabled);
        Serial.print(F(", Max tries: "));
        Serial.print(nMaxTries);
        Serial.print(F(", added at: "));
        Serial.println(this->m_pScheduleTable->getLastInsertPos());
      #endif 
  
      return this->m_pScheduleTable->getLastInsertPos();
    };
  };

  #ifdef SCHEDULER_DEBUG
    Serial.print(F("[Sched] addSchedule() type: "));
    Serial.print(nScheduleType);
    Serial.print(F(", enabled: "));
    Serial.print(bEnabled);
    Serial.println(F(", Failed! "));
  #endif 

  return 0;
};


void CDBTaskScheduler::removeSchedule(uint32_t dwScheduleID, bool bSuccess)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(this->m_pScheduleTable, dwScheduleID);
  int                 nScheduleType;
  uint32_t            dwLastExec;
  int                 nTries;
  int                 nMaxTrys;
  int                 nReSchedule;
  byte                bData[201];
  int                 nEnabled;
  
  if(rs.haveValidEntry() == true)
  {
    rs.getData(0, (void*)&nScheduleType, sizeof(nScheduleType));
    rs.getData(1, (void*)&dwLastExec, sizeof(dwLastExec));
    rs.getData(2, (void*)&nTries, sizeof(nTries));
    rs.getData(3, (void*)&nMaxTrys, sizeof(nMaxTrys));
    rs.getData(4, (void*)&nReSchedule, sizeof(nReSchedule));
    rs.getData(5, (void*)&bData, sizeof(bData));
    rs.getData(6, (void*)&nEnabled, sizeof(nEnabled));

    #ifdef SCHEDULER_DEBUG
      Serial.print(F("[Sched] removeSchedule() type: "));
      Serial.print(nScheduleType);
      Serial.print(F(", id: "));
      Serial.print(dwScheduleID);
      Serial.print(F(", enabled: "));
      Serial.println(nEnabled);
    #endif 

    this->m_pScheduleRemoveCallback(rs.getRecordPos(), nScheduleType, dwLastExec, nTries, nMaxTrys, (byte*)&bData, bSuccess);
  
    rs.remove();
  };
};


void CDBTaskScheduler::enableTask(uint32_t dwScheduleID)
{
  if(dwScheduleID > 0)
  {
    //variables
    ///////////
    int                 nEnabled = 1;
    CWSFFileDBRecordset rs(this->m_pScheduleTable, dwScheduleID);
  
    if(rs.haveValidEntry() == true)
    {
      #ifdef SCHEDULER_DEBUG
        Serial.print(F("[Sched] enableTask() ID: "));
        Serial.println(dwScheduleID);
      #endif 
      
      rs.setData(6, (byte*)&nEnabled, sizeof(nEnabled));
    }
    else
    {
      #ifdef SCHEDULER_DEBUG
        Serial.print(F("[Sched] enableTask() ERROR task not found ID: "));
        Serial.println(dwScheduleID);
      #endif 
    };
  };
};


void CDBTaskScheduler::invokeTask(uint32_t dwScheduleID)
{
  if(dwScheduleID > 0)
  {
    //variables
    ///////////
    CWSFFileDBRecordset rs(this->m_pScheduleTable, dwScheduleID);
    uint32_t            dwLastExec = 0;
      
    if(rs.haveValidEntry() == true)
    {
      #ifdef SCHEDULER_DEBUG
        Serial.print(F("[Sched] invokeTask() ID: "));
        Serial.println(dwScheduleID);
      #endif 
      
      rs.setData(1, (void*)&dwLastExec, sizeof(dwLastExec));

      this->m_lNextTaskCheck = 0;
    };
  };
};



bool     CDBTaskScheduler::hasSchedule(tHasSchedule hasScheduleFunction, void *pScheduleStructure)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(this->m_pScheduleTable);
  int                 nScheduleType;
  byte                *pData = new byte[SCHEDULERTABLE_DATA_SIZE + 1];
  bool                bRes   = false;
  
  while(rs.haveValidEntry() == true)
  {
    rs.getData(0, (void*)&nScheduleType, sizeof(nScheduleType));
    rs.getData(5, (void*)pData, SCHEDULERTABLE_DATA_SIZE);
    
    bRes = hasScheduleFunction(pData, nScheduleType, pScheduleStructure);

    if(bRes == true)
    {
      break;
    };

    rs.moveNext();
  };

  delete pData;

  return bRes;
};

uint32_t CDBTaskScheduler::findTaskByScheduleType(int nType)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(this->m_pScheduleTable);
  int                 nScheduleType;

  while(rs.haveValidEntry() == true)
  {
    rs.getData(0, (void*)&nScheduleType, sizeof(nScheduleType));
    
    if(nScheduleType == nType)
    {
      return rs.getRecordPos();
    };

    rs.moveNext();
  };

  return 0;
};


int CDBTaskScheduler::getTaskData(uint32_t dwScheduleID, byte *pData, int nMaxSize)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(this->m_pScheduleTable, dwScheduleID);
  int                 nScheduleType = -1;
  
  if(rs.haveValidEntry() == true)
  {
    rs.getData(0, (void*)&nScheduleType, sizeof(nScheduleType));
    rs.getData(5, pData, nMaxSize);
  };

  return nScheduleType;
};



bool CDBTaskScheduler::updateTaskData(uint32_t dwScheduleID, byte *pData, int nMaxSize)
{
  //variables
  ///////////
  CWSFFileDBRecordset rs(this->m_pScheduleTable, dwScheduleID);
  
  if(rs.haveValidEntry() == true)
  {
    rs.setData(5, (void*)pData, nMaxSize);

    return true;
  };

  return false;
};


void CDBTaskScheduler::startScheduler()
{
  if(this->m_pScheduleTable == NULL)
  {
    this->m_pScheduleTable = new CWSFFileDB(&LORALINK_DATA_FS, SCHEDULERTABLE_FILE, (int*)&nSchedulerTableDef, SCHEDULERTABLE_SIZE, true, DBRESERVE_SCHED_COUNT);
    this->m_pScheduleTable->open();
    
    if(this->m_pScheduleTable->isOpen() == true)
    {
      this->m_pScheduleTable->check();
      this->m_bSchedulerHalted = false;
  
      #ifdef SCHEDULER_DEBUG
        Serial.println(F("[Sched] DB open!"));
      #endif
    }
    else
    {
      #ifdef SCHEDULER_DEBUG
        Serial.println(F("[Sched] Failed to open DB!"));
      #endif
    };
  };
};



void CDBTaskScheduler::haltScheduler(bool bHalt)
{
  if(bHalt != this->m_bSchedulerHalted)
  {
    #ifdef SCHEDULER_DEBUG
      Serial.print(F("[Sched] set Halt to: "));
      Serial.println(bHalt);
    #endif
    
    this->m_bSchedulerHalted = bHalt;
  };
};


void CDBTaskScheduler::rescheduleAfterTimechange()
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  int                 nScheduleType;
  uint32_t            dwLastExec;
  uint32_t            dwNewTime;

  if(ClockPtr != NULL)
  {
    if(this->m_pScheduleTable == NULL)
    {
      Serial.println(F("[Sched] Open Database"));
      
      this->startScheduler();
    };
    
    #ifdef SCHEDULER_DEBUG
      Serial.print(F("[Sched] rescheduleAfterTimechange() have RTC: "));
      Serial.print(ClockPtr->haveRTC());
      Serial.print(F(", update source: "));
      Serial.println(ClockPtr->getUpdateSource());
    #endif 

    if(this->m_pScheduleTable->isOpen() == true)
    {
      this->m_nLastSource = ClockPtr->getUpdateSource();
      dwNewTime           = ClockPtr->getUnixTimestamp();
      pRecordset          = new CWSFFileDBRecordset(this->m_pScheduleTable);
  
      while(pRecordset->haveValidEntry() == true)
      {
        #ifdef SCHEDULER_DEBUG
          Serial.print(F("[Sched] change rec: "));
          Serial.println(pRecordset->getRecordPos());
        #endif 
        
        pRecordset->setData(1, (void*)&dwNewTime, sizeof(dwNewTime));
        
        pRecordset->moveNext();
      };
  
      delete pRecordset;
    }
    else
    {
      Serial.println(F("[Sched] Schedule DB failed"));
    };
  };
};



void CDBTaskScheduler::handleTask()
{
  //variables
  ///////////
  CWSFFileDBRecordset *pRecordset;
  int                 nScheduleType;
  uint32_t            dwLastExec;
  int                 nTries;
  int                 nMaxTrys;
  int                 nReSchedule;
  byte                bData[201];
  int                 nEnabled;

  if(this->m_lNextTaskCheck < millis())
  {
    this->m_lNextTaskCheck = millis() + TASK_SCHEDULER_TIME;
    
    if((this->m_bSchedulerHalted == false) && (ClockPtr != NULL))
    {
      #ifdef SCHEDULER_DEBUG
        Serial.println(F("[Sched] Check Schedules:"));
      #endif
  
      if(this->m_pScheduleTable != NULL)
      {
        if(this->m_pScheduleTable->isOpen() == true)
        {
          pRecordset = new CWSFFileDBRecordset(this->m_pScheduleTable);
      
          while(pRecordset->haveValidEntry() == true)
          {
            pRecordset->getData(0, (void*)&nScheduleType, sizeof(nScheduleType));
            pRecordset->getData(1, (void*)&dwLastExec, sizeof(dwLastExec));
            pRecordset->getData(2, (void*)&nTries, sizeof(nTries));
            pRecordset->getData(3, (void*)&nMaxTrys, sizeof(nMaxTrys));
            pRecordset->getData(4, (void*)&nReSchedule, sizeof(nReSchedule));
            pRecordset->getData(5, (void*)&bData, sizeof(bData));
            pRecordset->getData(6, (void*)&nEnabled, sizeof(nEnabled));
    
            #ifdef SCHEDULER_DEBUG
              Serial.print(F("[Sched] check Schedule "));
              Serial.print(pRecordset->getRecordPos());
              Serial.print(F(": type: "));
              Serial.print(nScheduleType);
              Serial.print(F(": time: "));
              Serial.print(dwLastExec);
              Serial.print(F(" / "));
              Serial.print(ClockPtr->getUnixTimestamp());
              Serial.print(F(" TTE: "));
              Serial.print((dwLastExec > ClockPtr->getUnixTimestamp() ? dwLastExec - ClockPtr->getUnixTimestamp() : 0));
              Serial.print(F(", enabled: "));
              Serial.println(nEnabled);
            #endif 
      
            if(nEnabled == 1)
            {
              if((nTries < nMaxTrys) || (nMaxTrys == 0))
              {
                if(dwLastExec < ClockPtr->getUnixTimestamp())
                {
                  dwLastExec = ClockPtr->getUnixTimestamp() + (uint32_t)nReSchedule;
                  nTries    += 1;
        
                  pRecordset->setData(1, (void*)&dwLastExec, sizeof(dwLastExec));
                  pRecordset->setData(2, (void*)&nTries, sizeof(nTries));
                  
                  this->m_pScheduleCallback(pRecordset->getRecordPos(), nScheduleType, dwLastExec, nTries, nMaxTrys, (byte*)&bData);
                };
              }
              else
              {
                #ifdef SCHEDULER_DEBUG
                  Serial.print(F("[Sched] remove Schedule (on max): "));
                  Serial.println(pRecordset->getRecordPos());
                #endif 
  
                this->removeSchedule(pRecordset->getRecordPos(), false);
              };
            };
            
            pRecordset->moveNext();
          };
      
          delete pRecordset;
  
          #ifdef SCHEDULER_DEBUG
            Serial.println(F("[Sched] End Check Schedules"));
          #endif
        }
        else
        {
          Serial.println(F("[Sched] Schedule DB failed"));
        };
      }
      else
      {
        Serial.println(F("[Sched] Schedule DB NULL"));
      };
    }
    else
    {
      #ifdef SCHEDULER_DEBUG
        Serial.println(F("[Sched] halted (or clock error)"));
      #endif
    };
  };
};
