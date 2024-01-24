#ifndef __WSFHELPER__
#define __WSFHELPER__

//includes
//////////
#include <Arduino.h>
#include <FS.h>



bool CheckIfDeviceExist(int nDevAddr);


String split(String s, char parser, int index);


bool WriteFile(fs::FS &fs, const char *szFile, byte *pData, int nLength);

#endif
