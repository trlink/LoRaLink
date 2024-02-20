#ifndef __CSKYNETMODEM__
#define __CSKYNETMODEM__ 



//defines
/////////
//#define L1_DEBUG
//#define L1_INFO
#define RX_TIMEOUT        3000  /*interval in which the CAD has to return RX, otherwise it's idle*/
#define MAX_DATA_LEN      255   /*max size of a lora packet*/


//modem states
#define MODEM_STATE_IDLE      1   //free 
#define MODEM_STATE_TX        2   //transmitting
#define MODEM_STATE_RX        3   //receiving
#define MODEM_STATE_ERROR     4   //error state
#define MODEM_STATE_WAIT_TX   5   //waiting after TX
#define MODEM_STATE_WAIT_RX   6   //wait after RX



//includes
//////////
#include <Arduino.h>
#include "HardwareConfig.h"
#include "CLoRaLinkDataIF.h"
#include <mutex>


//callback functions
////////////////////
typedef void(*tOnReceivedModemData)(byte *pData, int nDataLength, int nRSSI, void *pHandler, float fPacketSNR);
typedef void(*tOnModemStateChanged)(int nState);



/**
 * This class handles the LoRa Modem Connection
 * On state change or received data, it calls the callback functions
 */
class CLoRaModem : public CLoRaLinkDataIF
{
  public:
    CLoRaModem(tOnReceivedModemData onRxD, tOnModemStateChanged onState);

    ~CLoRaModem(); 

    void setHandler(void *pHandler);

    bool sendData(byte *pData, int nSize);

    void handleTask();

    bool Init(uint32_t lFreq, int nPower, uint32_t lBandwith, int nSpreadingFactor, int nCodingRate, int nSyncWord, int nPreambleLen);

    bool canTransmit();

    void Receive();

    long GetFrequencyError(void);

    //this method returns the modem state (logical and physical)
    int  GetModemState();


    void closeConnection();


  private:
    //variables
    ///////////
    #ifdef LORALINK_MODEM_SX126x
      SX126XLT             *m_pLoRaModem;
    #endif

    #ifdef LORALINK_MODEM_SX127x
      SX127XLT             *m_pLoRaModem;
    #endif
    
    int                   m_nTxPower;
    void                 *m_pHandler;
    tOnReceivedModemData  m_RxHandler;      //the callback which will be called on RX
    int                   m_nModemState;    //state of modem (used for UI)
    tOnModemStateChanged  m_pOnModemState;  //callback for modem state change events
    bool                  m_bInitOK;
    long                  m_lLastRx;        //time when the last packet was received/transmitted
    int                   m_nLastState;
    SemaphoreHandle_t     m_mutex;
    bool                  m_bTransmit;

    void UpdateModemState();

    bool ModemSendData(byte *pData, int nSize);
};


#endif
