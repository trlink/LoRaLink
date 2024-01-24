//includes
//////////
#include "CLoRaModem.h"
#include "LoRaLinkConfig.h"


int InterfaceToSF(int nValue)
{
  switch(nValue)
  {
    case 6: return LORA_SF6; break;  
    case 7: return LORA_SF7; break;
    case 8: return LORA_SF8; break;
    case 9: return LORA_SF9; break;
    case 10: return LORA_SF10; break;
    case 11: return LORA_SF11; break;
    case 12: return LORA_SF12; break;
  };
};


int InterfaceToCR(int nValue)
{
  switch(nValue)
  {
    case 1: return LORA_CR_4_5; break;  
    case 2: return LORA_CR_4_6; break;
    case 3: return LORA_CR_4_7; break;
    case 4: return LORA_CR_4_8; break;
  };
};


int InterfaceToBandwidth(int nValue)
{
  switch(nValue)
  {
    case 500: return LORA_BW_500; break;  
    case 250: return LORA_BW_250; break;
    case 125: return LORA_BW_125; break;
    case 62: return LORA_BW_062; break;
    case 41: return LORA_BW_041; break;  
    case 31: return LORA_BW_031; break;
    case 20: return LORA_BW_020; break;
    case 15: return LORA_BW_015; break;
    case 10: return LORA_BW_010; break;  
    case 7: return LORA_BW_007; break;
  };
};


CLoRaModem::CLoRaModem(tOnReceivedModemData onRxD, tOnModemStateChanged onState)
{
  #ifdef LORALINK_MODEM_SX126x
    this->m_pLoRaModem     = new SX126XLT();
  #endif
  #ifdef LORALINK_MODEM_SX127x
    this->m_pLoRaModem     = new SX127XLT();
  #endif
  this->m_pHandler       = NULL;
  this->m_RxHandler      = onRxD;
  this->m_nModemState    = MODEM_STATE_IDLE;
  this->m_nLastState     = MODEM_STATE_IDLE;
  this->m_pOnModemState  = onState;
  this->m_bInitOK        = false;
  this->m_lLastRx        = 0;
  this->m_mutex          = xSemaphoreCreateMutex();
  
  randomSeed(analogRead(0));

  //this task is running in a seperate thread
  //even if it supports the task manager IF,
  //i had to put it into a seperate thread to avoid 
  //packet loss...
  this->setTaskID(0);
};





CLoRaModem::~CLoRaModem()
{
  
};




