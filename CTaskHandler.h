#ifndef __TASKHANDLER__
#define __TASKHANDLER__

//includes
//////////
#include <mutex>
#include "CWSFLinkedList.h"



//defines
/////////
#define CTASK_DEBUG 1
#define CTASK_XDEBUG 0
#define CTASK_TDEBUG 0


class CTaskIF
{
  public:

    friend class CTaskHandler;
    
    //this function must be non-blocking
    virtual void handleTask() = 0;

    //set interval in which the task will be executed
    void setInterval(long lExecuteMs);

    //set timeout interval
    void setTimeout(long lTimeoutMs);

    //returns the ID of the task
    int  getTaskID();

    //called when the task timed out
    virtual void onTimeout();
        
  private:

    //variables
    ///////////
    long              m_lInterval = 0;
    long              m_lExecTimer = 0;
    long              m_lTimeoutMs = 0;
    int               m_nTaskID;

    
  protected:
  
    bool doExecute();
    
    void setTaskID(int nTaskID);

    bool isTimedout();
};


      



class CTaskHandler
{
  public:
    CTaskHandler();
    ~CTaskHandler();

    int  addTask(CTaskIF *pTask);
    void removeTask(int nTaskID);
    
    void handleTasks();
    
    
  private:
  
    //variables
    ///////////
    SemaphoreHandle_t m_mutex;
    CWSFLinkedList    *m_pList;
    int               m_nTaskID;
};



#endif
