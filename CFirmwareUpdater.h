#ifndef __OTAUPDATE__
#define __OTAUPDATE__


//includes
//////////
#include <Arduino.h>
#include "HardwareConfig.h"
#include <FS.h>


//defines
/////////
#define FIRMWARE_UPDATER_DEBUG      1


bool imageWriter(fs::FS &fs, char *szFileName);



#endif
