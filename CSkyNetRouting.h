#ifndef __SKYNET_ROUTING__
#define __SKYNET_ROUTING__


//includes
//////////
#include <Arduino.h>
#include "CWSFLinkedList.h"
#include "CSkyNetConnHandler.h"
#include <CWSFFileDB.h>


//defines
/////////
#define SKYNET_ROUTING_INFO  1
#define SKYNET_ROUTING_DEBUG 1
#define SKYNET_ROUTING_ERROR 1


//number of entries stored in RAM
//during tests the device can handle 
//up to 900 entries, but the webserver 
//will stop responding...
//
//we now restrict it to a max of 
//SKYNET_ROUTING_MAX_ENTRIES and
//put everything else which has 
//a hop count > 0 into a file.
//
//this means that the device has a max of 
//SKYNET_ROUTING_MAX_ENTRIES local devices
//in memory, which is a HUGE amount...
//
//entries will be removed after
//SKYNET_ROUTING_TIMEOUT_MIN minutes from 
//memory...
#define SKYNET_ROUTING_MAX_ENTRIES  100
#define SKYNET_ROUTING_TIMEOUT_MIN  (60 * 1000) * 45


//the routing data structure
struct _sSkyNetRoutingEntry
{
  uint32_t                  dwDeviceID;       //id of the node
  uint32_t                  dwViaNode;        //node over which the Device is reachable
  uint32_t                  dwHopCount;       //number of hops the message needs to pass
  int                       nDevType;         //type of the device
  CSkyNetConnectionHandler  *pConnHandler;    //handler over which the device was received
  long                      lTimeout;         //timeout when the entry will be removed
};



//routing-table definition
//
//this table stores the routing data, when the 
//node has a hop count > 0.
//
/////////////////////////////////////////////////
#define ROUTINGTABLE_FILE (char*)(String(LORALINK_DATA_ROOT) + String("/routing.tbl")).c_str()
#define ROUTINGTABLE_SIZE 4


const int nRoutingTableDef[] =
{
  sizeof(uint32_t),                 //node id
  sizeof(uint32_t),                 //via node
  sizeof(uint32_t),                 //hop count
  sizeof(int)                       //device type
};





//globals
/////////
extern    CWSFLinkedList  *g_pRoutingEntries;
extern    CWSFFileDB      *g_pRoutingTable;


void                  initRouting();
void                  insertIntoRoutingTable(uint32_t dwDeviceID, uint32_t dwViaNode, uint32_t dwHopCount, int nDevType, CSkyNetConnectionHandler *pConnHandler);
_sSkyNetRoutingEntry* FindRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID);
bool                  isInRoutingDB(uint32_t dwDeviceID, uint32_t dwViaNode);
void                  RemoveRoutingEntriesByTaskID(int nTaskID);
void                  RemoveRoutingEntriesByNodeID(uint32_t dwDeviceID);
void                  RemoveRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID);
bool                  SearchBestMatchingRoute(uint32_t dwReceiverNodeID, _sSkyNetRoutingEntry* pBestMatch);
void                  RemoveTimedOutEntries();
void                  UpdateNodeTimeout(uint32_t dwNodeID);





#endif
