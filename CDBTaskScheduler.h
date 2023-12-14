#ifndef __DBTASKSCHED__
#define __DBTASKSCHED__

//includes
//////////
#include <Arduino.h>
#include "HardwareConfig.h"
#include "CTaskHandler.h"
#include "LoRaLinkConfig.h"
#include "helper.h"
#include "CWSFClockHelper.h"
#include "CLoRaLinkDatabase.h"

//defines
/////////
//#define SCHEDULER_DEBUG
#define TASK_SCHEDULER_TIME       (5 * 1000)



//this callback will be called when the task will be executed
typedef void(*tOnTaskSchedule)(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData);

//this callback will be called, when the schedule will be removed
typedef void(*tOnTaskScheduleRemove)(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData, bool bSuccess);


//compare function
typedef bool(*tHasSchedule)(byte *pScheduleData, int nScheduleType, void *pScheduleStruct);



class CDBTaskScheduler : public CTaskIF
{
  public:
  
    CDBTaskScheduler(tOnTaskSchedule pScheduleCallback, tOnTaskScheduleRemove pScheduleRemoveCallback);
    ~CDBTaskScheduler();

    void      startScheduler();

    uint32_t  findTaskByScheduleType(int nType);
      
    //halt the scheduler when the device is not connected
    //or if schedules are added
    void      haltScheduler(bool bHalt);


    //call this function after startup to reschedule the tasks.
    //when the device has no internet connection and has no time set,
    //it points far in the future, which results in non working tasks,
    //after device reboot or time change
    void      rescheduleAfterTimechange();
    

    /* method to add a schedule:
     * 
     * nScheduleType        Free to use int
     * nReScheduleTime      time in seconds to reschedule the task
     * nMaxTries            max tries till give up (0 for unlimited)
     * pData                data to pass, musst be at least 200 bytes!
     * bEnabled             is the schedule enabled
     */
    uint32_t  addSchedule(int nScheduleType, int nReScheduleTime, int nMaxTries, byte *pData, bool bEnabled);

    //method to remove schedules, calls the callback with the provided success flag
    void      removeSchedule(uint32_t dwScheduleID, bool bSuccess);

    //enables a schedule
    void      enableTask(uint32_t dwScheduleID);

    //returns the binary task data
    int       getTaskData(uint32_t dwScheduleID, byte *pData, int nMaxSize);

    //updates the binary task data
    bool      updateTaskData(uint32_t dwScheduleID, byte *pData, int nMaxSize);   

    //manually invoke task (to speed things up)
    void      invokeTask(uint32_t dwScheduleID);

    //the task handler
    void      handleTask();

    bool      hasSchedule(tHasSchedule hasScheduleFunction, void *pScheduleStructure);

  private:
  
    //variables
    ///////////
    CWSFFileDB           *m_pScheduleTable;
    tOnTaskSchedule       m_pScheduleCallback;
    tOnTaskScheduleRemove m_pScheduleRemoveCallback;
    bool                  m_bSchedulerHalted;
    int                   m_nLastSource;
    long                  m_lNextTaskCheck;
};



#endif
