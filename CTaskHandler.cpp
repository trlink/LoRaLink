//includes
//////////
#include <Arduino.h>
#include "CTaskHandler.h"



void CTaskIF::setInterval(long lExecuteMs)
{
  this->m_lInterval   = lExecuteMs; 
  this->m_lExecTimer  = millis() + this->m_lInterval;
};


bool CTaskIF::isTimedout()
{
  return (this->m_lTimeoutMs <= millis());
};


void CTaskIF::onTimeout()
{
};


void CTaskIF::setTimeout(long lTimeoutMs)
{
  this->m_lTimeoutMs = millis() + lTimeoutMs;
};

bool CTaskIF::doExecute()
{
  if(this->m_lExecTimer <= millis())
  {
    this->m_lExecTimer = millis() + this->m_lInterval;

    return true;
  };

  return false;
};


void CTaskIF::setTaskID(int nTaskID)
{
  this->m_nTaskID = nTaskID;
};



int CTaskIF::getTaskID()
{
  return this->m_nTaskID;
};



CTaskHandler::CTaskHandler()
{
  this->m_nTaskID = 1;
  this->m_pList   = new CWSFLinkedList();
  this->m_mutex   = xSemaphoreCreateMutex();
};


CTaskHandler::~CTaskHandler()
{
  //variables
  ///////////
  CTaskIF *pTask; 
  
  this->m_pList->itterateStart();
  
  do 
  {
    pTask = (CTaskIF*)this->m_pList->getNextItem();

    if(pTask != NULL)
    {
      this->removeTask(pTask->getTaskID());
    
      pTask = (CTaskIF*)this->m_pList->getNextItem();
    };
  } while(pTask != NULL);
};



int CTaskHandler::addTask(CTaskIF *pTask)
{
  #if CTASK_DEBUG == 1
    Serial.println(F("[CTASK] --> addTask: "));
  #endif
  
  xSemaphoreTake(this->m_mutex, portMAX_DELAY);
  
  if(this->m_nTaskID == 250)
  {
    this->m_nTaskID = 10;
  };
  
  pTask->setTaskID(this->m_nTaskID++);
  
  this->m_pList->addItem(pTask);

  xSemaphoreGive(this->m_mutex);


  #if CTASK_DEBUG == 1
    Serial.println(F("[CTASK] <-- addTask: "));
  #endif

  return pTask->getTaskID();
};


    
void CTaskHandler::removeTask(int nTaskID)
{
  //variables
  ///////////
  CTaskIF *pTask; 

  #if CTASK_DEBUG == 1
    Serial.print(F("[CTASK] remove: "));
    Serial.println(nTaskID);
  #endif

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);
  
  this->m_pList->itterateStart();
  
  pTask = (CTaskIF*)this->m_pList->getNextItem();
  
  do 
  {
    if(pTask != NULL)
    {
      if(pTask->getTaskID() == nTaskID)
      {
        #if CTASK_DEBUG == 1
          Serial.print(F("[CTASK] removed: "));
          Serial.println(nTaskID);
        #endif
        
        this->m_pList->removeItem(pTask);

        break;
      };
    };

    pTask = (CTaskIF*)this->m_pList->getNextItem();
    
  } while(pTask != NULL);

  #if CTASK_DEBUG == 1
    Serial.print(F("[CTASK] remove end: "));
    Serial.println(nTaskID);
  #endif

  xSemaphoreGive(this->m_mutex);
};
    


void CTaskHandler::handleTasks()
{
  //variables
  ///////////
  CTaskIF *pTask;

  #if CTASK_TDEBUG == 1
    Serial.println(F("[CTASK] --> handleTasks()"));
  #endif 

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);
  
  pTask = (CTaskIF*)this->m_pList->getNextItem(); 
  
  if(pTask != NULL)
  {
    if(pTask->doExecute() == true)
    {
      #if CTASK_XDEBUG == 1
        Serial.print(F("[CTASK] --> execute Task: "));
        Serial.println(pTask->getTaskID());
      #endif 

      //exit mutex before exec the
      //task, otherwise a deadlock occurs!
      xSemaphoreGive(this->m_mutex);
      
      pTask->handleTask();

      #if CTASK_XDEBUG == 1
        Serial.print(F("[CTASK] <-- execute Task finished: "));
        Serial.println(pTask->getTaskID());
      #endif 

      return;
    }
    else
    {
      xSemaphoreGive(this->m_mutex);
    };
  }
  else
  {
    this->m_pList->itterateStart();

    xSemaphoreGive(this->m_mutex);
  };

  #if CTASK_TDEBUG == 1
    Serial.println(F("[CTASK] <-- handleTasks()"));
  #endif 

  
};
    
  
