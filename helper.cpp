//includes
//////////
#include "helper.h"
#include <Wire.h>
#include "LoRaLinkConfig.h"
#include "HardwareConfig.h"


#if LORALINK_HARDWARE_BATSENSE == 1

  void adc_init()
  {
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);
  };
  
  
  float battery_read()
  {
    int sum       = analogReadMilliVolts(LORALINK_HARDWARE_BATSENSE_PIN);
    float voltage = (sum / 1000.0) + DeviceConfig.fBattCorrection;
    
    return voltage;
  };
  
#endif


bool CheckIfDeviceExist(int nDevAddr)
{
  Wire.beginTransmission(nDevAddr);
      
  if(Wire.endTransmission() == 0) 
  {
    return true;
  };
    
  return false;
};


bool WriteFile(fs::FS &fs, const char *szFile, byte *pData, int nLength) 
{
  Serial.print(F("Write file: "));
  Serial.print(szFile);

  if(nLength > 0)
  {
    File file = fs.open(szFile, FILE_WRITE);
    
    if(!file)
    {
        Serial.println(F(" - failed to open file for writing"));
        return false;
    }
    
    if(file.write(pData, nLength))
    {
        Serial.println(F(" - file written"));

        return true;
    } 
    else 
    {
        Serial.println(F(" - write failed"));

        return false;
    };
  }
  else
  {
    Serial.println(F(" size 0 - write failed"));

    return false;
  };
};



String split(String s, char parser, int index) 
{
  String rs="";
  int parserIndex = index;
  int parserCnt=0;
  int rFromIndex=0, rToIndex=-1;
  
  while(index >= parserCnt) 
  {
    rFromIndex = rToIndex + 1;
    rToIndex = s.indexOf(parser, rFromIndex);
    
    if(index == parserCnt) 
    {
      if((rToIndex == 0) || (rToIndex == -1)) 
      {
        if(rFromIndex > 0)
        {
          rToIndex = s.length();
        }
        else
        {
          return "";
        };
      };
      
      return s.substring(rFromIndex, rToIndex);
    } 
    else parserCnt++;
  };
  
  return rs;
};
