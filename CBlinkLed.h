#ifndef CBLINKLED
#define CBLINKLED


//includes
//////////
#include <Arduino.h>




struct _sBlinkLed {
  int  nPin;          //led pin
  long lNextChange;   //time when the led will be turned off/on
  int  nOnDuration;   //duration the led is on
  int  nOffDuration;  //duration off
  bool bBlink;        //blinking on/off
  bool bDisabled;     //true if the LED should not blink
  int  nMode;         //0 = Normal blinking, 1 = only x times, 2 = after delay passed, for x times
  long lModeTimer;
  int  nModeCount;
  int  lModeDelay;
  int  nModeCounter;
};



class CBlinkLed 
{
  public:

    //init class with the number of leds to blink
    //used AddLed() to add leds...
    CBlinkLed(int nNumberOfLeds) 
    {
      this->m_nMaxLeds  = nNumberOfLeds;
      this->m_nLedsUsed = 0;
      this->m_pLeds     = new _sBlinkLed*[nNumberOfLeds];
      this->m_bAllOff   = false;
    };


    ~CBlinkLed() 
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        delete this->m_pLeds[n];
      };

      delete m_pLeds;
    };


    //add a led...
    bool AddLed(int nPin, int nOnDuration, int nOffDuration) 
    {
      if(this->m_nMaxLeds > this->m_nLedsUsed) 
      {
        this->m_pLeds[this->m_nLedsUsed] = new _sBlinkLed;
        
        this->m_pLeds[this->m_nLedsUsed]->nPin          = nPin;
        this->m_pLeds[this->m_nLedsUsed]->lNextChange   = millis() + nOnDuration;
        this->m_pLeds[this->m_nLedsUsed]->nOnDuration   = nOnDuration;
        this->m_pLeds[this->m_nLedsUsed]->nOffDuration  = nOffDuration;
        this->m_pLeds[this->m_nLedsUsed]->bBlink        = true;
        this->m_pLeds[this->m_nLedsUsed]->bDisabled     = false;
        this->m_pLeds[this->m_nLedsUsed]->nMode         = 0;
        this->m_pLeds[this->m_nLedsUsed]->lModeTimer    = 0;
        this->m_pLeds[this->m_nLedsUsed]->nModeCount    = 0;
        this->m_pLeds[this->m_nLedsUsed]->lModeDelay    = 0;
        this->m_pLeds[this->m_nLedsUsed]->nModeCounter  = 0;

        pinMode(nPin, OUTPUT);
        digitalWrite(nPin, LOW);
        
        this->m_nLedsUsed += 1;

        return true;
      }
      else 
      {
        return false;
      };
    };


    //returns the number of blinks pending
    //can be used to avoid blocking calls 
    //during blinking 
    int BlinkNumberOfTimesCount(int nPin)
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          return this->m_pLeds[n]->nModeCounter;
        };
      };
    }
    


    //this method enables blinking for an exact number of times
    bool BlinkNumberOfTimes(int nPin, int nBlinkCount)
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          this->m_pLeds[n]->bBlink        = true;
          this->m_pLeds[n]->nMode         = 1;
          this->m_pLeds[n]->nModeCounter  = nBlinkCount * 2;

          return true;
        };
      };

      return false;
    };


    bool BlinkNumberOfTimesRepeat(int nPin, int nBlinkCount, long lIntervalMs)
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          this->m_pLeds[n]->bBlink        = true;
          this->m_pLeds[n]->nMode         = 2;
          this->m_pLeds[n]->nModeCounter  = nBlinkCount * 2;
          this->m_pLeds[n]->nModeCount    = nBlinkCount * 2;
          this->m_pLeds[n]->lModeDelay    = lIntervalMs;
          this->m_pLeds[n]->lModeTimer    = 0;

          return true;
        };
      };

      return false;
    };


    //use this function to change the blink parameters (on/off time)
    //it will not enable or disable blinking, use SetBlinking() to set the LED blinking
    bool ChangeBlinkParameters(int nPin, int nOnDuration, int nOffDuration) 
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          this->m_pLeds[n]->nOnDuration = nOnDuration;
          this->m_pLeds[n]->nOffDuration = nOffDuration;

          return true;
        };
      };

      return false;
    };


    //this method turns a specific led off
    bool DisableLED(int nPin, bool bDisabled)
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          this->m_pLeds[n]->bDisabled = bDisabled;

          //turn led off
          digitalWrite(this->m_pLeds[n]->nPin, LOW);

          return true;
        };
      };

      return false;
    };


    //this method turns all leds off on device level
    bool SetAllLedOff(bool bAllOff) 
    {
      this->m_bAllOff = bAllOff;

      if(this->m_bAllOff == true) 
      {
        for(int n = 0; n < this->m_nLedsUsed; ++n) 
        {
          //reset
          this->m_pLeds[n]->nModeCounter = 0;
          
          //turn led off
          digitalWrite(this->m_pLeds[n]->nPin, LOW);
        };
      };

      return true;
    };
    

    //enable / disable blinking
    bool SetBlinking(int nPin, bool bBlinkOn) 
    {
      for(int n = 0; n < this->m_nLedsUsed; ++n) 
      {
        if(this->m_pLeds[n]->nPin == nPin) 
        {
          if(this->m_pLeds[n]->bDisabled == false)
          {
            if(this->m_pLeds[n]->bBlink != bBlinkOn)
            {
              this->m_pLeds[n]->bBlink = bBlinkOn;
    
              if(this->m_bAllOff == false) 
              {
                if(bBlinkOn == false) 
                {
                  //turn led off
                  digitalWrite(this->m_pLeds[n]->nPin, LOW);
                }
                else 
                {               
                  this->m_pLeds[n]->lNextChange = 0; 
                };
              };
            };
          };
           
          return true;
        };
      };
  
      return false;
    };




    //handler which does blinking, must be called in loop() or in an interrupt routine
    void DoBlink() 
    {
      if(this->m_bAllOff == false) 
      {
        for(int n = 0; n < this->m_nLedsUsed; ++n) 
        {
          switch(this->m_pLeds[n]->nMode)
          {
            //normal blinking
            case 0:
            {
              this->HandlePinState(n);
            };
            break;

            //only x times
            case 1:
            {
              if(this->m_pLeds[n]->nModeCounter > 0)
              {
                //only if state was changed
                if(this->HandlePinState(n) == true)
                {
                  this->m_pLeds[n]->nModeCounter -= 1;
                };
              };
            };
            break;

            //repeatedly blink after time passed
            case 2:
            {
              if(millis() > this->m_pLeds[n]->lModeTimer)
              {
                if(this->m_pLeds[n]->nModeCounter <= 0)
                {                  
                  this->m_pLeds[n]->lModeTimer   = millis() + this->m_pLeds[n]->lModeDelay;
                  this->m_pLeds[n]->nModeCounter = this->m_pLeds[n]->nModeCount;
                  this->m_pLeds[n]->lNextChange  = 0;
                };
              };

              if(this->m_pLeds[n]->nModeCounter > 0)
              {
                //only if state was changed
                if(this->HandlePinState(n) == true)
                {
                  this->m_pLeds[n]->nModeCounter -= 1;
                };
              };
            };
            break;
          };
        };
      };
    };

  private:
  
    //variables
    ///////////
    int          m_nMaxLeds;
    int          m_nLedsUsed;
    _sBlinkLed **m_pLeds;
    bool         m_bAllOff;


    //this method handles the PIN-State, if the LED is not disabled, and the state duration has passed,
    //it will set the output pin high (or low). It returns true if the state was changed
    bool HandlePinState(int nLED)
    {
      if(this->m_bAllOff == false)
      {
        if((this->m_pLeds[nLED]->lNextChange <= millis()) && (this->m_pLeds[nLED]->bBlink == true)) 
        {
          if(this->m_pLeds[nLED]->bDisabled == false)
          {
            if(digitalRead(this->m_pLeds[nLED]->nPin) == HIGH) 
            {           
              this->m_pLeds[nLED]->lNextChange = millis() + this->m_pLeds[nLED]->nOffDuration;     
              digitalWrite(this->m_pLeds[nLED]->nPin, LOW);
 
              return true;
            }
            else 
            {
              this->m_pLeds[nLED]->lNextChange = millis() + this->m_pLeds[nLED]->nOnDuration;     
              digitalWrite(this->m_pLeds[nLED]->nPin, HIGH);
  
              return true;
            };
          };
        };
      };

      return false;
    };
};




#endif
