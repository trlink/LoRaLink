#ifndef __WSFHELPER__
#define __WSFHELPER__

//includes
//////////
#include <Arduino.h>
#include <FS.h>


#if LORALINK_HARDWARE_BATSENSE == 1

  void adc_init();
  float battery_read();

#endif


bool CheckIfDeviceExist(int nDevAddr);


String split(String s, char parser, int index);


bool WriteFile(fs::FS &fs, const char *szFile, byte *pData, int nLength);

#endif
