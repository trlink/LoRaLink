//includes
//////////
#include "CSkyNetRouting.h"
#include "LoRaLinkConfig.h"

//globals
/////////
CWSFLinkedList *g_pRoutingEntries = NULL;



void      initRouting()
{
  g_pRoutingEntries = new CWSFLinkedList();
};



void      insertIntoRoutingTable(uint32_t dwDeviceID, uint32_t dwViaNode, uint32_t dwHopCount, int nDevType, CSkyNetConnectionHandler *pConnHandler)
{
  //variables
  ///////////
  _sSkyNetRoutingEntry *routing; 

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.println(F("[ROUTING] --> insertIntoRoutingTable()"));
  #endif

  if((DeviceConfig.dwDeviceID != dwDeviceID) && (DeviceConfig.dwDeviceID != dwViaNode))
  {
    //check if node is direct connected, if true,
    //remove all entries
    if(dwViaNode == 0)
    {
      //remove all previous routes, we now have a 
      //direct route...
      RemoveRoutingEntriesByNodeID(dwDeviceID);
    }
    else
    {
      //check if we have a direct route...
      if(FindRoutingEntry(dwDeviceID, 0, pConnHandler->getConnectionType()) != NULL)
      {
        #if SKYNET_ROUTING_INFO == 1
          Serial.print(F("[ROUTING] ignore route (have direct): Node: "));
          Serial.print(dwDeviceID);
          Serial.print(F(" over: "));
          Serial.print(dwViaNode);
          Serial.print(F(" Hops: "));
          Serial.println(dwHopCount);
        #endif

        #if SKYNET_ROUTING_DEBUG == 1
          Serial.println(F("[ROUTING] <-- insertIntoRoutingTable()"));
        #endif

        return;
      };
    };

    //check if exist...
    if(FindRoutingEntry(dwDeviceID, dwViaNode, pConnHandler->getConnectionType()) == NULL)
    {
      routing = new _sSkyNetRoutingEntry;
  
      if(routing != NULL)
      
      #if SKYNET_ROUTING_INFO == 1
        Serial.print(F("[ROUTING] Add node to RT: Node: "));
        Serial.print(dwDeviceID);
        Serial.print(F(" over: "));
        Serial.print(dwViaNode);
        Serial.print(F(" Hops: "));
        Serial.println(dwHopCount);
      #endif
  
      
      //insert
      routing->dwDeviceID   = dwDeviceID;
      routing->dwViaNode    = dwViaNode;
      routing->dwHopCount   = dwHopCount;
      routing->nDevType     = nDevType;
      routing->pConnHandler = pConnHandler;
    
      g_pRoutingEntries->addItem(routing);
    }
    else
    {
      #if SKYNET_ROUTING_INFO == 1
        Serial.print(F("[ROUTING] Route exist to: Node: "));
        Serial.print(dwDeviceID);
        Serial.print(F(" over: "));
        Serial.print(dwViaNode);
        Serial.print(F(" Hops: "));
        Serial.println(dwHopCount);
      #endif
    };
  }
  else
  {
    #if SKYNET_CONN_INFO == 1
      Serial.println(F("[ROUTING] Can't add own node"));
    #endif
  };

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.println(F("[ROUTING] <-- insertIntoRoutingTable()"));
  #endif
};


_sSkyNetRoutingEntry* FindRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID, int nConnectionType)
{
  //variables
  ///////////
  _sListItem            *item = g_pRoutingEntries->getList();
  _sSkyNetRoutingEntry  *routing; 

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.print(F("[ROUTING] --> FindRoutingEntry("));
    Serial.print(dwNodeID);
    Serial.print(F(", "));
    Serial.print(dwTransportNodeID);
    Serial.print(F(", "));
    Serial.print(nConnectionType);
    Serial.println(F(")"));
  #endif
  
  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if((routing->dwDeviceID == dwNodeID) && (routing->dwViaNode == dwTransportNodeID) && (routing->pConnHandler->getConnectionType() == nConnectionType))
        {
          #if SKYNET_CONN_INFO == 1
            Serial.print(F("[ROUTING] found entry for: "));
            Serial.println(dwNodeID);
          #endif

          #if SKYNET_ROUTING_DEBUG == 1
            Serial.println(F("[ROUTING] <-- FindRoutingEntry()"));
          #endif
          
          return routing;
        };
      };
      
      item = item->pNext;
    };
  };

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.println(F("[ROUTING] <-- FindRoutingEntry()"));
  #endif

  return NULL;
};


