#ifndef __SKYNET_ROUTING__
#define __SKYNET_ROUTING__


//includes
//////////
#include <Arduino.h>
#include "CWSFLinkedList.h"
#include "CSkyNetConnHandler.h"


//defines
/////////
#define SKYNET_ROUTING_INFO  1
#define SKYNET_ROUTING_DEBUG 1


//the routing data structure
struct _sSkyNetRoutingEntry
{
  uint32_t                  dwDeviceID;
  uint32_t                  dwViaNode;
  uint32_t                  dwHopCount;
  int                       nDevType;
  CSkyNetConnectionHandler  *pConnHandler;
};


//globals
/////////
extern    CWSFLinkedList *g_pRoutingEntries;



void                  initRouting();
void                  insertIntoRoutingTable(uint32_t dwDeviceID, uint32_t dwViaNode, uint32_t dwHopCount, int nDevType, CSkyNetConnectionHandler *pConnHandler);
_sSkyNetRoutingEntry* FindRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID, int nConnectionType);
void                  RemoveRoutingEntriesByTaskID(int nTaskID);
void                  RemoveRoutingEntriesByNodeID(uint32_t dwDeviceID);
void                  RemoveRoutingEntriesByMsgID(uint32_t dwMsgID);
void                  RemoveRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID);
_sSkyNetRoutingEntry* SearchBestMatchingRoute(uint32_t dwReceiverNodeID);






#endif
