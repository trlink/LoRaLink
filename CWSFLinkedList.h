#ifndef __CWSFLINKEDLIST__
#define __CWSFLINKEDLIST__

//includes
//////////
#include <Arduino.h>
#include <mutex>


#define WSFLINKEDLIST_DEBUG 0
#define WSFLINKEDLIST_ERROR 0


struct _sListItem
{
  void        *pItem;
  _sListItem  *pNext;
};


class CWSFLinkedList
{
  public:
    CWSFLinkedList();
    ~CWSFLinkedList();

    //functions to modify the list, these functions
    //are threadsafe
    bool        addItem(void *pItem);
    bool        removeItem(void *pItem);
    uint32_t    getItemCount();

    //functions to itterate over the 
    //list, do NOT modify the list using this functions!
    ////////////////////////////////////////////////////

    //get the list reference (if you need to itterate in multiple threads)
    _sListItem* getList();

    //single threaded list itteration
    void        itterateStart();
    void*       getNextItem();
    void*       getCurrentItem();
    
    

  private:

    //variables
    ///////////
    _sListItem        *m_pList;
    _sListItem        *m_pCurrent;
    SemaphoreHandle_t  m_mutex;
    uint32_t           m_dwItemCount;
};


#endif