void RemoveRoutingEntriesByNodeID(uint32_t dwDeviceID)
{
  //variables
  ///////////
  _sListItem            *item     = g_pRoutingEntries->getList();
  _sSkyNetRoutingEntry  *routing; 

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if(routing->dwDeviceID == dwDeviceID)
        {
          #if SKYNET_ROUTING_DEBUG == 1
            Serial.print(F("[ROUTING] remove routing to (by NodeID): "));
            Serial.println(routing->dwDeviceID);
          #endif
          
          g_pRoutingEntries->removeItem(routing);

          delete routing;
        };
      };
      
      item = item->pNext;
    };
  };
};



void RemoveRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID)
{
  //variables
  ///////////
  _sListItem            *item     = g_pRoutingEntries->getList();
  _sSkyNetRoutingEntry  *routing; 

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if((routing->dwDeviceID == dwNodeID) && (routing->dwViaNode == dwTransportNodeID))
        {
          #if SKYNET_ROUTING_DEBUG == 1
            Serial.print(F("[ROUTING] remove routing to (by NodeID, TransportNodeID): "));
            Serial.println(routing->dwDeviceID);
          #endif
          
          g_pRoutingEntries->removeItem(routing);

          delete routing;
        };
      };
      
      item = item->pNext;
    };
  };
};


void RemoveRoutingEntriesByTaskID(int nTaskID)
{
  //variables
  ///////////
  _sListItem            *item     = g_pRoutingEntries->getList();
  _sSkyNetRoutingEntry  *routing; 

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if(routing->pConnHandler->getTaskID() == nTaskID)
        {
          #if SKYNET_ROUTING_DEBUG == 1
            Serial.print(F("[ROUTING] remove routing to (by task): "));
            Serial.println(routing->dwDeviceID);
          #endif
          
          g_pRoutingEntries->removeItem(routing);

          delete routing;
        };
      };
      
      item = item->pNext;
    };
  };
};


_sSkyNetRoutingEntry* SearchBestMatchingRoute(uint32_t dwReceiverNodeID)
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing;
  _sSkyNetRoutingEntry  *best = NULL;
  int                   nRoutingPriority[] = {DEVICE_TYPE_REPEATER, DEVICE_TYPE_PUBLIC_REPEATER, DEVICE_TYPE_PERSONAL_REPEATER, DEVICE_TYPE_PERSONAL, DEVICE_TYPE_UNKNOWN};

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.print(F("[ROUTING] --> SearchBestMatchingRoute() for "));
    Serial.println(dwReceiverNodeID);
  #endif

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    for(int n = 0; n < 5; ++n)
    {
      #if SKYNET_ROUTING_DEBUG == 1
        Serial.print(F("[ROUTING] search for forward dev type: "));
        Serial.println(nRoutingPriority[n]);
      #endif

      item = g_pRoutingEntries->getList();
    
      while(item != NULL)
      {
        routing = (_sSkyNetRoutingEntry*)item->pItem;
  
        if(routing != NULL)
        {
          if(routing->dwDeviceID == dwReceiverNodeID)
          {
            #if SKYNET_ROUTING_INFO == 1
              Serial.print(F("[ROUTING] found entry for: "));
              Serial.print(dwReceiverNodeID);
              Serial.print(F(", hop count: "));
              Serial.print(routing->dwHopCount);
              Serial.print(F(" via: "));
              Serial.println(routing->dwViaNode);
            #endif

            if(best != NULL)
            {
              if(routing->dwHopCount < best->dwHopCount)
              {
                best = routing;
              };
            }
            else
            {
              best = routing;
            };
          };
        };
      
        item = item->pNext;
      };
    };
  };

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.print(F("[ROUTING] <-- SearchBestMatchingRoute() for "));
    Serial.println(dwReceiverNodeID);
  #endif

  return best;
};