bool CLoRaModem::Init(uint32_t lFreq, int nPower, uint32_t lBandwith, int nSpreadingFactor, int nCodingRate, int nSyncWord, int nPreambleLen)
{
  #ifdef L1_INFO
    Serial.println(F("[Modem] Init LORA Hardware:"));

    #ifdef LORALINK_MODEM_SX126x
      Serial.print(F("\tSX126x @ "));
    #endif
    #ifdef LORALINK_MODEM_SX127x
      Serial.print(F(" SX127x @ "));
    #endif
    
    Serial.print(F("\tFrequency: "));
    Serial.println(lFreq);
    
    Serial.print(F("\tPower: "));
    Serial.println(nPower);
    
    Serial.print(F("\tLoRa Bandwith: "));
    Serial.println(InterfaceToBandwidth(lBandwith));
  
    Serial.print(F("\tSpreadingFactor: "));
    Serial.println(InterfaceToSF(nSpreadingFactor));
  
    Serial.print(F("\tCodingRate: "));
    Serial.println(InterfaceToCR(nCodingRate));
  
    Serial.print(F("\tSyncWord: "));
    Serial.println(nSyncWord);
  #endif

  //https://github.com/JJJS777/Hello_World_RadioLib_Heltec-V3_SX1262
  //https://github.com/StuartsProjects/SX12XX-LoRa
  //http://community.heltec.cn/t/heltec-lora-32-v3-nightmares/12133/3
  SPI.begin(LORALINK_MODEM_SCK, LORALINK_MODEM_MISO, LORALINK_MODEM_MOSI, LORALINK_MODEM_SS);
  
  if(this->m_pLoRaModem->begin(LORALINK_MODEM_SS, LORALINK_MODEM_RST, LORALINK_MODEM_BUSY, LORALINK_MODEM_DI0, -1, LORALINK_MODEM_TYPE) == false) 
  {
    #ifdef L1_INFO
      Serial.println(F("[Modem] TRX err - Init fail"));
    #endif
    
    this->m_bInitOK     = false;
    this->m_nModemState = MODEM_STATE_ERROR;
    this->UpdateModemState();
    
    return false;
  };

  //reset the device
  this->m_pLoRaModem->resetDevice();

  this->m_pLoRaModem->setMode(MODE_STDBY_RC);
  
  #ifdef LORALINK_MODEM_SX126x
    this->m_pLoRaModem->setRegulatorMode(USE_DCDC);
    this->m_pLoRaModem->setPaConfig(0x04, PAAUTO, LORALINK_MODEM_TYPE);
    this->m_pLoRaModem->setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
    this->m_pLoRaModem->setDIO2AsRfSwitchCtrl();
    this->m_pLoRaModem->calibrateDevice(ALLDevices);                //is required after setting TCXO
    this->m_pLoRaModem->calibrateImage(0);
  #endif
  
  this->m_pLoRaModem->setPacketType(PACKET_TYPE_LORA);
  this->m_pLoRaModem->setRfFrequency(lFreq, 0);
  this->m_pLoRaModem->setModulationParams(InterfaceToSF(nSpreadingFactor), InterfaceToBandwidth(lBandwith), InterfaceToCR(nCodingRate), LDRO_AUTO);
  this->m_pLoRaModem->setBufferBaseAddress(0, 0);
  this->m_pLoRaModem->setPacketParams(nPreambleLen, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  this->m_pLoRaModem->setDioIrqParams(IRQ_RADIO_ALL, IRQ_RADIO_ALL/*(IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT + IRQ_CAD_ACTIVITY_DETECTED + IRQ_HEADER_VALID)*/, 0, 0);   //set for IRQ on TX done and timeout on DIO1
  this->m_pLoRaModem->setHighSensitivity();  //set for maximum gain
  this->m_pLoRaModem->setSyncWord(nSyncWord);
  this->m_nTxPower = nPower;

  this->m_pLoRaModem->setupLoRa(lFreq, 0, InterfaceToSF(nSpreadingFactor), InterfaceToBandwidth(lBandwith), InterfaceToCR(nCodingRate), LDRO_AUTO);


  Serial.print(F("[Modem] Read Freq: "));
  Serial.println(this->m_pLoRaModem->getFreqInt());

  if(this->m_pLoRaModem->getFreqInt() == lFreq)
  {
    this->m_bInitOK     = true;
    this->m_nModemState = MODEM_STATE_IDLE;
  };
  
  this->UpdateModemState();
  
  return true;
};


void CLoRaModem::setHandler(void *pHandler)
{
  this->m_pHandler = pHandler;
};


bool CLoRaModem::canTransmit()
{
  if(this->GetModemState() == MODEM_STATE_IDLE)
  {
    return true;
  };

  return false;
};

//this method returns the state of the LoRa-Modem.
//when the modem is IDLE, it checks if a signal was detected,
//if true, it returns that the modem is RECVIVING,
//otherwise TX or IDLE
int  CLoRaModem::GetModemState()
{
  //variables
  ///////////
  int nState = 0;

  xSemaphoreTake(this->m_mutex, portMAX_DELAY);

  if(ModemConfig.bDisableLoRaModem == false)
  {
    if(this->m_bInitOK == true)
    {
      //allways check real receiver state
      //if not transmitting/receiving
      if(this->m_nModemState != MODEM_STATE_TX)
      {
        nState = this->m_pLoRaModem->readIrqStatus();
  
        #ifdef L1_DEBUG
          Serial.print(F("[Modem] IRQ State: "));
          Serial.println(nState);
        #endif
          
        //check if the modem receives something
        if(((nState & IRQ_CAD_ACTIVITY_DETECTED) == IRQ_CAD_ACTIVITY_DETECTED) || ((nState & IRQ_HEADER_VALID) == IRQ_HEADER_VALID))
        {
          #ifdef L1_INFO
              Serial.println(F("[Modem] IRQ state: rx"));
          #endif

          //set RX state, until receive function is finished
          //or timeout in form of rx_delay occurs. The library does not
          //always return from receive() when packet is incomplete...
          this->m_lLastRx     = millis() + RX_TIMEOUT;
          this->m_nModemState = MODEM_STATE_RX;
        };
      };

      if(this->m_nModemState != MODEM_STATE_IDLE)
      {
        //return RX/TX for a short time longer, so that there is
        //a min. delay between transmissions
        if((this->m_lLastRx <= millis()) && (this->m_lLastRx > 0))
        {
          if(this->m_nModemState == MODEM_STATE_RX)
          {
            #ifdef L1_INFO
                Serial.println(F("[Modem] Rx timed out"));
            #endif

            //set modem idle again, after delay is passed and
            //the current state is not idle...
            this->m_nModemState = MODEM_STATE_WAIT_RX;
            this->m_lLastRx     = random(ModemConfig.lModemRxDelay / 2, ModemConfig.lModemRxDelay);
          };
          
          if(this->m_nModemState == MODEM_STATE_WAIT_RX)
          {
            #ifdef L1_INFO
                Serial.println(F("[Modem] Rx Delay passed"));
            #endif

            //set modem idle again, after delay is passed and
            //the current state is not idle...
            this->m_nModemState = MODEM_STATE_IDLE;
            this->m_lLastRx     = 0;
          };

          if(this->m_nModemState == MODEM_STATE_WAIT_TX)
          {
            #ifdef L1_INFO
                Serial.println(F("[Modem] Tx Delay passed"));
            #endif
            
            //set modem idle again, after delay is passed and
            //the current state is not idle...
            this->m_nModemState = MODEM_STATE_IDLE;
            this->m_lLastRx     = 0;
          };
        };        
      };
    }
    else
    {
      #ifdef L1_INFO
          Serial.println(F("[Modem] Modem in error cond"));
      #endif
  
      this->m_nModemState = MODEM_STATE_ERROR;
    };
  }
  else 
  {
    this->m_nModemState = MODEM_STATE_ERROR;
  };

  
  if(this->m_nLastState != this->m_nModemState)
  {
    this->m_nLastState = this->m_nModemState;

    this->UpdateModemState();
  };

  xSemaphoreGive(this->m_mutex);
  
  return this->m_nModemState;
};




void CLoRaModem::handleTask()
{
  //variables
  ///////////
  byte  *pData      = new byte[MAX_DATA_LEN + 1];
  int   nPos        = 0;
  int   nPacketSize = 0;
  int   nRSSI       = 0;
  float fSNR        = 0;
  bool  bDiscard    = false;

  memset(pData, 0, MAX_DATA_LEN);

  this->GetModemState();
  
  if(ModemConfig.bDisableLoRaModem == false)
  {
    //this function blocks for the max amount of time, if it receives something or not...
    nPacketSize = this->m_pLoRaModem->receive(pData, MAX_DATA_LEN, (long)ModemConfig.nTransmissionReceiveTimeout * 1000, WAIT_RX);

    xSemaphoreTake(this->m_mutex, portMAX_DELAY);
    
    //set rx state, if not already done by another 
    //task. set dynamic rx delay also
    this->m_lLastRx     = millis() + random(ModemConfig.lModemRxDelay / 2, ModemConfig.lModemRxDelay);
    this->m_nModemState = MODEM_STATE_WAIT_RX;

    xSemaphoreGive(this->m_mutex);
    
    this->UpdateModemState();

    if(nPacketSize > 0) 
    {   
      #ifdef L1_DEBUG
        Serial.print(F("[Modem] RX: Pckt Len: "));
        Serial.println(nPacketSize);
      #endif

      nRSSI = this->m_pLoRaModem->readPacketRSSI();
      fSNR  = this->m_pLoRaModem->readPacketSNR();
      
      #ifdef L1_INFO
        Serial.print(F("[Modem] RX: size: "));
        Serial.print(nPacketSize);
        Serial.print(F(" RSSI: "));
        Serial.print(nRSSI);  
        Serial.print(F(" SNR: "));
        Serial.println(fSNR);
      #endif
      
      ResetWatchDog();

      if(this->m_RxHandler != NULL)
      {
        this->m_RxHandler(pData, nPacketSize, nRSSI, this->m_pHandler, fSNR);
      };
    };
  };

  delete pData;
};


bool CLoRaModem::sendData(byte *pData, int nSize)
{
  return this->ModemSendData(pData, nSize);
};


bool CLoRaModem::ModemSendData(byte *pData, int nSize)
{
  //variables
  ///////////
  int nPos = 0;
  bool bRes = false;

  #ifdef L1_DEBUG
    Serial.print(F("[Modem] TX size: "));
    Serial.println(nSize);
  #endif

  if(nSize < MAX_DATA_LEN)
  {
    if((ModemConfig.bDisableLoRaModem == false) && (this->m_bInitOK == true)) 
    {
      this->GetModemState();
      
      if(this->m_nModemState == MODEM_STATE_IDLE)
      {
        this->m_lLastRx     = 0;
        this->m_nModemState = MODEM_STATE_TX;
        
        this->UpdateModemState();
        
        nPos = this->m_pLoRaModem->transmit(pData, nSize, ModemConfig.nTransmissionReceiveTimeout * 1000, this->m_nTxPower, WAIT_TX);
        
        if(nPos <= 0)
        {
          #ifdef L1_INFO
            Serial.print(F("[Modem] TX err: "));
            Serial.println(nPos);
          #endif
        }
        else 
        {
          #ifdef L1_INFO
            Serial.println(F("[Modem] TX OK"));
          #endif
          
          bRes = true;
        };

        xSemaphoreTake(this->m_mutex, portMAX_DELAY);
        
        //set transmit delay, this gives the modem some time
        //to transmit the internal buffer, switch to RX and check for packets...
        this->m_nModemState = MODEM_STATE_WAIT_TX;
        this->m_lLastRx     = millis() + ModemConfig.lModemTxDelay;

        xSemaphoreGive(this->m_mutex);
        
        this->UpdateModemState();
      }
      else
      {
        #ifdef L1_INFO
          Serial.println(F("[Modem] TX Err not idle"));
        #endif
      };
    }
    else 
    {
      #ifdef L1_INFO
        Serial.println(F("[Modem] TX disabled/err"));
      #endif
    };
  }
  else
  {
    #ifdef L1_INFO
      Serial.print(F("[Modem] TX size err: "));
      Serial.println(nSize);
    #endif
  };

  return bRes;
};



long CLoRaModem::GetFrequencyError(void)
{
  return this->m_pLoRaModem->getFrequencyErrorHz();
};



void CLoRaModem::UpdateModemState()
{
  if(this->m_pOnModemState != NULL)
  {
    this->m_pOnModemState(this->m_nModemState);
  };
};


void CLoRaModem::closeConnection()
{
};
