//includes
//////////
#include "CSkyNetRouting.h"
#include "LoRaLinkConfig.h"



//globals
/////////
CWSFLinkedList  *g_pRoutingEntries  = NULL;
CWSFFileDB      *g_pRoutingTable    = NULL;



//function predecl
//////////////////
void      insertIntoRoutingTableDB(uint32_t dwDeviceID, uint32_t dwViaNode, uint32_t dwHopCount, int nDevType);
bool      findInRoutingTableDB(CWSFFileDBRecordset * pRS, uint32_t dwDeviceID, uint32_t dwViaNode);
void      removeFromRoutingDB(uint32_t dwDeviceID, uint32_t dwViaNode);
        


void      initRouting()
{
  g_pRoutingEntries = new CWSFLinkedList();

  //open database
  g_pRoutingTable   = new CWSFFileDB(&LORALINK_DATA_FS, ROUTINGTABLE_FILE, (int*)&nRoutingTableDef, ROUTINGTABLE_SIZE, true, 10000);

  if(g_pRoutingTable->open() == true)
  {
    Serial.println(F("RoutingTable: DB Open!"));
  };
};



void      removeFromRoutingDB(uint32_t dwDeviceID, uint32_t dwViaNode)
{
  //variables
  ///////////
  CWSFFileDBRecordset   *pRS = new CWSFFileDBRecordset(g_pRoutingTable);
  uint32_t              dwDeviceID2;
  uint32_t              dwViaNode2;

  while(pRS->haveValidEntry() == true)
  {
    pRS->getData(0, (void*)&dwDeviceID2, sizeof(dwDeviceID2));
    pRS->getData(1, (void*)&dwViaNode2, sizeof(dwViaNode2));
  
    if((dwDeviceID2 == dwDeviceID) && (dwViaNode == 0))
    {
      pRS->remove();

      #if SKYNET_ROUTING_DEBUG == 1
        Serial.print(F("[ROUTING] remove routing to (by NodeID, TransportNodeID) from DB: "));
        Serial.print(dwDeviceID);
        Serial.print(F(" via "));
        Serial.println(dwViaNode);
      #endif
    }
    else
    {
      if(dwViaNode2 == dwViaNode)
      {
        #if SKYNET_ROUTING_DEBUG == 1
          Serial.print(F("[ROUTING] remove routing to (by NodeID, TransportNodeID) from DB: "));
          Serial.print(dwDeviceID);
          Serial.print(F(" via "));
          Serial.println(dwViaNode);
        #endif
        
        pRS->remove();
      };
    };
    
    pRS->moveNext();
  };

  delete pRS;
};



bool      findInRoutingTableDB(CWSFFileDBRecordset *pRS, uint32_t dwDeviceID, uint32_t dwViaNode)
{
  //variables
  ///////////
  uint32_t dwDeviceID2;
  uint32_t dwViaNode2;

  while(pRS->haveValidEntry() == true)
  {
    pRS->getData(0, (void*)&dwDeviceID2, sizeof(dwDeviceID2));
    pRS->getData(1, (void*)&dwViaNode2, sizeof(dwViaNode2));

    if((dwDeviceID2 == dwDeviceID) && (dwViaNode2 == dwViaNode))
    {
      return true;
    };
    
    pRS->moveNext();
  };

  return false;
};


bool      isInRoutingDB(uint32_t dwDeviceID, uint32_t dwViaNode)
{
  //variables
  ///////////
  CWSFFileDBRecordset   *pRS = new CWSFFileDBRecordset(g_pRoutingTable);
  bool                  bRes = findInRoutingTableDB(pRS, dwDeviceID, dwViaNode);

  delete pRS;

  return bRes;
};


