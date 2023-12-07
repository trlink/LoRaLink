#ifndef __CCLOCKHLPR__
#define __CCLOCKHLPR__


//includes
//////////
#include <Wire.h>
#include "DS3231.h"



//defines
/////////
#define CLOCK_DEBUG             0


#define UPDATE_SOURCE_NONE      0
#define UPDATE_SOURCE_NTP       1
#define UPDATE_SOURCE_RTC       2
#define UPDATE_SOURCE_NETWORK   3
#define UPDATE_SOURCE_EXTERNAL  4
#define UPDATE_SOURCE_WEB       5




//this helper class provides a clock
//it uses a RTC, connected to the i2c bus, or the internal timer,
//depends on the existence of a RTC module.
class CClockHlpr
{
  public:

    CClockHlpr(bool bHaveClock, TwoWire *pWire) 
    {
      this->m_pWireIF       = pWire;
      this->m_ds3231        = NULL;
      this->m_RTC           = NULL;
      this->m_bHaveClock    = bHaveClock;
      this->m_lDeltaClock   = millis();
      this->m_bTimeSet      = false;
      this->m_bUpdateRTC    = false;
      this->m_nUpdateSource = UPDATE_SOURCE_NONE;
      this->m_lNextUpdate   = 0;
      
      if(this->m_bHaveClock == true) 
      {
        this->m_ds3231        = new DS3231(*this->m_pWireIF);
        this->m_RTC           = new RTClib();
      
        //set 24h mode
        this->m_ds3231->setClockMode(false);
        
        this->m_dtStarted     = this->m_RTC->now(*this->m_pWireIF);
        this->m_bTimeSet      = true;
        this->m_nUpdateSource = UPDATE_SOURCE_RTC;

        #if CLOCK_DEBUG == 1
          Serial.print(F("[CLOCK] Init from RTC: "));
          Serial.println(this->GetTimeString());
        #endif
      };
    };




    bool haveRTC()
    {
      return this->m_bHaveClock;
    };

    int getUpdateSource()
    {
      return this->m_nUpdateSource;
    };
    

    //this method sets the dtStarted time, which will be used to calculate time and
    //updates the RTC, if present...
    void SetDateTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSource) 
    {
      this->m_bUpdateRTC    = true; 
      this->m_nUpdateSource = nSource;

      //set internal var
      this->m_dtStarted     = DateTime(nYear, nMonth, nDay, nHour, nMin, 0);
      this->m_dtCurrent     = this->m_dtStarted;   
      this->m_bTimeSet      = true;
      this->m_lDeltaClock   = millis();

      if(this->m_bHaveClock == true) 
      {
        //set RTC clock
        this->m_ds3231->setClockMode(false);
        this->m_ds3231->setYear(this->m_dtStarted.year() - 2000);
        this->m_ds3231->setMonth(this->m_dtStarted.month());
        this->m_ds3231->setDate(this->m_dtStarted.day());
        this->m_ds3231->setHour(this->m_dtStarted.hour());
        this->m_ds3231->setMinute(this->m_dtStarted.minute());
        this->m_ds3231->setSecond(0);
      };


      #if CLOCK_DEBUG == 1
        Serial.print(F("[CLOCK] Updated Time: "));
        Serial.println(this->GetTimeString());
      #endif
    };
    

    //this function updates the current time by using the RTC if present.
    //or calculates the time by using the internal timer.
    void updateCurrentTime() 
    {
      if(this->m_lNextUpdate <= millis())
      {
        if(this->m_bHaveClock == true) 
        {
          this->m_dtCurrent = this->m_RTC->now(*this->m_pWireIF);
        }
        else 
        {
          //calc from last known time
          this->m_dtCurrent = DateTime(this->m_dtStarted.unixtime() + (millis() - this->m_lDeltaClock) / 1000L);
        };

        //access rtc every 5min only
        this->m_lNextUpdate = millis() + (300 * 1000);
      }
      else
      {
        this->m_dtCurrent = DateTime(this->m_dtStarted.unixtime() + (millis() - this->m_lDeltaClock) / 1000L);
      };
    };


    DateTime GetCurrentTime() 
    {
      this->updateCurrentTime();
      return this->m_dtCurrent;
    };


    String GetTimeString() 
    {
      //variables
      ///////////
      char szTemp[20];

      this->updateCurrentTime();
      
      sprintf_P(szTemp, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), 
        this->GetCurrentTime().year(), 
        this->GetCurrentTime().month(), 
        this->GetCurrentTime().day(), 
        this->GetCurrentTime().hour(), 
        this->GetCurrentTime().minute(), 
        this->GetCurrentTime().second());
    
      return String(szTemp);
    };

    //converts a timestamp to a string
    String unixTimestampToString(uint32_t dwTimestamp)
    {
      //variables
      ///////////
      uint32_t dwTime;
      char     szData[30];
      DateTime dtTime;

      memset(szData, 0, sizeof(szData));
      
      dtTime = DateTime((time_t)dwTimestamp);
      sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());

      return String(szData);
    };


    //returns a value representing the minutes passed since 01.01.1970
    uint32_t getUnixTimestamp()
    {
      this->updateCurrentTime();

      return this->m_dtCurrent.unixtime();
    };
    

    bool timeSet()
    {
      return m_bTimeSet;
    };

  private:

    //variables
    ///////////
    bool           m_bHaveClock;
    bool           m_bTimeSet;              //false as long as the time was not set via command, or if a logger is present, the time was not updated
    DateTime       m_dtStarted;             //time when the controller was started and the RTC was queried / the date was set
    DateTime       m_dtCurrent;             //current time
    long           m_lDeltaClock;           //when clock not present, we use this to calc the time
    RTClib        *m_RTC;
    DS3231        *m_ds3231;                //the RTC
    TwoWire       *m_pWireIF;
    bool           m_bUpdateRTC;
    int            m_nUpdateSource;         //source of the update
    long           m_lNextUpdate;
};




extern CClockHlpr *ClockPtr;


#endif
