//includes
//////////
#include "CWSFLinkedList.h"



CWSFLinkedList::CWSFLinkedList()
{
  this->m_pList         = NULL;
  this->m_pCurrent      = NULL;  
  this->m_mutex         = xSemaphoreCreateMutex();
  this->m_dwItemCount   = 0;
};


CWSFLinkedList::~CWSFLinkedList()
{
  vSemaphoreDelete(this->m_mutex);
};


uint32_t CWSFLinkedList::getItemCount()
{
  return this->m_dwItemCount;
};

bool CWSFLinkedList::addItem(void *pItem)
{
  //variables
  ///////////
  _sListItem *pNew  = new _sListItem;
  _sListItem *pRoot;

  #if WSFLINKEDLIST_DEBUG == 1
    Serial.print(F("[WSFLL] --> addItem("));
    Serial.print((uint32_t)pItem);
    Serial.println(F(")"));
  #endif
  
  pNew->pItem = pItem;
  pNew->pNext = NULL;

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);

  pRoot                  = this->m_pList;
    
  if(this->m_pList == NULL)
  {
    this->m_pList       = pNew;
    this->m_dwItemCount = 1;
  }
  else
  {
    while(pRoot != NULL)
    {
      if(pRoot->pNext == NULL)
      {
        pRoot->pNext        = pNew;
        this->m_dwItemCount += 1;

        #if WSFLINKEDLIST_DEBUG == 1
          Serial.print(F("[WSFLL] item added at "));
          Serial.println(this->m_dwItemCount);
        #endif

        break;    
      };
      
      pRoot = pRoot->pNext;
    };
  };

  xSemaphoreGive(this->m_mutex);

  #if WSFLINKEDLIST_DEBUG == 1
    Serial.println(F("[WSFLL] <-- addItem"));
  #endif

  return true;
};



bool CWSFLinkedList::removeItem(void *pItem)
{
  //variables
  ///////////
  _sListItem *pRoot;
  _sListItem *pDel;

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);

  #if WSFLINKEDLIST_DEBUG == 1
    Serial.print(F("[WSFLL] --> removeItem("));
    Serial.print((uint32_t)pItem);
    Serial.println(F(")"));
  #endif

  if(pItem != NULL)
  {
    pRoot = this->m_pList;
  
    if(pRoot != NULL)
    {
      if(pRoot->pItem == pItem)
      {
        pDel                = pRoot;
        this->m_pList       = pRoot->pNext;

        if(this->m_pList == NULL)
        {
          this->m_dwItemCount = 0;
        }
        else 
        {
          this->m_dwItemCount -= 1;
        };
    
        if(this->m_pCurrent == pDel)
        {
          this->m_pCurrent    = NULL;
        };
    
        delete pDel;
    
        xSemaphoreGive(this->m_mutex);
  
        #if WSFLINKEDLIST_DEBUG == 1
          Serial.println(F("[WSFLL] <-- removeItem"));
        #endif
        
        return true;
      }
      else
      {
        while(pRoot->pNext != NULL)
        {
          if(pRoot->pNext->pItem == pItem)
          {
            pDel          = pRoot->pNext;
            pRoot->pNext  = pDel->pNext;
    
            if(this->m_pCurrent == pDel)
            {
              this->m_pCurrent = NULL;
            };
    
            this->m_dwItemCount -= 1;
    
            delete pDel;
    
            xSemaphoreGive(this->m_mutex);
  
            #if WSFLINKEDLIST_DEBUG == 1
              Serial.println(F("[WSFLL] <-- removeItem"));
            #endif
    
            return true;
          };
    
          pRoot = pRoot->pNext;
        };
      };
    };
  };

  #if WSFLINKEDLIST_ERROR == 1
    Serial.println(F("[WSFLL] Unable to remove item from List"));
  #endif
  
  xSemaphoreGive(this->m_mutex);

  #if WSFLINKEDLIST_DEBUG == 1
    Serial.println(F("[WSFLL] <-- removeItem"));
  #endif

  return false;
};


_sListItem* CWSFLinkedList::getList()
{
  return this->m_pList;
};

    
void CWSFLinkedList::itterateStart()
{
  xSemaphoreTake(this->m_mutex, portMAX_DELAY);
  this->m_pCurrent = this->m_pList;
  xSemaphoreGive(this->m_mutex);
};


void* CWSFLinkedList::getCurrentItem()
{
  return this->m_pCurrent->pItem;
};

void* CWSFLinkedList::getNextItem()
{
  //variables
  ///////////
  _sListItem *pDel;
  
  if(this->m_pCurrent != NULL)
  {
    xSemaphoreTake(this->m_mutex, portMAX_DELAY);
    
    pDel             = this->m_pCurrent;
    this->m_pCurrent = this->m_pCurrent->pNext;
    
    xSemaphoreGive(this->m_mutex);

    return pDel->pItem;
  };

  return NULL;
};
    
