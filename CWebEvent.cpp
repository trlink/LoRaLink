//includes
//////////
#include "CWebEvent.h"


CWebEvent::CWebEvent()
{
  this->m_pList = new CWSFLinkedList();
  this->m_mutex = xSemaphoreCreateMutex();
};


CWebEvent::~CWebEvent()
{
  vSemaphoreDelete(this->m_mutex);
};


void CWebEvent::addEvent(uint32_t dwUserID, int nEventID)
{
  this->addEvent(dwUserID, nEventID, 0);
}

void CWebEvent::addEventString(uint32_t dwUserID, int nEventID, char *szData)
{
  //variables
  ///////////
  _sWebEvent *pNew  = new _sWebEvent;

  pNew->dwUserID    = dwUserID;
  pNew->nEventID    = nEventID;
  pNew->dwDataID    = 0;
  pNew->lValidUntil = millis() + 3000;
  pNew->szData      = new char[WEB_API_MAX_STRING_SIZE + 1];

  memset(pNew->szData, 0, WEB_API_MAX_STRING_SIZE);
  strcpy(pNew->szData, szData);
  
  //remove events which are 
  //timed out...
  this->clearOldEvents();

  this->m_pList->addItem(pNew);
};

void CWebEvent::addEvent(uint32_t dwUserID, int nEventID, uint32_t dwDataID)
{
  //variables
  ///////////
  _sWebEvent *pNew  = new _sWebEvent;
  _sWebEvent *pRoot;
  

  pNew->dwUserID    = dwUserID;
  pNew->nEventID    = nEventID;
  pNew->dwDataID    = dwDataID;
  pNew->lValidUntil = millis() + 3000;
  pNew->szData      = NULL;
  
  //remove events which are 
  //timed out...
  this->clearOldEvents();
  
  this->m_pList->addItem(pNew);
};


void CWebEvent::clearOldEvents()
{
  //variables
  ///////////
  _sListItem *pList;
  _sWebEvent *pRoot;

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);

  pList = (_sListItem*)this->m_pList->getList();

  if(pList != NULL)
  {
    do
    {
      pRoot = (_sWebEvent*)pList->pItem;
  
      if(pRoot != NULL)
      {
        if(pRoot->lValidUntil < millis())
        {
          this->m_pList->removeItem(pRoot);

          if(pRoot->szData != NULL)
          {
            delete pRoot->szData;
          };

          delete pRoot;
        };
      };

      pList = pList->pNext;
    }
    while(pList != NULL);
  };

  xSemaphoreGive(this->m_mutex);
};


int CWebEvent::getEvent(uint32_t dwUserID, uint32_t *pdwDataID, char *szData)
{
  //variables
  ///////////
  _sWebEvent *pRoot;
  int         nRes = 0;

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);


  *pdwDataID  = 0;
  
  this->m_pList->itterateStart();
  
  do 
  {
    pRoot = (_sWebEvent*)this->m_pList->getNextItem();

    if(pRoot != NULL)
    {
      if(pRoot->dwUserID == dwUserID)
      {
        nRes        = pRoot->nEventID;
        *pdwDataID  = pRoot->dwDataID;

        this->m_pList->removeItem(pRoot);

        if(pRoot->szData != NULL)
        {
          strcpy(szData, pRoot->szData);
          
          delete pRoot->szData;
        };

        delete pRoot;

        break;
      };

      //in case of general events, store the last 
      //event and return it, if no user events are
      //available
      if(pRoot->dwUserID == 0)
      {
        nRes        = pRoot->nEventID;
        *pdwDataID  = pRoot->dwDataID;

        if(pRoot->szData != NULL)
        {
          strcpy(szData, pRoot->szData);
        };
      };

      pRoot = (_sWebEvent*)this->m_pList->getNextItem();
    };
  } while(pRoot != NULL);


  xSemaphoreGive(this->m_mutex);


  //remove events which are 
  //timed out...
  this->clearOldEvents();

  return nRes;
};
