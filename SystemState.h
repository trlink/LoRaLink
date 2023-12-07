#ifndef __LLSYSSTATE__
#define __LLSYSSTATE__



//includes
//////////
#include "HardwareConfig.h"


struct _sSystemState 
{
  bool          bSdOk;
  int           nModemState;
  bool          bConnected;
  uint64_t      lIntSpiBytesUsed;
  uint64_t      lIntSpiBytesAvail;
  uint64_t      lExtSpiBytesUsed;
  uint64_t      lExtSpiBytesAvail;
  long          lMemFreeOverall;
  long          lMemFreeModemDataTask;
  long          lMemFreeBlinkTask;
  long          lMemFreeModemTask;
  long          lMemFreeDisplayTask;
  long          lBlocksToTransfer;
  long          lOutstandingMsgs;

  #if LORALINK_HARDWARE_GPS == 1
    bool        bValidSignal;
    float       fAltitude;
    float       fLatitude;
    float       fLongitude;
    uint32_t    dwLastValid;
    char        szGpsTime[32];
    int         nNumSat;
    int         nHDOP;
    float       fSpeed;
    float       fCourse;
  #endif
};



extern _sSystemState LLSystemState;

#endif
