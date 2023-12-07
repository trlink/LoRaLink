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
  #ifdef LORALINK_HARDWARE_TBEAM

    #if LORALINK_HARDWARE_OLED == 1
      g_display_iic.beginTransmission(nDevAddr);
      
      if(g_display_iic.endTransmission() == 0) 
      {
        return true;
      };
    #else
      g_axp_iic.beginTransmission(nDevAddr);
      
      if(g_axp_iic.endTransmission() == 0) 
      {
        return true;
      };
    #endif
  #else
    #if LORALINK_HARDWARE_OLED == 1
      g_display_iic.beginTransmission(nDevAddr);
        
      if(g_display_iic.endTransmission() == 0) 
      {
        return true;
      };
    #else
      Wire.beginTransmission(nDevAddr);
        
      if(Wire.endTransmission() == 0) 
      {
        return true;
      };
    #endif
  #endif
  
  return false;
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
