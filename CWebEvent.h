#ifndef __CWEBEVENT__
#define __CWEBEVENT__


//includes
//////////
#include <Arduino.h>
#include "CWSFLinkedList.h"
#include <mutex>



#define WEB_API_MAX_STRING_SIZE 250


struct _sWebEvent
{
  int         nEventID;
  long        lValidUntil;
  uint32_t    dwUserID;
  uint32_t    dwDataID;
  char       *szData;
};


class CWebEvent
{
  public:
    CWebEvent();
    ~CWebEvent();

    void addEvent(uint32_t dwUserID, int nEventID);
    void addEvent(uint32_t dwUserID, int nEventID, uint32_t dwDataID);
    void addEventString(uint32_t dwUserID, int nEventID, char *szData);

    int  getEvent(uint32_t dwUserID, uint32_t *pdwDataID, char *szData);

    void clearOldEvents();

  private:
    //variables
    ///////////
    CWSFLinkedList    *m_pList;
    SemaphoreHandle_t  m_mutex;
};


extern CWebEvent *g_pWebEvent;

#endif