void      insertIntoRoutingTable(uint32_t dwDeviceID, uint32_t dwViaNode, uint32_t dwHopCount, int nDevType, CSkyNetConnectionHandler *pConnHandler)
{
  //variables
  ///////////
  _sSkyNetRoutingEntry  *routing; 
  CWSFFileDBRecordset   *pRS;
  void                  *pInsert[ROUTINGTABLE_SIZE + 1];

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.println(F("[ROUTING] --> insertIntoRoutingTable()"));
  #endif

  RemoveTimedOutEntries();

  //don't add me as node to the table
  //(sometimes i received something weird from the modem,
  //which I think is a buffer problem)
  if((DeviceConfig.dwDeviceID != dwDeviceID) && (DeviceConfig.dwDeviceID != dwViaNode))
  {
    //check if node is direct connected, if true,
    //remove all entries
    if(dwViaNode == 0)
    {
      //remove all previous routes, we now have a 
      //direct route...
      RemoveRoutingEntriesByNodeID(dwDeviceID);
      
      //remove all entries from the database,
      //the node is (now) directly connected
      removeFromRoutingDB(dwDeviceID, 0);
    }
    else
    {
      //check if we have a direct route from the same connection...
      if(FindRoutingEntry(dwDeviceID, 0) != NULL)
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


    if(dwHopCount == 0)
    {
      //check if the same route exist in memory...
      if(FindRoutingEntry(dwDeviceID, dwViaNode) == NULL)
      {
        if(g_pRoutingEntries->getItemCount() < SKYNET_ROUTING_MAX_ENTRIES)
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
          routing->lTimeout     = millis() + SKYNET_ROUTING_TIMEOUT_MIN;
        
          g_pRoutingEntries->addItem(routing);
        }
        else
        {
          #ifdef SKYNET_ROUTING_ERROR == 1
            Serial.print(F("[ROUTING] Route to Node: "));
            Serial.print(dwDeviceID);
            Serial.print(F(" over: "));
            Serial.print(dwViaNode);
            Serial.print(F(" Hops: "));
            Serial.print(dwHopCount);
            Serial.println(F(" reached limit!"));
          #endif
        };
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
      pRS = new CWSFFileDBRecordset(g_pRoutingTable);

      if(findInRoutingTableDB(pRS, dwDeviceID, dwViaNode) == false)
      {
        #if SKYNET_ROUTING_INFO == 1
          Serial.print(F("[ROUTING] Add node to RT (DB): Node: "));
          Serial.print(dwDeviceID);
          Serial.print(F(" over: "));
          Serial.print(dwViaNode);
          Serial.print(F(" Hops: "));
          Serial.println(dwHopCount);
        #endif

        pInsert[0] = (void*)&dwDeviceID;
        pInsert[1] = (void*)&dwViaNode;
        pInsert[2] = (void*)&dwHopCount;
        pInsert[3] = (void*)&nDevType;
      };

      delete pRS;
    };
  }
  else
  {
    #if SKYNET_CONN_ERROR == 1
      Serial.println(F("[ROUTING] Can't add own node"));
    #endif
  };

  #if SKYNET_ROUTING_DEBUG == 1
    Serial.println(F("[ROUTING] <-- insertIntoRoutingTable()"));
  #endif
};


void RemoveTimedOutEntries()
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing; 
  bool                  bRemoved;

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    do
    {
      bRemoved = false;
      item     = g_pRoutingEntries->getList();
      
      while(item != NULL)
      {
        routing = (_sSkyNetRoutingEntry*)item->pItem;
  
        if(routing != NULL)
        {
          if(routing->lTimeout < millis())
          {
            #if SKYNET_CONN_INFO == 1
              Serial.print(F("[ROUTING] remove entry (timeout): "));
              Serial.println(routing->dwDeviceID);
            #endif
  
            RemoveRoutingEntriesByNodeID(routing->dwDeviceID);
  
            bRemoved = true;
  
            break;
          };
        };

        item = item->pNext;
      };
    }
    while((g_pRoutingEntries->getItemCount() > 0) && (bRemoved == true));
  };
};


_sSkyNetRoutingEntry* FindRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID)
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
    Serial.println(F(")"));
  #endif
  
  if(g_pRoutingEntries->getItemCount() > 0)
  {
    while(item != NULL)
    {
      routing = (_sSkyNetRoutingEntry*)item->pItem;

      if(routing != NULL)
      {
        if((routing->dwDeviceID == dwNodeID) && (routing->dwViaNode == dwTransportNodeID))
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


void UpdateNodeTimeout(uint32_t dwNodeID)
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
        if(routing->dwDeviceID == dwNodeID)
        {
          routing->lTimeout = millis() + SKYNET_ROUTING_TIMEOUT_MIN;
        };
      };
      
      item = item->pNext;
    };
  };
};


void RemoveRoutingEntriesByNodeID(uint32_t dwDeviceID)
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing; 
  bool                  bRemoved;

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    do
    {
      bRemoved = false;
      item     = g_pRoutingEntries->getList();
      
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

            bRemoved = true;
  
            break;
          };
        };

        item = item->pNext;
      };
    }
    while((g_pRoutingEntries->getItemCount() > 0) && (bRemoved == true));
  };
};



