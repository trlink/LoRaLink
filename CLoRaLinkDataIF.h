#ifndef __LORALINKDATAIF__
#define __LORALINKDATAIF__


//includes
//////////
#include "CTaskHandler.h"





/**
 * This interface class can be used to implement new "modem" connections
 * You need to derive from this class and implement the minimal set of functions
 * provided by the class. 
 * 
 * the recieved data can be handled using a callback function (see CLoraModem-Class).
 * 
 * The TaskIF provides functions to handle the modem over the Task-Manager class, 
 * for this you need to check the TaskIF
 */
class CLoRaLinkDataIF : public CTaskIF
{
  public:
  
    virtual bool sendData(byte *pData, int nLength) = 0;
    virtual void closeConnection() = 0;
    virtual bool canTransmit() = 0;
};




#endif
