//includes
//////////
#include "CFirmwareUpdater.h"
#include <Update.h>



// perform the actual update from a given stream
bool performUpdate(Stream &updateSource, size_t updateSize) 
{
  //variables
  ///////////
  bool bRes = false;
  
  if(Update.begin(updateSize)) 
  {      
    size_t written = Update.writeStream(updateSource);

    if (written == updateSize) 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println(String(F("Written : ")) + String(written) + String(F(" successfully")));
      #endif
    }
    else 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println(String(F("Written only : ")) + String(written) + String(F("/")) + String(updateSize));
      #endif
    };
    
    if (Update.end()) 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println(F("OTA done!"));
      #endif
      
      if (Update.isFinished()) 
      {
        #if FIRMWARE_UPDATER_DEBUG == 1
          Serial.println(F("Update successfully completed"));
        #endif

        bRes = true;
      }
      else 
      {
        #if FIRMWARE_UPDATER_DEBUG == 1
          Serial.println(F("Update not finished? Something went wrong!"));
        #endif
      }
    }
    else 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println(String(F("Error Occurred. Error #: ")) + String(Update.getError()));
      #endif
    }
  }
  else
  {
    #if FIRMWARE_UPDATER_DEBUG == 1
      Serial.println(F("Not enough space to begin OTA"));
    #endif
  };


  return bRes;
};






bool imageWriter(fs::FS &fs, char *szFileName) 
{
  //variables
  ///////////
  File updateBin = fs.open(szFileName);
  size_t updateSize;
  bool bRes = false;
  
  if (updateBin) 
  {
    if(updateBin.isDirectory())
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.print(F("Error, "));
        Serial.print(szFileName);
        Serial.println(F(" is not a file"));
      #endif
      
      updateBin.close();
      
      return false;
    };

    updateSize = updateBin.size();

    if(updateSize > 0) 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println(F("Try to start update"));
      #endif
       
      bRes = performUpdate(updateBin, updateSize);
    }
    else 
    {
      #if FIRMWARE_UPDATER_DEBUG == 1
        Serial.println("Error, file is empty");
      #endif
    };

    updateBin.close();
  }
  else 
  {
    #if FIRMWARE_UPDATER_DEBUG == 1
      Serial.println(F("Could not load update"));
    #endif
  };

  return bRes;
};