void RemoveRoutingEntry(uint32_t dwNodeID, uint32_t dwTransportNodeID)
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing; 
  bool                  bRemoved;

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    do
    {
      bRemoved = false;
      item     = g_pRoutingEntries->getList();
      
      while(item != NULL)
      {
        routing = (_sSkyNetRoutingEntry*)item->pItem;
  
        if(routing != NULL)
        {
          if((routing->dwDeviceID == dwNodeID) && (routing->dwViaNode == dwTransportNodeID))
          {
            #if SKYNET_ROUTING_DEBUG == 1
              Serial.print(F("[ROUTING] remove routing to (by NodeID, TransportNodeID): "));
              Serial.print(routing->dwDeviceID);
              Serial.print(F(" via "));
              Serial.println(routing->dwViaNode);
            #endif
            
            g_pRoutingEntries->removeItem(routing);
  
            delete routing;
            
            bRemoved = true;
  
            break;
          };
        };

        item = item->pNext;
      };
    }
    while((g_pRoutingEntries->getItemCount() > 0) && (bRemoved == true));
  };

  if(dwTransportNodeID > 0)
  {
    removeFromRoutingDB(dwNodeID, dwTransportNodeID);
  };
};


void RemoveRoutingEntriesByTaskID(int nTaskID)
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing; 
  bool                  bRemoved;

  if(g_pRoutingEntries->getItemCount() > 0)
  {
    do
    {
      bRemoved = false;
      item     = g_pRoutingEntries->getList();
      
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

            bRemoved = true;
  
            break;
          };
        };

        item = item->pNext;
      };
    }
    while((g_pRoutingEntries->getItemCount() > 0) && (bRemoved == true));
  };
};


bool SearchBestMatchingRoute(uint32_t dwReceiverNodeID, _sSkyNetRoutingEntry* pBestMatch)
{
  //variables
  ///////////
  _sListItem            *item;
  _sSkyNetRoutingEntry  *routing;
  _sSkyNetRoutingEntry  *best = NULL;
  int                   nRoutingPriority[] = {DEVICE_TYPE_REPEATER, DEVICE_TYPE_PUBLIC_REPEATER, DEVICE_TYPE_PERSONAL_REPEATER, DEVICE_TYPE_PERSONAL, DEVICE_TYPE_UNKNOWN};
  CWSFFileDBRecordset   *pRS;
  uint32_t              dwDeviceID2;
  uint32_t              dwViaNode2;
  uint32_t              dwViaNode         = 0;
  uint32_t              dwHopCount        = 0xFFFFFFFF;
  uint32_t              dwHopCount2;
  bool                  bFound            = false;

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


  //if the node was not found, search inside the 
  //database for a via node...
  //this is L1 upwards, so the via node must be in 
  //memory (L0)...
  if(best == NULL)
  {
    pRS = new CWSFFileDBRecordset(g_pRoutingTable);
  
    while(pRS->haveValidEntry() == true)
    {
      pRS->getData(0, (void*)&dwDeviceID2, sizeof(dwDeviceID2));
      pRS->getData(1, (void*)&dwViaNode2, sizeof(dwViaNode2));
      pRS->getData(2, (void*)&dwHopCount2, sizeof(dwHopCount2));
  
      if(dwDeviceID2 == dwReceiverNodeID)
      {
        if(dwHopCount > dwHopCount2)
        {
          //check if the L0 route exist
          routing = FindRoutingEntry(dwViaNode2, 0);

          if(routing != NULL)
          {
            dwHopCount = dwHopCount2;
            dwViaNode  = dwViaNode2;
            
            //set data
            pBestMatch->dwDeviceID    = dwDeviceID2;
            pBestMatch->dwViaNode     = dwViaNode2; 
            pBestMatch->dwHopCount    = dwHopCount2;
            pBestMatch->pConnHandler  = routing->pConnHandler;

            #if SKYNET_ROUTING_INFO == 1
              Serial.print(F("[ROUTING] found entry in DB for: "));
              Serial.print(dwReceiverNodeID);
              Serial.print(F(", hop count: "));
              Serial.print(dwHopCount);
              Serial.print(F(" via: "));
              Serial.println(dwViaNode);
            #endif

            bFound = true;
          };
        };
      };
      
      pRS->moveNext();
    };
  }
  else
  {
    memcpy(pBestMatch, best, sizeof(_sSkyNetRoutingEntry));
    bFound = true;
  };
  
  #if SKYNET_ROUTING_DEBUG == 1
    Serial.print(F("[ROUTING] <-- SearchBestMatchingRoute() for "));
    Serial.println(dwReceiverNodeID);
  #endif

  return bFound;
};
