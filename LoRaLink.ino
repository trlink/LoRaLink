//includes
//////////
#include "HardwareConfig.h"
#include "LoRaLinkConfig.h"
#include "helper.h"
#include "CWSFClockHelper.h"
#include "CLoRaLinkDatabase.h"
#include "SystemState.h"
#include "CTaskHandler.h"
#include "CFirmwareUpdater.h"
#include "CSkyNetConnHandler.h"
#include "CSkyNetConnection.h"
#include "CLoRaModem.h"
#include "CBlinkLed.h"
#include "CLoRaLinkProtocol.h"
#include "CDBTaskScheduler.h"
#include "CDBTaskQuery.h"
#include "CWebEvent.h"
#include "CWSFLinkedList.h"
#include "CSkyNetRouting.h"
#include <Bounce2.h>


//https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/



//defines
/////////
#define WEBAPIDEBUG
//#define WEBAPIDEBUGX
//#define TCPAPIDEBUG
//#define TCPAPIERROR
//#define SERIAL1DEBUG
//#define GPSSTATSDEBUG



//struct to store received data and handle it in another thread
struct _sModemData
{
  byte                     *pData;
  int                       nDataLength;
  int                       nRSSI;
  CSkyNetConnectionHandler *pHandler; 
  float                     fPacketSNR;
};




//function predecl
//////////////////
void OnReceivedModemData(byte *pData, int nDataLength, int nRSSI, void *pHandler, float fPacketSNR);
void OnModemStateChanged(int nState);
void ModemTask(void *pParam);
void BlinkTask(void *pParam);
void ModemDataTask(void *pParam);
void OnSchedule(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData);
void OnTaskScheduleRemove(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData, bool bSuccess);
void OnLoRaLinkProtocolData(void *pProtocolMsg, byte *pData, int nDataLen);



//globals
/////////
CClockHlpr             *ClockPtr                = NULL;
_sSystemState           LLSystemState;
CTaskHandler           *g_pTaskHandler          = new CTaskHandler();
CLoRaModem             *g_pModemTask;
TaskHandle_t            g_Core0TaskHandle; 
TaskHandle_t            g_Core1TaskHandle; 
TaskHandle_t            g_Core0TaskHandle1;
TaskHandle_t            g_Core0TaskHandle2; 
TaskHandle_t            g_Core1TaskHandle2; 
CBlinkLed               g_ledblink(2);
CDBTaskScheduler       *g_pDbTaskScheduler      = new CDBTaskScheduler(OnSchedule, OnTaskScheduleRemove);
CSkyNetConnection      *g_pSkyNetConnection     = new CSkyNetConnection(g_pDbTaskScheduler, OnLoRaLinkProtocolData);
CLoRaLinkProtocol      *g_pCLoRaProtocol        = new CLoRaLinkProtocol(g_pSkyNetConnection, g_pDbTaskScheduler);
char                    g_szFileSystem[10];
CWSFLinkedList         *g_pModemMessages        = new CWSFLinkedList();
Bounce2::Button        *g_pUserButton           = new Bounce2::Button();


#ifdef LORALINK_HARDWARE_TBEAM
  AXP20X_Class         *g_pAxp                  = new AXP20X_Class(); 
#endif


#if LORALINK_HARDWARE_GPS == 1
  //variables
  ///////////
  TinyGPS              *g_pGPS                    = NULL;
  bool                  g_bLocationTrackingActive = false;
  int                   g_nLocationTrackingType   = GPS_POSITION_TYPE_NORMAL;
  TaskHandle_t          g_Core0TaskHandle3;
  bool                  g_bHaveData               = false;
  bool                  g_bRecordTrack            = false;
  uint32_t              g_dwRecordTrackID         = 0;
  long                  g_lTrackRecTimer          = 0;
  

  //function predecl
  //////////////////
  void GpsDataTask(void *pParam);
#endif


#if LORALINK_HARDWARE_OLED == 1
  Adafruit_SSD1306      g_display;

  //function predecl
  //////////////////
  void DisplayTask(void *pParam);
#endif


#if LORALINK_HARDWARE_SDCARD == 1
  //variables
  ///////////
  SPIClass g_spiSD;
#endif



#if LORALINK_HARDWARE_WIFI == 1
  //includes
  //////////
  #include <WiFi.h>
  #include "WebServer.h"
  #include <NTPClient.h>
  #include "CTCPServer.h"
  

  //variables
  ///////////
  DNSServer               g_dnsServer;
  CTCPServer             *g_pTCPServer          = NULL;
  int                     g_nWiFiConnects       = 0;
  CWSFLinkedList         *g_pMessages           = new CWSFLinkedList();
  long                    g_lReconnectToServerTimeout;
  bool                    g_bReconnectToServer;
  CWebEvent              *g_pWebEvent           = new CWebEvent();
  int                     g_nShoutoutCount      = 0;
  int                     g_nWebserverCheck     = 0;
  long                    g_lWiFiShutdownTimer  = 0;
  bool                    g_bWiFiConnected      = false;
  
  //function predecl
  //////////////////
  void ConnectToLoraLinkServer();
  void HandleLoRaLinkReconnect();
  void EnableWiFi();


  //this function will enable and setup WiFi
  void EnableWiFi()
  {
    //variables
    ///////////
    IPAddress IP;
    long      lTimeout;

    g_bWiFiConnected = false;
    
    WiFi.mode(WIFI_OFF);

    LoRaWiFiApCfg.bWiFiEnabled = true;

    Serial.println(F("Enable WiFi:"));

    if(strlen(LoRaWiFiCfg.szWLANSSID) > 0)
    {
      WiFi.mode(WIFI_AP_STA);
    }
    else 
    {
      WiFi.mode(WIFI_AP);  
    };

    WiFi.hostname("loralink");


    // Remove the password parameter, if you want the AP (Access Point) to be open
    if(strlen(LoRaWiFiApCfg.szWLANPWD) > 0) 
    {
      WiFi.softAP(LoRaWiFiApCfg.szWLANSSID, LoRaWiFiApCfg.szWLANPWD, LoRaWiFiApCfg.nChannel, (LoRaWiFiApCfg.bHideNetwork == true ? 1 : 0));
    }
    else 
    {
      WiFi.softAP(LoRaWiFiApCfg.szWLANSSID, NULL, LoRaWiFiApCfg.nChannel, (LoRaWiFiApCfg.bHideNetwork == true ? 1 : 0));
    };
    
    if(IP.fromString(LoRaWiFiApCfg.szDevIP)) 
    {
      IPAddress NMask(255, 255, 255, 0);
      
      WiFi.softAPConfig(IP, IP, NMask);
    }; 


    //check for WiFi Connect
    if(strlen(LoRaWiFiCfg.szWLANSSID) > 0)
    {
      Serial.print(F("Connect to WiFi: "));
      Serial.println(LoRaWiFiCfg.szWLANSSID);

      WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
      WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
      WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
      
      if(strlen(LoRaWiFiCfg.szWLANPWD) > 0)
      {
        WiFi.begin(LoRaWiFiCfg.szWLANSSID, LoRaWiFiCfg.szWLANPWD);
      }
      else
      {
        WiFi.begin(LoRaWiFiCfg.szWLANSSID);
      };

      
      lTimeout = millis() +  30000;

      //wait till wifi is connected
      while((g_bWiFiConnected == false) && (millis() < lTimeout));
      {
        delay(250);
        
        ResetWatchDog();
      };
    };
  };
  
  /**
   * this task handles the webserver connections and process dns requests
   */
  void WebServerTask(void *pParam)
  {
    #ifdef WEBAPIDEBUG
      Serial.println(F("Webserver Task started"));
    #endif
    
    while(true)
    {
      LLSystemState.lMemFreeWebServerTask     = uxTaskGetStackHighWaterMark(NULL);

      g_dnsServer.processNextRequest();
  
      HandleWebserver();

      delay(1);

      ResetWatchDog();  
    };
  };


  //handle connect to lora link server
  //the re-connect will only be done every 5 sec
  void HandleLoRaLinkReconnect()
  {
    if(g_bReconnectToServer == true)
    {
      if(millis() > g_lReconnectToServerTimeout)
      {
        g_bReconnectToServer        = false;
        g_lReconnectToServerTimeout = 0;
      
        ConnectToLoraLinkServer();
      };
    };
  };
  

  void ConnectToLoraLinkServer()
  {
    if(IpLinkConfig.bClientEnabled == true)
    {
      #ifdef TCPAPIDEBUG
        Serial.print(F("LoRa-Link: connect to: "));
        Serial.println(IpLinkConfig.szClient);
      #endif

      CTCPClient                *pNewClient = new CTCPClient(IpLinkConfig.szClient, IpLinkConfig.wClientPort);
      CSkyNetConnectionHandler  *pHandler   = new CSkyNetConnectionHandler(pNewClient, SKYNET_CONN_TYPE_IP_SERVER, g_pSkyNetConnection);

      pNewClient->setObjectReference(pHandler);
      pNewClient->setCallBacks(onIpConnData, onIpConnDisc);
      
      g_pTaskHandler->addTask(pNewClient);
      g_pTaskHandler->addTask(pHandler);

      g_pSkyNetConnection->addHandler(pHandler);
    };
  };


  /**
   * this callback handles the data received over an ip connection. 
   * it stores the data into the data queue
   */
  void onIpConnData(void *pClient, byte *pData, int nDataLen)
  {
    //variables
    ///////////
    _sModemData              *pModemData = new _sModemData;

    #ifdef TCPAPIDEBUG
      Serial.print(F("Got: "));
      Serial.print(nDataLen);
      Serial.print(F(" bytes from: "));
      Serial.println(((CTCPClient*)pClient)->getClient().remoteIP());
    #endif

    ResetWatchDog();

    //only store the data, don't handle it here,
    //otherwise the modem looses packets...
    pModemData->nDataLength   = nDataLen;
    pModemData->pData         = new byte[nDataLen + 1];
    memcpy(pModemData->pData, pData, nDataLen);
    pModemData->nRSSI         = 0;
    pModemData->pHandler      = ((CSkyNetConnectionHandler*)((CTCPClient*)pClient)->getObjectReference());
    pModemData->fPacketSNR    = 0.0;
  
    g_pModemMessages->addItem(pModemData);
  };



  void onIpConnDisc(void *pClient)
  {
    //variables
    ///////////
    CTCPClient                *pTCPClient   = (CTCPClient*)pClient;
    CSkyNetConnectionHandler  *pHandler     = (CSkyNetConnectionHandler*)pTCPClient->getObjectReference();
    int nHandlerTaskID                      = 0;
    int nClientTaskID                       = pTCPClient->getTaskID();
    int nConnType;

    
    if(pHandler != NULL)
    {
      nConnType       = pHandler->getConnectionType();
      nHandlerTaskID  = pHandler->getTaskID();
    };

    #ifdef TCPAPIDEBUG
      Serial.print(F("Client disconnected - remove client Task: "));
      Serial.print(nClientTaskID);
      Serial.print(F(" Handler Task: "));
      Serial.print(nHandlerTaskID);
      Serial.print(F(" conn type: "));
      Serial.println(nConnType);
    #endif

    if(pHandler != NULL)
    {
      g_pTaskHandler->removeTask(nHandlerTaskID);
      
      //remove from routing entries
      RemoveRoutingEntriesByTaskID(nHandlerTaskID);
      
      //remove from skynet
      g_pSkyNetConnection->removeHandler(pHandler);

      delete pHandler;
    };

    g_pTaskHandler->removeTask(nClientTaskID);

    delete pTCPClient;

    if(nConnType == SKYNET_CONN_TYPE_IP_SERVER)
    {
      #ifdef TCPAPIDEBUG
        Serial.println(F("Link disconnected - reconnect"));
      #endif
      
      g_lReconnectToServerTimeout = millis() + 5000;
      g_bReconnectToServer        = true;
    };
  };



  void onIpClientConnect(void *pClient)
  {
    #ifdef TCPAPIDEBUG
      Serial.print(F("Got IP conn from: "));
      Serial.println(((CTCPClient*)pClient)->getClient().remoteIP());
    #endif

    CSkyNetConnectionHandler *pHandler = new CSkyNetConnectionHandler((CTCPClient*)pClient, SKYNET_CONN_TYPE_IP_CLIENT, g_pSkyNetConnection);

    ((CTCPClient*)pClient)->setObjectReference(pHandler);
    ((CTCPClient*)pClient)->setCallBacks(onIpConnData, onIpConnDisc);

    g_pTaskHandler->addTask((CTCPClient*)pClient);
    g_pTaskHandler->addTask(pHandler);

    g_pSkyNetConnection->addHandler(pHandler);
  };



  void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    #ifdef TCPAPIDEBUG
      Serial.println(F("WiFi connected"));
    #endif

    g_nWiFiConnects = 0;
  };
  

  void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    Serial.print(F("WiFi connected: IP address: "));
    Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
  
    sprintf_P(LoRaWiFiCfg.szDevIP, PSTR("%i.%i.%i.%i"), IPAddress(info.got_ip.ip_info.ip.addr)[0], IPAddress(info.got_ip.ip_info.ip.addr)[1], IPAddress(info.got_ip.ip_info.ip.addr)[2], IPAddress(info.got_ip.ip_info.ip.addr)[3]);

    ResetWatchDog();

    //check for NTP update on connect
    if(DeviceConfig.bUpdateTimeNTP == true)
    {
      WiFiUDP   ntpUDP;
      NTPClient timeClient(ntpUDP);
      
      timeClient.begin();
      timeClient.update();

      if(timeClient.isTimeSet() == true)
      {
        unsigned long epochTime = timeClient.getEpochTime();
        struct tm *ptm = gmtime((time_t*)&epochTime);
  
        ClockPtr->SetDateTime(1900 + ptm->tm_year, ptm->tm_mon + 1, ptm->tm_mday, timeClient.getHours(), timeClient.getMinutes(), UPDATE_SOURCE_NTP);

        Serial.print(F("Updated time: "));
        Serial.println(ClockPtr->GetTimeString());
        
        ResetWatchDog();

        //update schedules after reboot
        g_pDbTaskScheduler->rescheduleAfterTimechange();
        g_pDbTaskScheduler->haltScheduler(false);
      }
      else
      {
        #ifdef TCPAPIDEBUG
          Serial.println(F("Update time failed"));
        #endif
      };
    };


    if(strlen(DynDNSConfig.szProvider) > 0)
    {
      EasyDDNS.update(120000);
    };

    ResetWatchDog();

    if(IpLinkConfig.bClientEnabled == true)
    {
      g_lReconnectToServerTimeout = millis() + 5000;
      g_bReconnectToServer        = true;
    };

    ResetWatchDog();

    g_bWiFiConnected = true;
  };  


  void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    #ifdef TCPAPIERROR
      Serial.println(F("Disconnected from AP!"));
    #endif

    g_nWiFiConnects += 1;
    g_bWiFiConnected = false;

    if(g_nWiFiConnects > 15)
    {
      #ifdef TCPAPIERROR
        Serial.println(F("Disable WiFi reconnect"));
      #endif

      WiFi.disconnect();

      memset(LoRaWiFiCfg.szDevIP, 0, sizeof(LoRaWiFiApCfg.szDevIP));
      strcpy_P(LoRaWiFiCfg.szDevIP, PSTR("Not connected"));
    }
    else
    {
      #ifdef TCPAPIDEBUG
        Serial.println(F("WiFi reconnect"));
      #endif
      
      WiFi.disconnect();
      WiFi.reconnect();  
    };
  };  


  void sendAnswerWrongCreds(HTTPResponse *resp)
  {
    sendStringResponse(resp, 404, (char*)(String(F("text/plain"))).c_str(), (char*)(String(F("ERROR - wrong credentials"))).c_str());
  };


  void handleChatRequests(HTTPResponse *resp, char *pData, int nDataLength, DynamicJsonDocument doc)
  { 
    //variables
    ///////////    
    DynamicJsonDocument docResp(50);
    _sNewMessage        *pNewMsg;
    int                 nRespCode = 0;
    
    ResetWatchDog();

    
    if(CheckApiUserPassword(doc[F("userID")], doc[F("hash")]) == true)
    {
      #if LORALINK_HARDWARE_GPS == 1
      
        if(strcmp_P(doc[F("chatcmd")], PSTR("enablePositionTracking")) == 0)
        {
          //variables
          ///////////
          uint32_t          dwTaskID = g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_SEND_POSITION);
          sDBTaskQueryUser  task;  
          byte              bData[200];
  
          if(dwTaskID == 0)
          {
            task.dwDeviceID     = 0; //all
            task.dwUserToInform = 0; //all
            task.dwContactID    = (int)doc[F("type")];
            task.dwReleaseTask  = 0;
            
            memset(bData, 0, sizeof(bData));
            memcpy(&bData, (void*)&task, sizeof(sDBTaskQueryUser));
  
            g_pDbTaskScheduler->addSchedule(DBTASK_SEND_POSITION, LORALINK_POSITION_INTERVAL_SECONDS, 0, (byte*)&bData, true);
  
            g_bLocationTrackingActive = true;
            g_nLocationTrackingType   = task.dwContactID; //used as type
          }
          else
          {
            task.dwDeviceID     = 0; //to all devices
            task.dwUserToInform = 0; //to all user
            task.dwContactID    = (int)doc[F("type")];
            task.dwReleaseTask  = 0;
  
            memset(bData, 0, sizeof(bData));
            memcpy(&bData, (void*)&task, sizeof(sDBTaskQueryUser));
  
            g_pDbTaskScheduler->updateTaskData(dwTaskID, (byte*)&bData, sizeof(sDBTaskQueryUser));
  
            g_bLocationTrackingActive = true;
            g_nLocationTrackingType   = task.dwContactID; //used as type
          };
  
          docResp["response"] = String(F("OK"));
          nRespCode = 200;
        };
  
        if(strcmp_P(doc[F("chatcmd")], PSTR("disablePositionTracking")) == 0)
        {
          //variables
          ///////////
          uint32_t          dwTaskID = g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_SEND_POSITION);
  
          if(dwTaskID != 0)
          {
            g_pDbTaskScheduler->removeSchedule(dwTaskID, true);
          };
  
          docResp["response"] = String(F("OK"));
          nRespCode = 200;
        };
  
        
        if(strcmp_P(doc[F("chatcmd")], PSTR("allowPositionTracking")) == 0)
        {
          //variables
          ///////////
          char                szDatabaseFile[50];
          CWSFFileDB          *pDatabase;
          CWSFFileDBRecordset *pRecordset;
          int                 nBlocked = 0;
  
          memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
          sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, (uint32_t)doc[F("userID")]);
    
          pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);
          pDatabase->open();
  
          if(pDatabase->isOpen() == true)
          {
            pRecordset = new CWSFFileDBRecordset(pDatabase, (uint32_t)doc[F("contactID")]);
  
            if(pRecordset->haveValidEntry() == true)
            {
              nBlocked = (int)doc[F("allowed")];
  
              pRecordset->setData(6, (void*)&nBlocked, sizeof(nBlocked));
  
              docResp["response"] = String(F("OK"));
              nRespCode = 200;
            }
            else
            {
              docResp["response"] = String(F("ERR"));
              nRespCode = 500;
            };
  
            delete pRecordset;
          }
          else
          {
            docResp["response"] = String(F("ERR"));
            nRespCode = 500;
          };
  
          delete pDatabase;
        };

        if(strcmp_P(doc[F("chatcmd")], PSTR("startTrackRecord")) == 0)
        {
          //variables
          ///////////
          char                szDatabaseFile[50];
          CWSFFileDB          *pDatabase;
          char                *szDesc = new char[256];
          uint32_t            dwUserID = (uint32_t)doc[F("userID")];
          uint32_t            dwTime   = ClockPtr->getUnixTimestamp();
          void                *pInsert[TRACKHEADTABLE_SIZE + 1];
          sDBTaskSendFile     sTrackRec;
          byte                *bData = new byte[201];

          memset(bData, 0, 200);
          memset(szDesc, 0, 255);
          memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
          sprintf((char*)&szDatabaseFile, TRACKHEADTABLE_FILE);

          strcpy(szDesc, (const char*)doc[F("desc")]);
          
          if((g_bRecordTrack == false) && (g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_RECORD_TRACK) == 0))
          {
            Serial.print(F("Start track rec for: "));
            Serial.print(dwUserID);
            Serial.print(F(" desc: "));
            Serial.println(szDesc);
            
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackHeadTableDef, TRACKHEADTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
            pDatabase->open();
    
            if(pDatabase->isOpen() == true)
            {
              pInsert[0] = (void*)&dwUserID;
              pInsert[1] = (void*)&dwTime;
              pInsert[2] = (void*)szDesc;

              pDatabase->insertData(pInsert);
              g_dwRecordTrackID = pDatabase->getLastInsertPos();
              

              sTrackRec.dwFileID      = g_dwRecordTrackID;
              sTrackRec.dwReleaseTask = 0;

              memcpy(bData, (void*)&sTrackRec, sizeof(sDBTaskSendFile));

              g_pDbTaskScheduler->addSchedule(DBTASK_RECORD_TRACK, LORALINK_POSITION_INTERVAL_SECONDS, 0, bData, true);

              g_bRecordTrack = true;
              
              docResp["response"] = String(F("OK"));
              nRespCode = 200;
            }
            else
            {
              docResp["response"] = String(F("ERR"));
              nRespCode = 500;
            };
    
            delete pDatabase;
          }
          else
          {
            docResp["response"] = String(F("ERR"));
            nRespCode = 500;
          };

          delete szDesc;
          delete bData;
        };

        if(strcmp_P(doc[F("chatcmd")], PSTR("getTracks")) == 0)
        {
          //variables
          ///////////
          char                szDatabaseFile[50];
          CWSFFileDB          *pDatabase;
          CWSFFileDBRecordset *pRecordset;
          char                *buffer = new char[301];
          char                *szDesc = new char[256];
          uint32_t            dwTime;
          uint32_t            dwUserID;
          bool                bHadEntry = false;
          DateTime            dtTime;


          memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
          sprintf((char*)&szDatabaseFile, TRACKHEADTABLE_FILE);

          pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackHeadTableDef, TRACKHEADTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
          pDatabase->open();

          memset(buffer, 0, 200);
          strcpy_P(buffer, PSTR("{\"Tracks\": ["));
          resp->print(buffer);
    
          if(pDatabase->isOpen() == true)
          {
            pRecordset = new CWSFFileDBRecordset(pDatabase);

            while(pRecordset->haveValidEntry() == true)
            {
              pRecordset->getData(0, (void*)&dwUserID, sizeof(uint32_t));

              if(dwUserID == (uint32_t)doc[F("userID")])
              {
                memset(szDesc, 0, 255);
                
                pRecordset->getData(1, (void*)&dwTime, sizeof(uint32_t));
                pRecordset->getData(2, (void*)szDesc, 255);

                dtTime = DateTime((time_t)dwTime);
                
                if(bHadEntry == true)
                {
                  memset(buffer, 0, 300);
                  sprintf_P(buffer, PSTR(", "));
                  resp->print(buffer);
                };

                memset(buffer, 0, 300);
                sprintf_P(buffer, PSTR("{\"Time\": \"%4i-%02i-%02i %02i:%02i:%02i\", \"Desc\": \"%s\", \"ID\": %u, \"Active\": %i}"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second(), szDesc, pRecordset->getRecordPos(), (g_dwRecordTrackID == pRecordset->getRecordPos() ? 1 : 0));
                resp->println(buffer);

                bHadEntry = true;
              };
              
              ResetWatchDog();

              pRecordset->moveNext();
            };

            delete pRecordset;
          };
            
          // we are done, send the footer
          memset(buffer, 0, 300);
          strcpy_P(buffer, PSTR("]}"));
          resp->println(buffer);
        
          delete pDatabase;
          delete buffer;
          delete szDesc;

          docResp.clear();

          return;
        };

        if(strcmp_P(doc[F("chatcmd")], PSTR("getTrackData")) == 0)
        {
          //variables
          ///////////
          char                szDatabaseFile[50];
          CWSFFileDB          *pDatabase;
          CWSFFileDBRecordset *pRecordset;
          char                *buffer = new char[201];
          float               fLat, fLon, fAlt;
          uint32_t            dwTime;
          uint32_t            dwTrackID;
          bool                bHadEntry = false;
          DateTime            dtTime;
          

          if(g_bRecordTrack == false)
          {
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, TRACKWAYPOINTTABLE_FILE);

            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackWaypointTableDef, TRACKWAYPOINTTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
            pDatabase->open();

            memset(buffer, 0, 200);
            strcpy_P(buffer, PSTR("{\"Waypoints\": ["));
            resp->print(buffer);
    
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase);

              while(pRecordset->haveValidEntry() == true)
              {
                pRecordset->getData(0, (void*)&dwTrackID, sizeof(uint32_t));

                if(dwTrackID == (uint32_t)doc[F("TrackID")])
                {
                  pRecordset->getData(1, (void*)&fLat, sizeof(float));
                  pRecordset->getData(2, (void*)&fLon, sizeof(float));
                  pRecordset->getData(3, (void*)&fAlt, sizeof(float));
                  pRecordset->getData(4, (void*)&dwTime, sizeof(uint32_t));

                  dtTime = DateTime((time_t)dwTime);
                  
                  if(bHadEntry == true)
                  {
                    memset(buffer, 0, 200);
                    sprintf_P(buffer, PSTR(", "));
                    resp->print(buffer);
                  };

                  memset(buffer, 0, 200);
                  sprintf_P(buffer, PSTR("{\"Lat\": %f, \"Lon\": %f, \"Alt\": %f, \"Time\": \"%4i-%02i-%02i %02i:%02i:%02i\"}"), fLat, fLon, fAlt, dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
                  resp->println(buffer);

                  bHadEntry = true;
                };
                
                ResetWatchDog();

                pRecordset->moveNext();
              };

              delete pRecordset;
            };


            // we are done, send the footer
            memset(buffer, 0, sizeof(buffer));
            strcpy_P(buffer, PSTR("]}"));
            resp->println(buffer);
            
            delete pDatabase;
            delete buffer;

            docResp.clear();

            return;
          }
          else
          {
            //cant load data, when recording is active, since
            //the file is opened and closed after every waypoint
            //so in the end one operation will fail, when the
            //same file is opened twice...
            
            docResp["response"] = String(F("ERR"));
            nRespCode = 500;
          };

          delete buffer;
        };

        if(strcmp_P(doc[F("chatcmd")], PSTR("stopTrackRecord")) == 0)
        {         
          if(g_bRecordTrack == true)
          {
            g_pDbTaskScheduler->removeSchedule(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_RECORD_TRACK), true);

            g_dwRecordTrackID = 0;
            g_bRecordTrack    = false;
  
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
          }
          else
          {
            docResp["response"] = String(F("ERR"));
            nRespCode = 500;
          };
        };

        if(strcmp_P(doc[F("chatcmd")], PSTR("deleteTrack")) == 0)
        {
          //variables
          ///////////
          char                szDatabaseFile[50];
          CWSFFileDB          *pDatabase;
          CWSFFileDBRecordset *pRecordset;
          uint32_t            dwTrackID;
          
          if(g_bRecordTrack == false)
          {
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, TRACKHEADTABLE_FILE);
  
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackHeadTableDef, TRACKHEADTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
            pDatabase->open();
      
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase, (uint32_t)doc[F("TrackID")]);
  
              if(pRecordset->haveValidEntry() == true)
              {
                pRecordset->remove();

                delete pRecordset;
                delete pDatabase;

                //delete waypoints
                memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
                sprintf((char*)&szDatabaseFile, TRACKWAYPOINTTABLE_FILE);
    
                pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackWaypointTableDef, TRACKWAYPOINTTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
                pDatabase->open();
                
                if(pDatabase->isOpen() == true)
                {
                  pRecordset = new CWSFFileDBRecordset(pDatabase);
    
                  while(pRecordset->haveValidEntry() == true)
                  {
                    pRecordset->getData(0, (void*)&dwTrackID, sizeof(uint32_t));
    
                    if(dwTrackID == (uint32_t)doc[F("TrackID")])
                    {
                      pRecordset->remove();
                    };

                    ResetWatchDog();

                    pRecordset->moveNext();
                  };

                  delete pRecordset;
                };

                docResp["response"] = String(F("OK"));
                nRespCode = 200;
              }
              else 
              {
                delete pRecordset;

                docResp["response"] = String(F("ERR"));
                nRespCode = 500;
              };

              delete pDatabase;
            }
            else 
            {
              docResp["response"] = String(F("ERR"));
              nRespCode = 500;

              delete pDatabase;
            };
          }
          else
          {
            //cant load data, when recording is active, since
            //the file is opened and closed after every waypoint
            //so in the end one operation will fail, when the
            //same file is opened twice...
            
            docResp["response"] = String(F("ERR"));
            nRespCode = 500;
          };
        };
      #endif

      //functions for which the internal clock is neccessary!
      if(nRespCode == 0)
      {
        if(ClockPtr->timeSet() == true)
        {
          if(strcmp_P(doc[F("chatcmd")], PSTR("newChatMsg")) == 0)
          {        
            if((strlen((const char*)doc[F("rcptTo")]) >= 3) && (strlen((const char*)doc[F("rcptTo")]) <= 50) && (strlen((const char*)doc[F("msg")]) > 0))
            {
              pNewMsg                   = new _sNewMessage;
  
              pNewMsg->nMsgType         = DATATABLE_DATATYPE_MSG;
              pNewMsg->strRcptTo        = (const char*)doc[F("rcptTo")]; 
              pNewMsg->dwRcptDevID      = (uint32_t)doc[F("devID")];
              pNewMsg->strUser          = split(pNewMsg->strRcptTo, '@', 0);
              pNewMsg->strDev           = split(pNewMsg->strRcptTo, '@', 1);
              pNewMsg->dwUserID         = (uint32_t)doc[F("userID")]; 
              pNewMsg->dwRcptUserID     = (uint32_t)doc[F("rcptID")];
              pNewMsg->dwRcptContactID  = (uint32_t)doc[F("contactID")];
        
              memset(pNewMsg->szMsg, 0, LORALINK_MAX_MESSAGE_SIZE);
              strcpy(pNewMsg->szMsg, (const char*)doc[F("msg")]);
    
              
              if((pNewMsg->strUser.length() > 0) && (pNewMsg->strDev.length() > 0))
              {
                g_pMessages->addItem(pNewMsg);
    
                docResp["response"] = String(F("OK"));
                nRespCode = 200;
              }
              else
              {
                docResp["response"] = String(F("ERROR - param"));
                nRespCode = 503;
  
                delete pNewMsg;
              };
            }
            else
            {
              docResp["response"] = String(F("ERROR - param"));
              nRespCode = 503;
            };
          };
  
  
          if(strcmp_P(doc[F("chatcmd")], PSTR("newShoutOut")) == 0)
          {
            //variables
            ///////////
            char szUser[30];
  
            memset(szUser, 0, sizeof(szUser));
            strcpy(szUser, (const char*)doc[F("userName")]);
  
            if(strlen(szUser) > 0)
            {
              pNewMsg = new _sNewMessage;
    
              pNewMsg->nMsgType         = DATATABLE_DATATYPE_SHOUT;
              pNewMsg->strUser          = szUser;
              pNewMsg->strDev           = DeviceConfig.szDevName;
              pNewMsg->dwUserID         = (uint32_t)doc[F("userID")]; 
        
              memset(pNewMsg->szMsg, 0, LORALINK_MAX_MESSAGE_SIZE);
              strcpy(pNewMsg->szMsg, (const char*)doc[F("msg")]);
    
              
              if((pNewMsg->strUser.length() > 0) && (pNewMsg->strDev.length() > 0))
              {
                g_pMessages->addItem(pNewMsg);
    
                docResp["response"] = String(F("OK"));
                nRespCode = 200;
              }
              else
              {
                docResp["response"] = String(F("ERROR - param"));
                nRespCode = 503;
    
                delete pNewMsg;
              };
            }
            else
            {
              docResp["response"] = String(F("ERROR - param"));
              nRespCode = 503;
            };
          };
        }
        else
        {
          docResp["response"] = String(F("ERROR - time"));
          nRespCode = 503;
        };
      };
    }
    else
    {
      docResp["response"] = String(F("ERROR - wrong credentials"));
      nRespCode = 404;
    };
  
      
    sendJsonResponse(resp, nRespCode, docResp);
    docResp.clear();

    ResetWatchDog();
  };


  void handleConfigRequests(HTTPResponse *resp, char *pData, int nDataLength, DynamicJsonDocument doc)
  {
    //variables
    ///////////
    int                 nRespCode = 200;
    bool                bHandled  = false;
    DynamicJsonDocument docResp(1500);
            
    //commands which doesnt require a valid login
    /////////////////////////////////////////////
    if(strcmp_P(doc[F("command")], PSTR("AdminLogin")) == 0)
    {
      if(CheckApiAdminPassword(doc[F("Username")], doc[F("Password")]) == true)
      {
        docResp[F("response")] = String(F("OK"));
        
        nRespCode = 200;
        bHandled  = true;
      }
      else
      {
        sendAnswerWrongCreds(resp);

        return;
      };
    };


    if(strcmp_P(doc[F("command")], PSTR("GetWiFi")) == 0)
    {
      //variables
      ///////////
      JsonObject            root        = docResp.to<JsonObject>();
      JsonArray             networks    = root.createNestedArray("networks");
    
      if((int)doc[F("scanWiFi")] == 1)
      {
        Serial.println(F("Scan for networks:"));
  
        //scan for networks
        int nAvailNetworks = WiFi.scanNetworks();
      
        if(nAvailNetworks > 0)
        {
          for(int n = 0; n < nAvailNetworks; ++n) 
          {
            Serial.print(F("Found WiFi Net: "));
            Serial.println(WiFi.SSID(n));
    
            networks.add(WiFi.SSID(n));
          };      
        };
      };
      
      PrepareSerializeWiFiConfig(docResp, &LoRaWiFiCfg);

      docResp["response"] = String(F("OK"));
      nRespCode           = 200;
      bHandled            = true;
    };


    
    if(strcmp_P(doc[F("command")], PSTR("createFolder")) == 0)
    {
      //variables
      ///////////
      String strFolder = String((const char*)doc[F("Folder")]) + String((const char*)doc[F("NewFolder")]);

      //folders are only supported on
      //fatfs (sd cards)
      #if LORALINK_HARDWARE_SDCARD == 1
        SD.mkdir(strFolder);

        docResp["response"] = String(F("OK"));
        nRespCode           = 200;
        bHandled            = true;
      #else
        docResp["response"] = String(F("ERR"));
        nRespCode           = 500;
        bHandled            = true;
      #endif
    };


    //used to check if a specific file exist on the webserver
    if(strcmp_P(doc[F("command")], PSTR("fileExist")) == 0)
    { 
      if(LORALINK_WEBAPP_FS.exists((const char*)doc[F("file")]) == true)
      {
        docResp["response"] = String(F("OK"));
      }
      else
      {
        docResp["response"] = String(F("ERR"));
      };

      nRespCode           = 200;
      bHandled            = true;
    };


    if(strcmp_P(doc[F("command")], PSTR("GetDeviceCfg")) == 0)
    {  
      docResp["response"] = String(F("OK"));
      
      PrepareSerializeDeviceConfig(docResp);
      
      nRespCode = 200;

      bHandled = true;
    };


    if(strcmp_P(doc[F("command")], PSTR("GetLinkCfg")) == 0)
    {
      docResp["response"] = String(F("OK"));
      PrepareSerializeLinkConfig(docResp);
      
      nRespCode = 200;

      bHandled = true;
    };


    if(strcmp_P(doc[F("command")], PSTR("GetDDNSCfg")) == 0)
    {
      docResp["response"] = String(F("OK"));
      PrepareSerializeDynDNSConfig(docResp);
      
      nRespCode = 200;
      bHandled = true;
    };


    if(strcmp_P(doc[F("command")], PSTR("GetFilesystemCfg")) == 0)
    {
      #if LORALINK_HARDWARE_SPIFFS == 1
        docResp["SPIFFS"] = true;
      #else
        docResp["SPIFFS"] = false;
      #endif
  
      #if LORALINK_HARDWARE_SDCARD == 1
        docResp["SDCARD"] = true;
      #else
        docResp["SDCARD"] = false;
      #endif
      
      docResp["response"] = String(F("OK"));
      
      nRespCode = 200;

      bHandled = true;
    };


    if(strcmp_P(doc[F("command")], PSTR("CreateUser")) == 0)
    {
      //variables
      ///////////
      char                *szUser = new char[31];
      char                *szMail = new char[256];
      char                *szPwd  = new char[40];
           
      memset(szUser, 0, 31);
      memset(szMail, 0, 256);
      memset(szPwd, 0, 40);
      
      strcpy(szUser, doc[F("UserName")]);
      strcpy(szMail, doc[F("mail")]);
      strcpy(szPwd,  doc[F("pwd")]);
    
      if((strlen(szUser) > 0) && (strlen(szPwd) > 0))
      {       
        if(g_pUserTable->getRecordCount() < DeviceConfig.nMaxUser)
        {
          if(CreateUser(szUser, szPwd, szMail) == true)
          {
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
          }
          else 
          {
            docResp["response"] = String(F("ERR - User Exist!"));  
            nRespCode = 429;
          };
        }
        else
        {
          docResp["response"] = String(F("ERR - Max Users Reached!"));  
          nRespCode = 429;
        };
      }
      else
      {
        docResp["response"] = String(F("ERR - param"));  
        nRespCode = 200;
      };

      bHandled = true;

      delete szUser;
      delete szMail;
      delete szPwd;
    };


    if(strcmp_P(doc[F("command")], PSTR("UserLogin")) == 0)
    {
      CWSFFileDBRecordset  *rsUser;     
      char                 szHash[41];
      byte                 bBlocked = 0;
      uint32_t             dwTime = ClockPtr->getUnixTimestamp();
  
      if((strlen((const char*)doc[F("Username")]) > 0) && (strlen((const char*)doc[F("Password")]) > 0))
      {
        rsUser = new CWSFFileDBRecordset(g_pUserTable);
    
        if(FindUserByName(rsUser, (const char*)doc[F("Username")]) == true)
        {
          if(rsUser->getData(5, (char*)&szHash, 40) == true)
          {
            if(strcasecmp(szHash, (const char*)doc[F("Password")]) == 0)
            {
              if(rsUser->getData(8, (byte*)&bBlocked, sizeof(bBlocked)) == true)
              {    
                if(bBlocked == 0)
                {
                  //update last login
                  rsUser->setData(1, (void*)&dwTime, sizeof(dwTime));
  
                  docResp["UserID"] = rsUser->getRecordPos();
                  docResp["response"] = String(F("OK"));
                }
                else
                {
                  docResp["response"] = String(F("ERR - Blocked"));
                };
              };
            };
          }
          else
          {
            docResp["response"] = String(F("ERROR - wrong credentials (PW)"));
            nRespCode = 404;
          };
        }
        else
        {
          docResp["response"] = String(F("ERROR - User does not exist!"));
          nRespCode = 404;
        };
  
        delete rsUser;
      }
      else
      {
        docResp["response"] = String(F("ERROR - wrong credentials (PW)"));
        nRespCode = 404;
      };

      bHandled = true;
    }; 
  

    if(strcmp_P(doc[F("command")], PSTR("readWebEvents")) == 0)
    {
      //variables
      ///////////
      uint32_t dwUserID = doc[F("userID")];
      uint32_t dwDataID = 0;
      char    *szData   = new char[WEB_API_MAX_STRING_SIZE + 1];
      char    *szResp   = new char[WEB_API_MAX_STRING_SIZE + 501];
      int     nEventID;
      
      memset(szData, 0, WEB_API_MAX_STRING_SIZE); 
      memset(szResp, 0, WEB_API_MAX_STRING_SIZE + 500);

      nEventID = g_pWebEvent->getEvent(dwUserID, (uint32_t*)&dwDataID, szData);

      sprintf_P(szResp, PSTR("{\"nEventID\": %i, \"dwDataID\": %u, \"bConnected\": %s, \"szData\": \"%s\", \"response\": \"OK\", \"lUptimeSec\": %u"), 
        nEventID, 
        dwDataID, 
        (LLSystemState.bConnected == true ? "true" : "false"), 
        szData, 
        millis() / 1000
      );

      #if LORALINK_HARDWARE_GPS == 1
        sprintf_P(szResp + strlen(szResp), 
          PSTR(", \"bHaveGPS\": true, \"bRecordTrack\": %s, \"bValidSignal\": %s, \"fLatitude\": %f, \"fLongitude\": %f, \"fAltitude\": %f, \"bTrackingActive\": %s, \"nTrackingType\": %i, \"fSpeed\": %f, \"fCourse\": %f, \"nHDOP\": %i, \"nNumSat\": %i, \"dwLastValid\": %u}"), 
          (g_bRecordTrack == true ? "true" : "false"),
          (LLSystemState.bValidSignal == true ? "true" : "false"),  
          LLSystemState.fLatitude,
          LLSystemState.fLongitude,
          LLSystemState.fAltitude,
          (g_bLocationTrackingActive == true ? "true" : "false"),  
          g_nLocationTrackingType,
          LLSystemState.fSpeed,
          LLSystemState.fCourse,
          LLSystemState.nHDOP,
          LLSystemState.nNumSat,
          (millis() - LLSystemState.dwLastValid) / 1000
        );
        
      #else
        sprintf_P(szResp + strlen(szResp), 
          PSTR(", \"bHaveGPS\": false}");
        );
      #endif
      
      sendStringResponse(resp, 200, (char*)(String(F("application/json"))).c_str(), szResp);
      
      delete szData;
      delete szResp;

      return;
    };


    if(strcmp_P(doc[F("command")], PSTR("getNodeList")) == 0)
    {
      //variables
      ///////////
      CWSFFileDBRecordset *pRecordset = new CWSFFileDBRecordset(g_pNodeTable);
      char buffer[200];
      bool bRes = false;
      byte bData;
      uint32_t dwData;
      float fData;
      int nData;
      char szData[101];
      DateTime dtTime;
 
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)&buffer, PSTR("{\"Nodes\": ["));
      resp->println(buffer);

      
      while(pRecordset->haveValidEntry() == true)
      {
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("["));
        resp->println(buffer);

        
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%u\", "), pRecordset->getRecordPos());
        resp->println(buffer);
        

        //id  
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        dwData = 0;
        pRecordset->getData(0, (void*)&dwData, sizeof(dwData));
        sprintf_P((char*)&buffer, PSTR("\"%u\", "), dwData);
        resp->println(buffer);

        //device name
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        pRecordset->getData(1, (void*)&szData, sizeof(szData));
        sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
        resp->println(buffer);

        //last heard
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        dwData = 0;
        pRecordset->getData(6, (void*)&dwData, sizeof(dwData));
        dtTime = DateTime((time_t)dwData);
        sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
        sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
        resp->println(buffer);

        //pos N
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        fData = 0;
        pRecordset->getData(3, (void*)&fData, sizeof(fData));
        sprintf_P((char*)&buffer, PSTR("\"%f\", "), fData);
        resp->println(buffer);

        //pos E
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        fData = 0;
        pRecordset->getData(4, (void*)&fData, sizeof(fData));
        sprintf_P((char*)&buffer, PSTR("\"%f\", "), fData);
        resp->println(buffer);

        //rssi
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        nData = 0;
        pRecordset->getData(7, (void*)&nData, sizeof(nData));
        sprintf_P((char*)&buffer, PSTR("\"%i\", "), nData);
        resp->println(buffer);

        //snr
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        fData = 0;
        pRecordset->getData(8, (void*)&fData, sizeof(fData));
        sprintf_P((char*)&buffer, PSTR("\"%f\""), fData);
        resp->println(buffer);
        
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("]"));
        resp->println(buffer);
        
        
        if(pRecordset->moveNext() == true)
        {
          memset(buffer, 0, sizeof(buffer));
          sprintf_P((char*)&buffer, PSTR(", "));
          resp->println(buffer);
        };

        ResetWatchDog();
      };

      // we are done, send the footer
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)&buffer, PSTR("]}"));
      resp->println(buffer);
      
      delete pRecordset;

      docResp.clear();

      return;
    };


    if(strcmp_P(doc[F("command")], PSTR("getRoutes")) == 0)
    {
      //variables
      ///////////
      _sListItem            *item       = g_pRoutingEntries->getList();
      CWSFFileDBRecordset   *pRecordset = new CWSFFileDBRecordset(g_pRoutingTable);
      _sSkyNetRoutingEntry  *routing; 
      char                  buffer[200];
      uint32_t              dwData;
      int                   nData;
      
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)&buffer, PSTR("{\"Routes\": ["));
      resp->print(buffer);

      //get L0 routing
      if(g_pRoutingEntries->getItemCount() > 0)
      {
        while(item != NULL)
        {
          routing = (_sSkyNetRoutingEntry*)item->pItem;
    
          if(routing != NULL)
          {
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("["));
            resp->print(buffer);

            //dev id        
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("\"%u\", "), routing->dwDeviceID);
            resp->print(buffer);
            
            //transport node id  
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("\"%u\", "), routing->dwViaNode);
            resp->print(buffer);

            //Conn type
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("\"%i\", "), routing->pConnHandler->getConnectionType());
            resp->print(buffer);

            //hop count
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("\"%i\", "), routing->dwHopCount);
            resp->print(buffer);

            //Dev type
            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("\"%i\""), routing->nDevType);
            resp->print(buffer);

            memset(buffer, 0, sizeof(buffer));
            sprintf_P((char*)&buffer, PSTR("]"));
            resp->print(buffer);
        
            item = item->pNext;
              
            if(item != NULL)
            {
              memset(buffer, 0, sizeof(buffer));
              sprintf_P((char*)&buffer, PSTR(", "));
              resp->println(buffer);
            };
          };
          
          ResetWatchDog();
        };

        if(pRecordset->haveValidEntry() == true)
        {
          memset(buffer, 0, sizeof(buffer));
          sprintf_P((char*)&buffer, PSTR(", "));
          resp->println(buffer);
        };
      };

      //get routing from database
      //> L0
      while(pRecordset->haveValidEntry() == true)
      {
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("["));
        resp->print(buffer);

        //dev id    
        pRecordset->getData(0, (void*)&dwData, sizeof(dwData));    
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%u\", "), 0);
        resp->print(buffer);
        
        //transport node id  
        pRecordset->getData(1, (void*)&dwData, sizeof(dwData)); 
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%u\", "), dwData);
        resp->print(buffer);

        //Conn type
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%i\", "), 0);
        resp->print(buffer);

        //hop count  
        pRecordset->getData(2, (void*)&dwData, sizeof(dwData)); 
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%u\", "), dwData);
        resp->print(buffer);

        //dev type  
        pRecordset->getData(3, (void*)&nData, sizeof(nData)); 
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"%i\""), nData);
        resp->print(buffer);

        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("]"));
        resp->print(buffer);

        pRecordset->moveNext();

        if(pRecordset->haveValidEntry() == true)
        {
          memset(buffer, 0, sizeof(buffer));
          sprintf_P((char*)&buffer, PSTR(", "));
          resp->println(buffer);
        };
      };

      // we are done, send the footer
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)&buffer, PSTR("]}"));
      resp->println(buffer);

      docResp.clear();
      delete pRecordset;
      
      return;
    };




    if(strcmp_P(doc[F("command")], PSTR("getKnownNodes")) == 0)
    {
      //variables
      ///////////
      CWSFFileDBRecordset *pRecordset           = new CWSFFileDBRecordset(g_pNodeTable);
      bool bRes = false;
      byte bData;
      uint32_t dwData;
      float fData;
      char szData[101];
      DateTime dtTime;
      char buffer[200];
         
         
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)buffer, PSTR("{\"Nodes\": ["));
      resp->println(buffer);
      
      while(pRecordset->haveValidEntry() == true)
      {
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("{"));
        resp->println(buffer);
        
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("\"EntryID\": %u, "), pRecordset->getRecordPos());
        resp->println(buffer);
        
        //id
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        dwData = 0;
        pRecordset->getData(0, (void*)&dwData, sizeof(dwData));
        sprintf_P(szData, PSTR("%u\0"), dwData);
        sprintf_P((char*)&buffer, PSTR("\"NodeID\": \"%s\", "), szData);
        resp->println(buffer);

        //device name
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        pRecordset->getData(1, (void*)&szData, sizeof(szData));
        sprintf_P((char*)&buffer, PSTR("\"DevName\": \"%s\", "), szData);
        resp->println(buffer);
        

        //last heard
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        dwData = 0;
        pRecordset->getData(6, (void*)&dwData, sizeof(dwData));
        dtTime = DateTime((time_t)dwData);
        sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
        sprintf_P((char*)&buffer, PSTR("\"LastHeard\": \"%s\", "), szData);
        resp->println(buffer);

        //pos N
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        fData = 0;
        pRecordset->getData(3, (void*)&fData, sizeof(fData));
        sprintf_P((char*)&buffer, PSTR("\"posN\": \"%f\", "), fData);
        resp->println(buffer);

        //pos E
        memset(buffer, 0, sizeof(buffer));
        memset(szData, 0, sizeof(szData));
        fData = 0;
        pRecordset->getData(4, (void*)&fData, sizeof(fData));
        sprintf_P((char*)&buffer, PSTR("\"posE\": \"%f\""), fData);
        resp->println(buffer);
        
        memset(buffer, 0, sizeof(buffer));
        sprintf_P((char*)&buffer, PSTR("}"));
        resp->println(buffer);
        
        
        if(pRecordset->moveNext() == true)
        {
          memset(buffer, 0, sizeof(buffer));
          sprintf_P((char*)&buffer, PSTR(", "));
          resp->println(buffer);
        };

        ResetWatchDog();
      };

      // we are done, send the footer
      memset(buffer, 0, sizeof(buffer));
      strcpy_P((char*)&buffer, PSTR("]}"));
      resp->println(buffer);
      
      delete pRecordset;

      docResp.clear();

      return;
    };


    if(strcmp_P(doc[F("command")], PSTR("UpdateAdminLogin")) == 0)
    {
      DynamicJsonDocument doc2(sizeof(sAdminConfig) + 50);
      char      szOutput[sizeof(sAdminConfig) + 50];
      int       nLength;

      if(CheckApiAdminPassword(doc[F("OldUsername")], doc[F("OldPassword")]) == true)
      {
        DeSerializeAdminConfig(doc); 
        PrepareSerializeAdminConfig(doc2);
  
        nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
  
        if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_ADMIN, (byte*)&szOutput, nLength) == true)
        {
          Serial.print(F("Config written..."));
        };
  
        nRespCode = 200;
        docResp["response"] = String(F("OK"));
        
        doc2.clear();
      }
      else
      {
        nRespCode = 403;
        docResp["response"] = String(F("ERR"));
      };

      bHandled = true;
    };


    if(bHandled == false)
    {
      //API Commands which require admin access
      /////////////////////////////////////////
      if((doc.containsKey(F("Username")) == true) && (doc.containsKey(F("Password")) == true))
      {
        if(CheckApiAdminPassword(doc[F("Username")], doc[F("Password")]) == true)
        {
          if(strcmp_P(doc[F("command")], PSTR("GetWiFiAP")) == 0)
          {
            docResp["response"] = String(F("OK"));
            
            PrepareSerializeWiFiConfig(docResp, &LoRaWiFiApCfg);
            nRespCode = 200;
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("GetModemCfg")) == 0)
          {  
            docResp["response"] = String(F("OK"));
            
            PrepareSerializeModemConfig(docResp);
            nRespCode = 200;
            bHandled = true;
          };


          if(strcmp_P(doc[F("command")], PSTR("deviceReset")) == 0)
          {  
            ESP.restart();
          };
    
    
          
          if(strcmp_P(doc[F("command")], PSTR("SetWiFi")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sLoRaWiFiCfg) + 100);
            char      szOutput[sizeof(sLoRaWiFiCfg) + 100];
            int       nLength;
      
            DeSerializeWiFiConfig(doc, &LoRaWiFiCfg); 
            PrepareSerializeWiFiConfig(doc2, &LoRaWiFiCfg);
      
            nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_CLNT, (byte*)&szOutput, nLength) == true)
            {
              Serial.print(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
           
            doc2.clear();
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("SetModemCfg")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sModemConfig) + 300);
            char      szOutput[sizeof(sModemConfig) + 300];
            int       nLength;
      
            DeSerializeModemConfig(doc);
            PrepareSerializeModemConfig(doc2);
      
            nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_MODEM, (byte*)&szOutput, nLength) == true)
            {
              Serial.print(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
            
            doc2.clear();
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("SetWiFiAP")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sLoRaWiFiCfg) + 100);
            char      szOutput[sizeof(sLoRaWiFiCfg) + 100];
            int       nLength;
      
            DeSerializeWiFiConfig(doc, &LoRaWiFiApCfg);
        
            PrepareSerializeWiFiConfig(doc2, &LoRaWiFiApCfg);
      
            nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_AP, (byte*)&szOutput, nLength) == true)
            {
              Serial.print(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
      
            doc2.clear();
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("SetDDNSCfg")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sDynamicDNSConfig) + 250);
            char      szOutput[sizeof(sDynamicDNSConfig) + 250];
            int       nLength;
      
            DeSerializeDynDNSConfig(doc);
      
            PrepareSerializeDynDNSConfig(doc2);
      
            nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_DDNS, (byte*)&szOutput, nLength) == true)
            {
              Serial.println(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
              
            doc2.clear();
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("SetLinkCfg")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sIpLinkConfig) + 250);
            char      szOutput[sizeof(sIpLinkConfig) + 250];
            int       nLength;
          
            DeSerializeLinkConfig(doc);
      
            PrepareSerializeLinkConfig(doc2);
      
            nLength = serializeJson(doc2, (char*)&szOutput, sizeof(szOutput));
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_IPLINK, (byte*)szOutput, nLength) == true)
            {
              Serial.println(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
      
            doc2.clear();
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("SetDeviceCfg")) == 0)
          {
            //variables
            ///////////
            DynamicJsonDocument doc2(sizeof(sDeviceConfig) + 500);
            char      *szOutput = new char[sizeof(sDeviceConfig) + 500];
            int       nLength;
      
            DeSerializeDeviceConfig(doc);
      
            PrepareSerializeDeviceConfig(doc2);
      
            nLength = serializeJson(doc2, szOutput, sizeof(sDeviceConfig) + 500);
      
            if(WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_DEVICE, (byte*)szOutput, nLength) == true)
            {
              Serial.println(F("Config written..."));
            };
      
            docResp["response"] = String(F("OK"));
            nRespCode = 200;
      
            doc2.clear();
            bHandled = true;
            delete szOutput;
          };


              
          if(strcmp_P(doc[F("command")], PSTR("deleteFile")) == 0)
          {
            docResp["response"] = String(F("ERR"));
            nRespCode = 403;
                
            #if LORALINK_HARDWARE_SPIFFS == 1
              if(strcmp_P(g_szFileSystem, PSTR("SPIFFS")) == 0)
              {
                SPIFFS.remove((const char*)doc[F("File")]);
                docResp["response"] = String(F("OK"));
                nRespCode = 200;
              }
              else
              {
                docResp["response"] = String(F("ERR"));
                nRespCode = 403;
              };
            #endif
      
            #if LORALINK_HARDWARE_SDCARD == 1
              if(strcmp_P(g_szFileSystem, PSTR("SDCARD")) == 0)
              {
                SD.remove((const char*)doc[F("File")]);
                docResp["response"] = String(F("OK"));
                nRespCode = 200;
              }
              else
              {
                docResp["response"] = String(F("ERR"));
                nRespCode = 403;
              };
            #endif
  
            bHandled = true;
          };
    
          //block user account on device level
          if(strcmp_P(doc[F("command")], PSTR("blockUser")) == 0)
          {
            //variables
            ///////////
            CWSFFileDBRecordset *rsUser;
            uint32_t            dwID = 0;
            byte                bBlocked = 0;
      
            dwID = doc[F("ID")];
            bBlocked = doc[F("blocked")];
            rsUser = new CWSFFileDBRecordset(g_pUserTable, dwID);
        
            if(rsUser->haveValidEntry() == true)
            {
              rsUser->setData(8, (void*)&bBlocked, sizeof(bBlocked));
        
              docResp["response"] = String(F("OK"));
            }
            else 
            {
              docResp["response"] = String(F("ERR"));  
            };
        
            delete rsUser;
            bHandled = true;
          };


          
          if(strcmp_P(doc[F("command")], PSTR("resetUserPwd")) == 0)
          {
            //variables
            ///////////
            CWSFFileDBRecordset *rsUser;
            uint32_t            dwID = 0;
            char                szHash[50];
      
            dwID = doc[F("ID")];
            memset(szHash, 0, sizeof(szHash));
            strcpy(szHash, (const char*)doc[F("newPassword")]);
            
            rsUser = new CWSFFileDBRecordset(g_pUserTable, dwID);
        
            if(rsUser->haveValidEntry() == true)
            {
              rsUser->setData(5, (void*)&szHash, sizeof(szHash));
        
              docResp["response"] = String(F("OK"));
            }
            else 
            {
              docResp["response"] = String(F("ERR"));  
            };
        
            delete rsUser;
            bHandled = true;
          };
    
     
          //delete user incl. data
          if(strcmp_P(doc[F("command")], PSTR("deleteUser")) == 0)
          {
            //variables
            ///////////
            CWSFFileDBRecordset *rsUser;
            uint32_t            dwID = 0;
            char                szFileName[50];
        
            dwID   = doc[F("ID")];
            rsUser = new CWSFFileDBRecordset(g_pUserTable, dwID);
        
            if(rsUser->haveValidEntry() == true)
            {
              rsUser->remove();


              //delete user data files
              memset(szFileName, 0, sizeof(szFileName));
              sprintf((char*)&szFileName, CONTACTSTABLE_FILE, dwID);

              LORALINK_DATA_FS.remove(szFileName);

              memset(szFileName, 0, sizeof(szFileName));
              sprintf((char*)&szFileName, CHATHEADTABLE_FILE, dwID);

              LORALINK_DATA_FS.remove(szFileName);

              memset(szFileName, 0, sizeof(szFileName));
              sprintf((char*)&szFileName, CHATMESSAGETABLE_FILE, dwID);

              LORALINK_DATA_FS.remove(szFileName);
              
        
              docResp["response"] = String(F("OK"));
            }
            else 
            {
              docResp["response"] = String(F("ERR"));  
            };
        
            delete rsUser;
            bHandled = true;
          };  
    
    
          if(strcmp_P(doc[F("command")], PSTR("getFiles")) == 0)
          {
            //variables
            ///////////
            char szFolder[50];
            bool bRes = false;
            byte bData;
            uint32_t dwData;
            float fData;
            char szData[101];
            uint64_t nFree = 0, nUsed = 0;
            File fRoot, file;
                    
            Serial.println(F("getFiles() prepare response..."));
        
            memset(g_szFileSystem, 0, sizeof(g_szFileSystem));
            strcpy(g_szFileSystem, doc[F("FileSystem")]);
        
            memset(szFolder, 0, sizeof(szFolder));
            strcpy(szFolder, doc[F("Folder")]);
                  
            #if LORALINK_HARDWARE_SPIFFS == 1    
              if(strcmp_P(g_szFileSystem, PSTR("SPIFFS")) == 0)
              {
                fRoot   = SPIFFS.open(szFolder);
                nFree   = LLSystemState.lIntSpiBytesAvail - LLSystemState.lIntSpiBytesUsed;
                nUsed   = LLSystemState.lIntSpiBytesUsed;
              };
            #endif
        
            #if LORALINK_HARDWARE_SDCARD == 1
              if(strcmp_P(g_szFileSystem, PSTR("SDCARD")) == 0)
              {
                fRoot   = SD.open(szFolder);
                nFree   = LLSystemState.lExtSpiBytesAvail - LLSystemState.lExtSpiBytesUsed;
                nUsed   = LLSystemState.lExtSpiBytesUsed;
              };
            #endif
                    
            if(fRoot)
            {
              file = fRoot.openNextFile();
            }
            else
            {
              Serial.print(F("No files in "));
              Serial.println(szFolder);
            };
      
            memset(szData, 0, sizeof(szData));
            sprintf_P((char*)&szData, PSTR("{\"Free\": %llu, \"Used\": %llu, \"Files\": ["), nFree, nUsed);
            resp->print(szData);

            //check if in another directory...
            //provide link to go back
            if(strcmp(szFolder, "/") != 0)
            {
              memset(szData, 0, sizeof(szData));
              sprintf_P((char*)&szData, PSTR("[\"1\", \"<a href='#' onclick='javascript: openDirectory(\\\"..\\\");'>..</a>\", \"0\", \"\"]"));
              resp->print(szData);
            }
            else
            {
              memset(szFolder, 0, sizeof(szFolder));
            };
          
            if(file)
            {
              if(strlen(szFolder) > 0)
              {
                memset(szData, 0, sizeof(szData));
                sprintf_P((char*)&szData, PSTR(","));
                resp->print(szData);
              };
              
              do
              {
                memset(szData, 0, sizeof(szData));
                sprintf_P((char*)&szData, PSTR("["));
                resp->print(szData);
          
                //is dir
                memset(szData, 0, sizeof(szData));
                sprintf_P((char*)&szData, PSTR("\"%i\", "), file.isDirectory());
                resp->print(szData);
          
                //name
                memset(szData, 0, sizeof(szData));
                
                if(file.isDirectory() == true)
                {
                  sprintf_P((char*)&szData, PSTR("\"<a href='#' onclick='javascript: openDirectory(\\\"%s/%s\\\");'>%s</a>\", "), szFolder, file.name(), file.name());
                }
                else
                {
                  sprintf_P((char*)&szData, PSTR("\"<a href='%s/%s' download='%s'>%s</a>\", "), szFolder, file.name(), file.name(), file.name());
                };
                  
                resp->print(szData);
          
                //size
                memset(szData, 0, sizeof(szData));
                         
                if(file.isDirectory() == false)
                {
                  sprintf_P((char*)&szData, PSTR("\"%i\", "), file.size());
                }
                else
                {
                  sprintf_P((char*)&szData, PSTR("\"%i\", "), 0);
                };
                  
                resp->print(szData);
      
                memset(szData, 0, sizeof(szData));
                
                //for delete link
                if(file.isDirectory() == false)
                {
                  sprintf_P((char*)&szData, PSTR("\"<a href='#' onclick='javascript: deleteFile(\\\"%s/%s\\\");'>delete</a>\""), szFolder, file.name());
                }
                else
                {
                  sprintf_P((char*)&szData, PSTR("\"\""));
                };
                
                resp->print(szData);
      
      
                memset(szData, 0, sizeof(szData));
                sprintf_P((char*)&szData, PSTR("]"));
                resp->print(szData);
      
                file.close();
       
                file = fRoot.openNextFile();
      
                if(file)
                {
                  memset(szData, 0, sizeof(szData));
                  sprintf_P((char*)&szData, PSTR(", "));
                  resp->print(szData);
                };
                
                ResetWatchDog();
              }
              while(file);
            };
        
            // we are done, send the footer
            memset(szData, 0, sizeof(szData));
            strcpy_P((char*)&szData, PSTR("]}"));
            resp->print(szData);
          
            fRoot.close();
            docResp.clear();
    
            return;
          };
      
    
          if(strcmp_P(doc[F("command")], PSTR("getUserList")) == 0)
          {
            //variables
            ///////////
            char buffer[200];
            CWSFFileDBRecordset *pRecordset           = new CWSFFileDBRecordset(g_pUserTable);
            bool bRes = false;
            byte bData;
            uint32_t dwData;
            float fData;
            char szData[101];
            DateTime dtTime;
            
            
            memset(buffer, 0, sizeof(buffer));
            strcpy_P((char*)&buffer, PSTR("{\"User\": ["));
            resp->println(buffer);
               
            while(pRecordset->haveValidEntry() == true)
            {   
              memset(buffer, 0, sizeof(buffer)); 
              sprintf_P((char*)&buffer, PSTR("["));
              resp->println(buffer);
      
              sprintf_P((char*)&buffer, PSTR("\"%u\", "), pRecordset->getRecordPos());
              resp->println(buffer);
      
              //user
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              pRecordset->getData(0, (void*)&szData, sizeof(szData));
              sprintf_P((char*)buffer, PSTR("\"%s\", "), szData);
              resp->println(buffer);
              
      
              //last login
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              dwData = 0;
              pRecordset->getData(1, (void*)&dwData, sizeof(dwData));
              dtTime = DateTime((time_t)dwData);
              sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
              sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
              resp->println(buffer);
    
      
              //last heard
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              dwData = 0;
              pRecordset->getData(2, (void*)&dwData, sizeof(dwData));
              dtTime = DateTime((time_t)dwData);
              sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
              sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
              resp->println(buffer);
              
    
              //user created
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              dwData = 0;
              pRecordset->getData(9, (void*)&dwData, sizeof(dwData));
              dtTime = DateTime((time_t)dwData);
              sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
              sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
              resp->println(buffer);
              
      
              //email
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              pRecordset->getData(3, (void*)&szData, sizeof(szData));
              sprintf_P((char*)&buffer, PSTR("\"%s\", "), szData);
              resp->println(buffer);  
              
      
              //forward Msgs
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              bData = 0;
              pRecordset->getData(4, (void*)&bData, sizeof(bData));
              sprintf_P((char*)&buffer, PSTR("\"%i\", "), bData);
              resp->println(buffer);
    
    
              //show date & time
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              bData = 0;
              pRecordset->getData(6, (void*)&bData, sizeof(bData));
              sprintf_P((char*)&buffer, PSTR("\"%i\", "), bData);
              resp->println(buffer);
                
      
              //user blocked
              memset(buffer, 0, sizeof(buffer));
              memset(szData, 0, sizeof(szData));
              bData = 0;
              pRecordset->getData(8, (void*)&bData, sizeof(bData));
              sprintf_P((char*)&buffer, PSTR("\"%i\""), bData);
              resp->println(buffer);
                
                
              memset(buffer, 0, sizeof(buffer));
              sprintf_P((char*)&buffer, PSTR("]"));
              resp->println(buffer);  
                
                
              if(pRecordset->moveNext() == true)
              {
                memset(buffer, 0, sizeof(buffer));
                sprintf_P((char*)&buffer, PSTR(", "));
                resp->println(buffer);
              };
    
              ResetWatchDog();
            };
    
    
            // we are done, send the footer
            memset(buffer, 0, sizeof(buffer));
            strcpy_P((char*)&buffer, PSTR("]}"));
            resp->println(buffer);
            
            delete pRecordset;
            docResp.clear();
    
            return;
          };
        };
      };
      
      //user level auth
      /////////////////
      if((doc.containsKey(F("userID")) == true) && (doc.containsKey(F("hash")) == true))
      {
        if(CheckApiUserPassword(doc[F("userID")], doc[F("hash")]) == true)
        {
          if(strcmp_P(doc[F("command")], PSTR("clearChat")) == 0)
          {
            deleteChatMessagesByID((uint32_t)doc[F("chatID")], (uint32_t)doc[F("userID")], true);
      
            docResp["response"] = String(F("OK"));  
            bHandled = true;
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("deleteChat")) == 0)
          {
            deleteChatByID((uint32_t)doc[F("chatID")], (uint32_t)doc[F("userID")]);
      
            docResp["response"] = String(F("OK"));
            bHandled = true;
          };
          
    
          if(strcmp_P(doc[F("command")], PSTR("resetChatNewMsgCount")) == 0)
          {
            //variables
            ///////////    
            char                  szDatabaseFile[50];
            CWSFFileDB            *pDatabase;
            CWSFFileDBRecordset   *pRecordset;
            int                   nValue = 0;
            
              
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, (uint32_t)doc[F("userID")]);
          
          
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, false, DBRESERVE_CHAT_COUNT);
      
            pDatabase->open();
      
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase, (uint32_t)doc[F("chatID")]);
      
              if(pRecordset->haveValidEntry() == true)
              {
                pRecordset->setData(5, (void*)&nValue, sizeof(nValue));
      
                docResp["response"] = String(F("OK"));
              }
              else
              {
                docResp["response"] = String(F("ERR"));
                nRespCode = 404;
              };
      
              delete pRecordset;
            }
            else
            {
              docResp["response"] = String(F("ERR DB"));
              nRespCode = 500;
            };
      
            delete pDatabase;
            bHandled = true;
          };


          if(strcmp_P(doc[F("command")], PSTR("blockUserContact")) == 0)
          {
            //variables
            ///////////
            char                szDatabaseFile[50];
            CWSFFileDB          *pDatabase;
            CWSFFileDBRecordset *pRecordset;
            int                 nBlocked = 0;

            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, (uint32_t)doc[F("userID")]);
      
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);
            pDatabase->open();

            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase, (uint32_t)doc[F("contactID")]);

              if(pRecordset->haveValidEntry() == true)
              {
                nBlocked = (int)doc[F("blocked")];

                pRecordset->setData(5, (void*)&nBlocked, sizeof(nBlocked));

                docResp["response"] = String(F("OK"));
                bHandled = true;
              }
              else
              {
                docResp["response"] = String(F("ERR"));
                bHandled = true;
              };

              delete pRecordset;
            };

            delete pDatabase;
          };
          
        
          if(strcmp_P(doc[F("command")], PSTR("loadUserContacts")) == 0)
          {
            //variables
            ///////////
            char                szDatabaseFile[50];
            CWSFFileDB          *pDatabase;
            CWSFFileDBRecordset *pRecordset;
            bool bRes = false;
            char szData[50];
            uint32_t dwData;
            int      nData;            
            char buffer[200];
    
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, (uint32_t)doc[F("userID")]);
            
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);
            pDatabase->open();
      
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase);
      
              memset(buffer, 0, sizeof(buffer));
              strcpy_P((char*)&buffer, PSTR("{\"response\": \"OK\", \"contacts\": ["));    
              resp->print(buffer);    
    
              while(pRecordset->haveValidEntry() == true)
              {      
                memset(buffer, 0, sizeof(buffer));
                strcpy_P((char*)&buffer, PSTR("{"));
                resp->print(buffer);
    
                memset(buffer, 0, sizeof(buffer));
                sprintf_P((char*)&buffer, PSTR("\"ID\": %u, "), pRecordset->getRecordPos());
                resp->print(buffer);
        
                //user id
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(0, (void*)&dwData, sizeof(dwData));
                sprintf_P((char*)&buffer, PSTR("\"UserID\": %u, "), dwData);
                resp->print(buffer);
        
                //dev id
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(1, (void*)&dwData, sizeof(dwData));
                sprintf_P((char*)&buffer, PSTR("\"DeviceID\": %u, "), dwData);
                resp->print(buffer);
                    
                //user
                memset(buffer, 0, sizeof(buffer));
                memset(szData, 0, sizeof(szData));
                pRecordset->getData(2, (void*)&szData, sizeof(szData));
                sprintf_P((char*)&buffer, PSTR("\"Name\": \"%s\", "), szData);
                resp->print(buffer);
                
    
                //dev
                memset(buffer, 0, sizeof(buffer));
                memset(szData, 0, sizeof(szData));
                pRecordset->getData(3, (void*)&szData, sizeof(szData));
                sprintf_P((char*)&buffer, PSTR("\"Device\": \"%s\", "), szData);
                resp->print(buffer);

                //blocked
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(5, (void*)&nData, sizeof(nData));
                sprintf_P((char*)&buffer, PSTR("\"Blocked\": %i, "), nData);
                resp->print(buffer);

                //blocked
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(6, (void*)&nData, sizeof(nData));
                sprintf_P((char*)&buffer, PSTR("\"AllowTracking\": %i, "), nData);
                resp->print(buffer);
    
                //state
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(4, (void*)&nData, sizeof(nData));
                sprintf_P((char*)&buffer, PSTR("\"State\": %i "), nData);
                resp->print(buffer);
                
                memset(buffer, 0, sizeof(buffer));
                strcpy_P((char*)&buffer, PSTR("}"));
                resp->print(buffer);
                            
                if(pRecordset->moveNext() == true)
                {
                  memset(buffer, 0, sizeof(buffer));
                  strcpy_P((char*)&buffer, PSTR(", "));
                  resp->print(buffer);
                };
    
                ResetWatchDog();
              };
    
              // we are done, send the footer
              memset(buffer, 0, sizeof(buffer));
              strcpy_P((char*)&buffer, PSTR("]}"));
              resp->print(buffer);
              
              delete pRecordset;
              delete pDatabase;

              docResp.clear();
              
              return;
            }
            else
            {
              delete pDatabase;
              
              docResp["response"] = String(F("ERR - no contacts"));
              bHandled = true;
            };
          };
        
    
          if(strcmp_P(doc[F("command")], PSTR("loadChatHead")) == 0)
          {
            //variables
            ///////////    
            CWSFFileDB            *pDatabase;
            char                  szDatabaseFile[50];
            CWSFFileDBRecordset   *pRecordset;
            CWSFFileDB            *pDB;
            CWSFFileDBRecordset   *pRS;
            bool bRes = false;
            char szData[55];
            uint32_t dwData;            
            int nData;
            DateTime dtTime;
            char szMsg[LORALINK_MAX_MESSAGE_SIZE + 1];
            char buffer[LORALINK_MAX_MESSAGE_SIZE + 200];
            
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CHATHEADTABLE_FILE, (uint32_t)doc[F("userID")]);
      
      
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatHeadTableDef, CHATHEADTABLE_SIZE, false, DBRESERVE_CHAT_COUNT);
            pDatabase->open();
      
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase);
                  
              memset(buffer, 0, sizeof(buffer));
              strcpy_P((char*)&buffer, PSTR("{\"response\": \"OK\", \"chatHeads\": ["));
              resp->println(buffer);
    
              while(pRecordset->haveValidEntry() == true)
              {
                memset(buffer, 0, sizeof(buffer));
                sprintf_P((char*)&buffer, PSTR("{"));
                resp->println(buffer);
    
                memset(buffer, 0, sizeof(buffer));
                sprintf_P((char*)&buffer, PSTR("\"ID\": %u, "), pRecordset->getRecordPos());
                resp->println(buffer);
        
                //chat type
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(0, (void*)&nData, sizeof(nData));
                sprintf_P((char*)&buffer, PSTR("\"ChatType\": %i, "), nData);
                resp->println(buffer);
        
                //chat recipient
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(1, (void*)&szData, sizeof(szData));
                sprintf_P((char*)&buffer, PSTR("\"ChatRcpt\": \"%s\", "), szData);
                resp->println(buffer);
                    
                //chat started time
                memset(buffer, 0, sizeof(buffer));
                memset(szData, 0, sizeof(szData));
                pRecordset->getData(2, (void*)&dwData, sizeof(dwData));
                dtTime = DateTime((time_t)dwData);
                sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
                sprintf_P((char*)&buffer, PSTR("\"StartTime\": \"%s\", "), szData);
                resp->println(buffer);
      
                //last msg id
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(3, (void*)&dwData, sizeof(dwData));
                sprintf_P((char*)&buffer, PSTR("\"LastMsgID\": %u, "), dwData);
                resp->println(buffer);
    
    
                memset(buffer, 0, sizeof(buffer));
                
                if(dwData != 0)
                {
                  memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
                  sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, (uint32_t)doc[F("userID")]);
    
                  memset(szMsg, 0, sizeof(szMsg));
    
                  pDB = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, false, DBRESERVE_CHATMSG_COUNT);
                  pDB->open();
    
                  if(pDB->isOpen() == true)
                  {
                    pRS = new CWSFFileDBRecordset(pDB, dwData);
    
                    if(pRS->haveValidEntry() == true)
                    {
                      pRS->getData(6, (byte*)&szMsg, sizeof(szMsg));
                    };
    
                    delete pRS;
                  };
    
                  delete pDB;
    
                  sprintf_P((char*)&buffer, PSTR("\"LastMsgText\": \"%s\", "), szMsg);
                }
                else
                {
                  sprintf_P((char*)&buffer, PSTR("\"LastMsgText\": \"\", "));
                };
    
                resp->println(buffer);
    
                    
      
                //last msg time
                memset(buffer, 0, sizeof(buffer));
                memset(szData, 0, sizeof(szData));
                pRecordset->getData(4, (void*)&dwData, sizeof(dwData));
                dtTime = DateTime((time_t)dwData);
                sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
                sprintf_P((char*)&buffer, PSTR("\"LastMsgTime\": \"%s\", "), szData);
                resp->println(buffer);
    
                    
                //unread msgs
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(5, (void*)&nData, sizeof(nData));
                sprintf_P((char*)&buffer, PSTR("\"UnreadMsgs\": %i, "), nData);
                resp->println(buffer);
    
                    
                //contact, group, shout out id
                memset(buffer, 0, sizeof(buffer));
                pRecordset->getData(6, (void*)&dwData, sizeof(dwData));
                sprintf_P((char*)&buffer, PSTR("\"ContactID\": %u "), dwData);
                resp->println(buffer);
      
    
                memset(buffer, 0, sizeof(buffer));
                sprintf_P((char*)&buffer, PSTR("}"));
                resp->println(buffer);
                    
                    
                if(pRecordset->moveNext() == true)
                {
                  memset(buffer, 0, sizeof(buffer));
                  sprintf_P((char*)&buffer, PSTR(", "));
                  resp->println(buffer);
                };
      
                ResetWatchDog();
              };
      
              // we are done, send the footer
              memset(buffer, 0, sizeof(buffer));
              strcpy_P((char*)&buffer, PSTR("]}"));
              resp->println(buffer);
    
              delete pRecordset;
              delete pDatabase;

              docResp.clear();
              
              return;
            }
            else
            {
              delete pDatabase;
              
              docResp["response"] = String(F("ERR - no chats"));
              bHandled = true;
            }; 
          };
    
  
          if(strcmp_P(doc[F("command")], PSTR("loadShoutOutMsgs")) == 0)
          {
            //variables
            ///////////
            CWSFFileDBRecordset   *pRecordset;
            uint32_t              dwData;
            int                   nData;
            char                  *szData = new char[LORALINK_MAX_MESSAGE_SIZE + 1];
            DateTime              dtTime;
            char                  *buffer = new char[LORALINK_MAX_MESSAGE_SIZE + 200];
            bool                  bNeedSep = false;
            
            
            if(g_pShoutOutTable->isOpen() == true)
            {
              if(doc.containsKey(F("EntryID")))
              {
                pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable, (uint32_t)doc[F("EntryID")]);
              }
              else
              {
                pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable);
              };
    
              memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);     
              strcpy_P(buffer, PSTR("{\"shoutoutMsgs\": ["));
              resp->print(buffer);
      
              while(pRecordset->haveValidEntry() == true)
              {
                if(bNeedSep == true)
                {
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);            
                  sprintf_P(buffer, PSTR(", "));
                  resp->print(buffer);
                };
                
                bNeedSep = true;
                  
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);     
                sprintf_P(buffer, PSTR("{"));
                resp->print(buffer);
    
                //print entry id
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);  
                sprintf_P(buffer, PSTR("\"ID\": %u, "), pRecordset->getRecordPos());
                resp->print(buffer);
      
                //sender
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);
                memset(szData, 0, LORALINK_MAX_MESSAGE_SIZE);
                pRecordset->getData(0, (void*)szData, LORALINK_MAX_MESSAGE_SIZE);
                sprintf_P(buffer, PSTR("\"Sender\": \"%s\", "), szData);
                resp->print(buffer);
  
                //send / Recieved time
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200); 
                memset(szData, 0, LORALINK_MAX_MESSAGE_SIZE);
                dwData = 0;
                pRecordset->getData(1, (void*)&dwData, sizeof(dwData));
                dtTime = DateTime((time_t)dwData);
                sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
                sprintf_P(buffer, PSTR("\"SentTime\": \"%s\", "), szData);
                resp->print(buffer);
  
  
                //message
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);
                memset(szData, 0, LORALINK_MAX_MESSAGE_SIZE + 1);
                pRecordset->getData(2, (void*)szData, LORALINK_MAX_MESSAGE_SIZE);
                sprintf_P(buffer, PSTR("\"Msg\": \"%s\" "), szData);
                resp->print(buffer);
                
                    
                memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);
                sprintf_P(buffer, PSTR("}"));
                resp->print(buffer);
  
                if(doc.containsKey(F("EntryID")))
                {
                  break;
                };
              
                      
        
                pRecordset->moveNext();
            
                ResetWatchDog();
              };
              
      
              //end of db
              //we are done, send the footer
              memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 200);
              strcpy_P(buffer, PSTR("]}"));
              resp->print(buffer);
                                  
              delete pRecordset;

              delete buffer;
              delete szData;

              docResp.clear();
              
              return;
            }
            else
            {
              delete buffer;
              delete szData;
              
              sendStringResponse(resp, 200, (char*)(String(F("application/json"))).c_str(), (char*)(String(F("{\"shoutoutMsgs\": []}"))).c_str());

              docResp.clear();
              
              return;
            };
          };
    
    
          if(strcmp_P(doc[F("command")], PSTR("loadChatMsgs")) == 0)
          {
            //variables
            ///////////
            char                  szDatabaseFile[50];
            CWSFFileDB            *pDatabase;
            CWSFFileDBRecordset   *pRecordset;
            uint32_t              dwData;
            int                   nData;
            char                  *szData = new char[LORALINK_MAX_MESSAGE_SIZE + 1];
            DateTime              dtTime;
            char                  *buffer = new char[LORALINK_MAX_MESSAGE_SIZE + 301];
            bool                  bNeedSep = false;
            
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, (uint32_t)doc[F("userID")]);
      
      
            pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, false, DBRESERVE_CHATMSG_COUNT);
            pDatabase->open();
      
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase);
    
              memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300);     
              strcpy_P(buffer, PSTR("{\"chatMsgs\": ["));
              resp->print(buffer);
      
              while(pRecordset->haveValidEntry() == true)
              {
                //chatHeadID
                pRecordset->getData(0, (void*)&dwData, sizeof(dwData));
    
                if(dwData == (uint32_t)doc[F("chatID")])
                {     
                  if(bNeedSep == true)
                  {
                    memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300);            
                    sprintf_P(buffer, PSTR(", "));
                    resp->print(buffer);
                  };
                  
                  bNeedSep = true;
                    
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300);     
                  sprintf_P(buffer, PSTR("{"));
                  resp->print(buffer);
      
                  //print entry id
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300);   
                  sprintf_P(buffer, PSTR("\"ID\": %u, "), pRecordset->getRecordPos());
                  resp->print(buffer);
        
                  //chat head id
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300);  
                  sprintf_P(buffer, PSTR("\"ChatHeadID\": %u, "), dwData);
                  resp->print(buffer);
      
                  //msg size
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  pRecordset->getData(1, (void*)&nData, sizeof(nData));
                  sprintf_P(buffer, PSTR("\"MsgSize\": %i, "), nData);
                  resp->print(buffer);
      
                  //Tx Complete
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  pRecordset->getData(2, (void*)&nData, sizeof(nData));
                  sprintf_P(buffer, PSTR("\"TxComplete\": %i, "), nData);
                  resp->print(buffer);
      
                  //Direction
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  pRecordset->getData(3, (void*)&nData, sizeof(nData));
                  sprintf_P(buffer, PSTR("\"Direction\": %i, "), nData);
                  resp->print(buffer);
                  
        
                  //send / Recieved time
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  memset(szData, 0, LORALINK_MAX_MESSAGE_SIZE);
                  dwData = 0;
                  pRecordset->getData(4, (void*)&dwData, sizeof(dwData));
                  dtTime = DateTime((time_t)dwData);
                  sprintf_P(szData, PSTR("%4i-%02i-%02i %02i:%02i:%02i\0"), dtTime.year(), dtTime.month(), dtTime.day(), dtTime.hour(), dtTime.minute(), dtTime.second());
                  sprintf_P(buffer, PSTR("\"MsgTime\": \"%s\", "), szData);
                  resp->print(buffer);
                  
      
                  //contact ID
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  pRecordset->getData(5, (void*)&dwData, sizeof(dwData));
                  sprintf_P(buffer, PSTR("\"ContactID\": %u, "), dwData);
                  resp->print(buffer);
                  
      
                  //message
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  memset(szData, 0, LORALINK_MAX_MESSAGE_SIZE);
                  pRecordset->getData(6, (void*)szData, LORALINK_MAX_MESSAGE_SIZE);
                  sprintf_P(buffer, PSTR("\"Message\": \"%s\", "), szData);
                  resp->print(buffer);
    
      
                  //read
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  pRecordset->getData(7, (void*)&nData, sizeof(nData));
                  sprintf_P(buffer, PSTR("\"MsgRead\": %i "), nData);
                  resp->print(buffer);
                      
                  memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
                  sprintf_P(buffer, PSTR("}"));
                  resp->println(buffer);
                };
                      
        
                pRecordset->moveNext();
            
                ResetWatchDog();
              };
              
      
              //end of db
              //we are done, send the footer
              memset(buffer, 0, LORALINK_MAX_MESSAGE_SIZE + 300); 
              strcpy_P(buffer, PSTR("]}"));
              resp->print(buffer);
                  
                  
              delete pRecordset;
              delete pDatabase;
              delete buffer;
              delete szData;

              docResp.clear();
    
              return;
            }
            else
            {
              sendStringResponse(resp, 200, (char*)(String(F("application/json"))).c_str(), (char*)(String(F("{\"chatMsgs\": []}"))).c_str());
              
              delete pDatabase;
              delete buffer;
              delete szData;
    
              return;
            };
          };
        };
      };
    };

    if(bHandled == false)
    {
      sendAnswerWrongCreds(resp);
    }
    else
    {
      sendJsonResponse(resp, nRespCode, docResp);
      docResp.clear();
    };
    
    ResetWatchDog();
  };


  //this method handles post requests from the web server
  void ApiCallbackHandler(void *req, void *res, char *pData, int nDataLength)
  {
    //variables
    ///////////
    HTTPRequest           *request = (HTTPRequest*)req;
    HTTPResponse          *resp    = (HTTPResponse*)res;
    DynamicJsonDocument   doc(nDataLength + 100);
    DeserializationError  err;

    #ifdef WEBAPIDEBUGX
      Serial.print(F("Got POST Request: "));
      Serial.print(pData);
      Serial.print(F(" length: "));
      Serial.println(nDataLength);
    #endif
    
    err = deserializeJson(doc, pData);

    if(err.code() == DeserializationError::Ok) 
    {
      if(doc.containsKey(F("command")))
      {
        //handle configuration related commands
        handleConfigRequests(resp, pData, nDataLength, doc);
      }
      else if(doc.containsKey(F("chatcmd")))
      {
        //handle configuration related commands
        handleChatRequests(resp, pData, nDataLength, doc);
      }
      else
      {
        sendStringResponse(resp, 503, (char*)(String(F("text/plain"))).c_str(), (char*)(String(F("ERROR - missing command"))).c_str());
      };
    }
    else 
    {
      #ifdef WEBSERVERDEBUG
        Serial.print(F("Webserver failed to deserialize Data: "));
        Serial.print(pData);
        Serial.print(F(" err code: "));
        Serial.println(err.code());
      #endif
  
      if(request != NULL)
      {
        sendStringResponse(resp, 503, (char*)(String(F("text/plain"))).c_str(), "");
      };
    };

    ResetWatchDog();

    doc.clear();
  };



  void handleQueuedMessages()
  {
    //variables
    ///////////
    _sNewMessage        *pNewMsg;

    if(g_pMessages->getItemCount() > 0)
    {
      g_pMessages->itterateStart();
    
      do 
      {
        pNewMsg = (_sNewMessage*)g_pMessages->getNextItem();
    
        if(pNewMsg != NULL)
        {
          switch(pNewMsg->nMsgType)
          {
            case DATATABLE_DATATYPE_SHOUT:
            {
              #ifdef WEBAPIDEBUG
                Serial.println(F("Add Queued ShoutOut:"));
              #endif
  
              limitShoutEntrys(DeviceConfig.nMaxShoutOutEntries);
  
              g_pCLoRaProtocol->addShoutOut(DeviceConfig.dwDeviceID, pNewMsg->dwUserID, pNewMsg->strUser, pNewMsg->strDev, (char*)&pNewMsg->szMsg);
            };
            break;
            
            case DATATABLE_DATATYPE_MSG:
            {
              #ifdef WEBAPIDEBUG
                Serial.println(F("Add Queued message:"));
              #endif
             
              //add outgoing message
              if(g_pCLoRaProtocol->addMessage(pNewMsg->dwUserID, pNewMsg->strUser, pNewMsg->strDev, (char*)&pNewMsg->szMsg, pNewMsg->dwRcptDevID, pNewMsg->dwRcptUserID, pNewMsg->dwRcptContactID, CHAT_DIRECTION_OUTGOING) == true)
              {
                #ifdef WEBAPIDEBUG
                  Serial.println(F("Chk if local"));
                #endif
      
      
                //add web event for user (msg saved OK)
                g_pWebEvent->addEvent(pNewMsg->dwUserID, 2);
                
                //check if local conversation...
                //if local, add the message as incoming
                //to the other user without sending it...
                if(strcasecmp(pNewMsg->strDev.c_str(), DeviceConfig.szDevName) == 0)
                {
                  #ifdef WEBAPIDEBUG
                    Serial.println(F("Add incoming msg"));
                  #endif
                  
                  pNewMsg->dwRcptUserID = GetUserIdByName((char*)pNewMsg->strUser.c_str());
      
                  memset(pNewMsg->szSender, 0, sizeof(pNewMsg->szSender));
                  
                  if(GetUserNameByID(pNewMsg->dwRcptUserID, (char*)&pNewMsg->szSender, sizeof(pNewMsg->szSender)) == true)
                  {
                    if(pNewMsg->dwRcptUserID > 0)
                    {
                      #ifdef WEBAPIDEBUG
                        Serial.println(F("add local incoming msg"));
                      #endif
                      
                      g_pCLoRaProtocol->addMessage(pNewMsg->dwRcptUserID, pNewMsg->szSender, DeviceConfig.szDevName, (char*)&pNewMsg->szMsg, 0, 0, 0, CHAT_DIRECTION_INCOMING);

                      #ifdef WEBAPIDEBUG
                        Serial.println(F("Add web event..."));
                      #endif
                      
                      //add web event for user
                      g_pWebEvent->addEvent(pNewMsg->dwRcptUserID, 1);

                      #ifdef WEBAPIDEBUG
                        Serial.println(F("Msg saved..."));
                      #endif
                    }
                    else
                    {
                      #ifdef WEBAPIDEBUG
                        Serial.println(F("Failed to insert incoming msg"));
                      #endif
                    };
                  }
                  else
                  {
                    #ifdef WEBAPIDEBUG
                      Serial.println(F("Failed to get incoming user"));
                    #endif
                  };
                };
              }
              else
              {
                //add web event for user (msg saved ERR)
                g_pWebEvent->addEvent(pNewMsg->dwUserID, 3);
              };
            };
            break;
  
          };
          
          g_pMessages->removeItem(pNewMsg);
          
          delete pNewMsg;
          
          ResetWatchDog();
        
          pNewMsg = (_sNewMessage*)g_pMessages->getNextItem();
        };
      } while(pNewMsg != NULL);
    };

    ResetWatchDog();
  };

#endif



//https://www.hackster.io/superturis/make-your-own-sd-shield-101fd7






void setup() 
{
  //variables
  ///////////
  long                lTimeout;
  int                 nRes = 0;
  char                szDatabaseFile[33];
  CWSFFileDBRecordset *pRecordset;
  int                 nTemp;

  Serial.begin(115200);

  Serial.print(F("Init LoRa-Link on "));
  Serial.println(LORALINK_HARDWARE_NAME);


  g_pUserButton->attach(USER_BUTTON, INPUT);
  g_pUserButton->setPressedState(LOW); 
  

  g_lReconnectToServerTimeout = 0;
  g_bReconnectToServer        = false;

  #if LORALINK_HARDWARE_BATSENSE == 1
    Serial.println(F("Init ADC (RDiv)"));
    adc_init();
  #endif

  //set status LED, which is always present 
  //even if configurred as int led
  pinMode(LORALINK_HARDWARE_STATUS_LED_PIN, OUTPUT);
  g_ledblink.AddLed(LORALINK_HARDWARE_STATUS_LED_PIN, 100, 900);

  #if LORALINK_HARDWARE_LED == 1
    pinMode(LORALINK_HARDWARE_TX_LED_PIN, OUTPUT);
    
    g_ledblink.AddLed(LORALINK_HARDWARE_TX_LED_PIN, 30000, 1);
    
    g_ledblink.SetBlinking(LORALINK_HARDWARE_TX_LED_PIN, false);
  #endif

  
  //init state
  LLSystemState.bSdOk             = false;
  LLSystemState.nModemState       = 0;
  LLSystemState.bConnected        = false;
  LLSystemState.lBlocksToTransfer = 0;
  LLSystemState.lOutstandingMsgs  = 0;

  
  #if LORALINK_HARDWARE_OLED == 1
    Serial.println(F("Init OLED"));

    Wire.begin(LORALINK_HARDWARE_OLED_SDA, LORALINK_HARDWARE_OLED_SCL);
    
    g_display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, LORALINK_HARDWARE_OLED_RESET);
    
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!g_display.begin(SSD1306_SWITCHCAPVCC, LORALINK_HARDWARE_SCREEN_ADDRESS)) 
    {
      Serial.println(F("SSD1306 failed"));
    };

    g_display.display();
    delay(1000);

    g_display.clearDisplay();
    g_display.setTextSize(1);      // Normal 1:1 pixel scale
    g_display.setTextColor(SSD1306_WHITE); // Draw white text
    g_display.cp437(true);         // Use full 256 char 'Code Page 437' font

    // Clear the buffer
    g_display.print(F("LoRaLink "));
    g_display.print(LORALINK_VERSION_STRING);
    g_display.print(F(" Init:\n"));
    g_display.display();
  #endif


  #ifdef LORALINK_HARDWARE_TBEAM

    //power button stuff
    //Register the PMU interrupt pin, it will be triggered on the falling edge
    pinMode(PMU_IRQ_PIN, INPUT);

    #if LORALINK_HARDWARE_OLED == 0
      Wire.begin();
    #endif
    
    nRes = g_pAxp->begin(Wire, AXP192_SLAVE_ADDRESS);
  
    if(nRes == AXP_FAIL) 
    {
      Serial.println(F("failed to initialize communication with AXP192"));
    }
    else
    {
      /*
       * Set the power of LoRa and GPS module to 3.3V
       **/
      g_pAxp->setLDO2Voltage(3300);   //LoRa VDD
      g_pAxp->setLDO3Voltage(3300);   //GPS  VDD
      g_pAxp->setDCDC1Voltage(3300);  //3.3V Pin next to 21 and 22 is controlled by DCDC1
  
      #if LORALINK_HARDWARE_GPS == 1
        Serial.println(F("Set GPS On..."));
        
        g_pAxp->setPowerOutPut(AXP192_LDO2, AXP202_ON);  // Lora on T-Beam V1.0

        //init sys state
        LLSystemState.bValidSignal = false;
        LLSystemState.fLatitude    = 0.0;
        LLSystemState.fLongitude   = 0.0;
        
        memset(LLSystemState.szGpsTime, 0, sizeof(LLSystemState.szGpsTime));
      #else
        Serial.println(F("Set GPS OFF..."));
      
        g_pAxp->setPowerOutPut(AXP192_LDO2, AXP202_OFF);  // Lora on T-Beam V1.0
      #endif
      
      g_pAxp->setPowerOutPut(AXP192_LDO3, AXP202_ON);  // Gps on T-Beam V1.0
      g_pAxp->setPowerOutPut(AXP192_DCDC2, AXP202_ON); 
      g_pAxp->setPowerOutPut(AXP192_EXTEN, AXP202_ON);
      g_pAxp->setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED on T-Beam v1.0
      g_pAxp->setChgLEDMode(AXP20X_LED_OFF);
      g_pAxp->adc1Enable(AXP202_BATT_CUR_ADC1, 1);

      
      
      attachInterrupt(PMU_IRQ_PIN, [] 
      {
          g_pAxp->readIRQ();
          
          if(g_pAxp->isPEKShortPressIRQ() == true) 
          {
            // Clear all interrupt status
            g_pAxp->clearIRQ();
          };
      }, FALLING);
  
      // Before using IRQ, remember to clear the IRQ status register
      g_pAxp->clearIRQ();
  
      // Turn on the key to press the interrupt function
      g_pAxp->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    };
  #endif

  #if LORALINK_HARDWARE_SDCARD == 1

    Serial.println(F("Init SDCARD"));

    #if LORALINK_HARDWARE_OLED == 1
      g_display.print(F("Init SDCard...\n"));
      g_display.display();
    #endif
    
    g_spiSD.begin(LORALINK_HARDWARE_SDCARD_SCK, LORALINK_HARDWARE_SDCARD_MISO, LORALINK_HARDWARE_SDCARD_MOSI, LORALINK_HARDWARE_SDCARD_CS);
      
    if(SD.begin(LORALINK_HARDWARE_SDCARD_CS, g_spiSD, 4000000, "/sd", LORALINK_HARDWARE_MAX_FILES, true) == false)
    {
      Serial.println(F("Failed to mount SD Card"));

      LLSystemState.bSdOk = false;

      delay(10000);  
      ESP.restart();
    }
    else 
    {
      if(SD.cardType() == CARD_NONE)
      {
        Serial.println(F("ERROR: No SD card inserted"));

        LLSystemState.bSdOk = false;

        delay(10000);  
        ESP.restart();
      };
    
      Serial.print("SD Card Type: ");
      
      if (SD.cardType() == CARD_MMC)
      {
        Serial.println("MMC");
      } 
      else if(SD.cardType() == CARD_SD)
      {
        Serial.println("SDSC");
      } 
      else if(SD.cardType() == CARD_SDHC)
      {
        Serial.println("SDHC");
      } 
      else 
      {
        Serial.println("UNKNOWN");
      };
      
      Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
      Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    
      Serial.println("SUCCESS! (SD CARD)");
      
      LLSystemState.bSdOk = true;
    };
  #endif


  #if LORALINK_HARDWARE_SPIFFS == 1
    
    Serial.println(F("Init SPIFFS"));

    #if LORALINK_HARDWARE_OLED == 1
      g_display.print(F("Init SPIFFS...\n"));
      g_display.display();
    #endif
    
    if(!SPIFFS.begin(false, "/spiffs", LORALINK_HARDWARE_MAX_FILES, NULL))
    {
      Serial.println(F("An Error has occurred while mounting SPIFFS"));

      delay(10000);  
      ESP.restart();
    };
  #endif


  ReadDeviceConfig();

  //check for update
  char *szHotfixFile = LORALINK_FIRMWARE_FILE;
  File updateBin     = LORALINK_FIRMWARE_FS.open(szHotfixFile);
  
  if(updateBin) 
  {
    updateBin.close();

    if(imageWriter(LORALINK_FIRMWARE_FS, szHotfixFile) == true)
    {
      LORALINK_FIRMWARE_FS.remove(szHotfixFile);

      #if LORALINK_HARDWARE_OLED == 1
        g_display.print(F("Update complete...\n"));
        g_display.display();
      #endif

      delay(5000);
      ESP.restart();
    };
  };

  #if LORALINK_HARDWARE_OLED == 1
    g_display.print(F("Init Database...\n"));
    g_display.display();
  #endif

  if(initDatabase(LORALINK_DATA_FS) == false)
  {
    Serial.println(F("Unable to init database"));
    
    delay(5000);
    ESP.restart();
  };

  //init SKYNET protocoll routing
  initRouting();


  //check if RTC clock exist
  if(CheckIfDeviceExist(104) == true) 
  {
    Serial.println(F("Found DS3231 - Init RTC clock"));

    ClockPtr = new CClockHlpr(true, &Wire);

    //update schedules after reboot
    g_pDbTaskScheduler->rescheduleAfterTimechange();
    g_pDbTaskScheduler->haltScheduler(false);
  }
  else
  {
    Serial.println(F("DS3231 not present - Init int. clock"));

    ClockPtr = new CClockHlpr(false, &Wire);
  };

  
  #if LORALINK_HARDWARE_WIFI == 1
    //variables
    ///////////
    IPAddress IP;

    Serial.println(F("Init WIFI"));

    #if LORALINK_HARDWARE_OLED == 1
      g_display.print(F("Init WIFI...\n"));
      g_display.display();
    #endif

    EnableWiFi();
    

    if(strlen(LoRaWiFiCfg.szWLANSSID) > 0)
    {
      //check for dyndns config
      if(strlen(DynDNSConfig.szProvider) > 0)
      {
        EasyDDNS.service(DynDNSConfig.szProvider);
  
        if(DynDNSConfig.bAuthWithUser == false)
        {
          EasyDDNS.client(DynDNSConfig.szDomain, DynDNSConfig.szUser); // Enter your DDNS Domain & Token
        }
        else
        {
          EasyDDNS.client(DynDNSConfig.szDomain, DynDNSConfig.szUser, DynDNSConfig.szPassword);
        };
  
        // Get Notified when your IP changes
        EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP) {
          Serial.print("EasyDDNS - IP Change Detected: ");
          Serial.println(newIP);
        });
  
        ResetWatchDog();
      };


      if(IpLinkConfig.bServerEnabled == true)
      {
        g_pTCPServer = new CTCPServer(IpLinkConfig.wServerPort, onIpClientConnect);
        g_pTCPServer->setInterval(0);
  
        g_pTaskHandler->addTask(g_pTCPServer);
      };
    };
    
    //setup dns responder
    if(IP.fromString(LoRaWiFiApCfg.szDevIP)) 
    {
      g_dnsServer.start(53, "*", IP);
    };

    Serial.println(F("Start webserver..."));

    //init web event handler
    g_pWebEvent = new CWebEvent();

    //Setup internal webserver
    StartWebservers(ApiCallbackHandler);

    //create a task to handle the web requests
    xTaskCreatePinnedToCore(WebServerTask, "WebServerTask", LORALINK_STACKSIZE_WEBSERVER, NULL, 1, &g_Core1TaskHandle2, 1);
  #endif


  

  
  //enable scheduler (important, db is not open yet!)
  g_pDbTaskScheduler->haltScheduler(false);
  //update schedules after reboot
  g_pDbTaskScheduler->rescheduleAfterTimechange();



  #if LORALINK_HARDWARE_LORA == 1
    //start Modem
    g_pModemTask = new CLoRaModem(OnReceivedModemData, OnModemStateChanged);
    CSkyNetConnectionHandler *pModemConnHandler = new CSkyNetConnectionHandler(g_pModemTask, SKYNET_CONN_TYPE_LORA, g_pSkyNetConnection);
    
    g_pModemTask->setHandler(pModemConnHandler);
  
  
    //init modem:
    Serial.print(F("Init LoRa Modem : "));
    Serial.print(ModemConfig.lBandwidth);
    Serial.print(F("khz @"));
    Serial.print(ModemConfig.lFrequency / 1000);
    Serial.print(F("kHz, SF "));
    Serial.print(ModemConfig.nSpreadingFactor);
    Serial.print(F(" CR "));
    Serial.println(ModemConfig.nCodingRate);
  
    //create modem task, the task is exclusively bound to core 0,
    //it does not utilise a handler...
    if(g_pModemTask->Init(ModemConfig.lFrequency, ModemConfig.nTxPower, ModemConfig.lBandwidth, ModemConfig.nSpreadingFactor, ModemConfig.nCodingRate, ModemConfig.nSyncWord, ModemConfig.nPreamble) == true)
    {
      //add the connection handler to a task handler
      //since the taskhandler only handles the states, set the interval to 1sec
      //the modem data has a seperate thread...
      pModemConnHandler->setInterval(1000);
      
      g_pTaskHandler->addTask(pModemConnHandler);
    
      //add to skynet connection
      g_pSkyNetConnection->addHandler(pModemConnHandler);
    
      //check if modem was initialized
      if(ModemConfig.bDisableLoRaModem == false)
      {
        if(g_pModemTask->GetModemState() == MODEM_STATE_ERROR)
        {
          Serial.print(F("Init Modem: FAILED"));
          delay(5000);
      
          ESP.restart();
        };
      };
    }
    else
    {
      if(ModemConfig.bDisableLoRaModem == false)
      {
        Serial.print(F("Init Modem: FAILED"));
        delay(5000);
    
        ESP.restart();
      };
    };

    //this task is responsible to receive data and queue them internally
    xTaskCreatePinnedToCore(ModemTask, "ModemTask", LORALINK_STACKSIZE_MODEM, NULL, 1, &g_Core0TaskHandle, 0);

  #endif

  //a task responsible for let the leds blink
  xTaskCreatePinnedToCore(BlinkTask, "BlinkTask", LORALINK_STACKSIZE_BLINK, NULL, 1, &g_Core0TaskHandle1, 1);

  //if an OLED display is attached, create a task, to show a nice info screen
  #if LORALINK_HARDWARE_OLED == 1
    xTaskCreatePinnedToCore(DisplayTask, "DisplayTask", LORALINK_STACKSIZE_DISPLAY, NULL, 1, &g_Core1TaskHandle, 1);
  #endif

  //this task dequeues the received modem data (IP or LORA) and processes it...
  xTaskCreatePinnedToCore(ModemDataTask, "ModemDataTask", LORALINK_STACKSIZE_MODEM_DATA, NULL, 1, &g_Core0TaskHandle2, 0);


  //add the SkyNet protocol handler to the task handler
  //this handler handles the protocol state for all connections
  //added to the handler, this task will also send the outgoing
  //messages (he will block the taskhandler until all msgs are sent!)...
  g_pSkyNetConnection->setInterval(1000);
  g_pTaskHandler->addTask(g_pSkyNetConnection);


  //add the db task scheduler to the task 
  //handler
  g_pTaskHandler->addTask(g_pDbTaskScheduler);


  //create filetransfer/resume task if not existing...
  //this is the case after firmware update or clean install
  if(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_RSTFILETRANSFER) == 0)
  {
    sDBTaskRestartFileTransfer sTaskData;

    sTaskData.dwLastFile    = 0;
    sTaskData.dwLastRequest = 0;
    
    g_pDbTaskScheduler->addSchedule(DBTASK_RSTFILETRANSFER, 600, 0, (byte*)&sTaskData, true);
  };


  //create a task checking if the webserver still responds
  //this can happen due to errors insdide the webserver or ip stack
  //mostly happens when the device runs out of memory.
  //when this task failes 3 times, it will reboot the device
  #if LORALINK_HARDWARE_WIFI == 1
  
    if(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_CHECKWEBSERVER) == 0)
    {
      g_pDbTaskScheduler->addSchedule(DBTASK_CHECKWEBSERVER, 600, 0, NULL, true);
    };

    //when the device is a repeater, wifi will be disabled after 5 
    //minutes after reboot to conserve battery...
    g_lWiFiShutdownTimer = millis() + (5 * 60 * 1000);
  #endif

  


  #if LORALINK_HARDWARE_GPS == 1
    //init GPS
    Serial1.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); 

    g_pGPS = new TinyGPS();

    LLSystemState.bValidSignal  = false;   
    LLSystemState.fLatitude     = 0;
    LLSystemState.fLongitude    = 0;
    LLSystemState.fAltitude     = 0;
    LLSystemState.dwLastValid   = 0;

    xTaskCreatePinnedToCore(GpsDataTask, "GpsDataTask", LORALINK_STACKSIZE_GPS_DATA, NULL, 1, &g_Core0TaskHandle3, 0);
    

    //check if there is a running tracking
    if(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_SEND_POSITION) != 0)
    {
      //variables
      ///////////
      sDBTaskQueryUser task;  
      byte             bData[40];

      memset(bData, 0, sizeof(bData));
      
      g_pDbTaskScheduler->getTaskData(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_SEND_POSITION), (byte*)&bData, sizeof(bData));

      memcpy(&task, bData, sizeof(sDBTaskQueryUser));
 
      g_bLocationTrackingActive = true;
      g_nLocationTrackingType   = task.dwContactID & 0xFF; //used as type

      Serial.println(F("Location tracking active!"));
    };

    //check if there is a running track recording
    //this is an enabler only, when the device crashes or looses power, 
    //so it has no handler...
    if(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_RECORD_TRACK) != 0)
    {
      //variables
      ///////////
      sDBTaskSendFile  task;  
      byte             bData[40];

      memset(bData, 0, sizeof(bData));

      g_pDbTaskScheduler->getTaskData(g_pDbTaskScheduler->findTaskByScheduleType(DBTASK_RECORD_TRACK), (byte*)&bData, sizeof(bData));

      memcpy(&task, bData, sizeof(sDBTaskSendFile));
      
      g_bRecordTrack            = true;
      g_dwRecordTrackID         = task.dwFileID;
      
      Serial.println(F("Track recording active!"));
    };
    
  #endif


  //count the blocks which are to transmit      
  if(g_pDataTable->isOpen() == true)
  {
    pRecordset = new CWSFFileDBRecordset(g_pDataTable);  

    //record exist?
    while(pRecordset->haveValidEntry() == true)
    {
      pRecordset->getData(2, (void*)&nTemp, sizeof(nTemp));

      if(nTemp == 0)
      {
        LLSystemState.lBlocksToTransfer += 1;
      };

      pRecordset->moveNext();
    };

    delete pRecordset;
  };
};




void loop() 
{
  //variables
  ///////////
  long            lCheckTimer = 0;

  esp_task_wdt_init(180, false);

  while(true)
  {
    g_pUserButton->update();

    if(g_pUserButton->pressed() == true)
    {
      #if LORALINK_HARDWARE_WIFI == 1
        if(LoRaWiFiApCfg.bWiFiEnabled == true)
        {
          LoRaWiFiApCfg.bWiFiEnabled = false;
          WiFi.mode(WIFI_OFF);
        }
        else
        {
          EnableWiFi();
        };
      #endif
    };

    #if LORALINK_HARDWARE_WIFI == 1
      //turn wifi off, 5min after startup, when the 
      //device is a repeater without login
      if((DeviceConfig.nDeviceType == DEVICE_TYPE_REPEATER) && (millis() > g_lWiFiShutdownTimer) && (g_lWiFiShutdownTimer != 0))
      {
        g_lWiFiShutdownTimer        = 0;
        LoRaWiFiApCfg.bWiFiEnabled  = false;
        
        WiFi.mode(WIFI_OFF);
      };
    #endif
    
    LLSystemState.lMemFreeOverall           = esp_get_free_heap_size();

    //halt the scheduler, if the system is not connected to
    //another node...
    g_pDbTaskScheduler->haltScheduler(!LLSystemState.bConnected);


    //check free space
    if(lCheckTimer < millis())
    {
      lCheckTimer                           = millis() + 10000;
      
      LLSystemState.bConnected              = g_pSkyNetConnection->isConnected();
      
      #if LORALINK_HARDWARE_SPIFFS == 1
        LLSystemState.lIntSpiBytesUsed      = SPIFFS.usedBytes();
        LLSystemState.lIntSpiBytesAvail     = SPIFFS.totalBytes();
      #else
        LLSystemState.lIntSpiBytesUsed      = 0;
        LLSystemState.lIntSpiBytesAvail     = 0;
      #endif
  
      #if LORALINK_HARDWARE_SDCARD == 1
        LLSystemState.lExtSpiBytesUsed      = SD.usedBytes();
        LLSystemState.lExtSpiBytesAvail     = SD.totalBytes();
      #else
        LLSystemState.lExtSpiBytesUsed      = 0;
        LLSystemState.lExtSpiBytesAvail     = 0;
      #endif
    };

  
    //call the task scheduler / handler
    g_pTaskHandler->handleTasks();
    ResetWatchDog();
    
    delay(10);

    #if LORALINK_HARDWARE_WIFI == 1
      //handle the messages received by the webserver
      handleQueuedMessages();

      HandleLoRaLinkReconnect();
    #endif
  };
};





/**
 * this callback handles the received and queued modem data
 */
void OnReceivedModemData(byte *pData, int nDataLength, int nRSSI, void *pHandler, float fPacketSNR)
{
  //variables
  ///////////
  CSkyNetConnectionHandler *pModemConnHandler = (CSkyNetConnectionHandler*)pHandler;
  _sModemData              *pModemData        = new _sModemData;

  Serial.print(F("--> OnReceivedModemData: bytes: "));
  Serial.println(nDataLength);

  //only store the data, don't handle it here,
  //otherwise the modem looses packets...
  pModemData->nDataLength   = nDataLength;
  pModemData->pData         = new byte[nDataLength + 1];
  memcpy(pModemData->pData, pData, nDataLength);
  pModemData->nRSSI         = nRSSI;
  pModemData->pHandler      = pModemConnHandler;
  pModemData->fPacketSNR    = fPacketSNR;

  
  
  g_pModemMessages->addItem(pModemData);
  
  
  ResetWatchDog();

  Serial.print(F("<-- OnReceivedModemData: bytes: "));
  Serial.println(nDataLength);
};



/**
 * this callback handles the modem states 
 */
void OnModemStateChanged(int nState)
{
  LLSystemState.nModemState = nState;  

  #if LORALINK_HARDWARE_LED == 1
  
    switch(nState)
    {
      case MODEM_STATE_TX:
      {
        g_ledblink.SetBlinking(LORALINK_HARDWARE_TX_LED_PIN, true);
      };
      break;

      default:
      {
        g_ledblink.SetBlinking(LORALINK_HARDWARE_TX_LED_PIN, false);
      };
      break;
    };

  #endif
};




/**
 * this task handles the received and queued modem data. processing can take some time,
 * so I had to seperate it, otherwise it would break reception...
 */
void ModemDataTask(void *pParam)
{
  //variables
  ///////////
  _sModemData              *pModemData;

  esp_task_wdt_init(6000, false);
  
  while(true)
  {
    LLSystemState.lMemFreeModemDataTask = uxTaskGetStackHighWaterMark(NULL);

    if(g_pModemMessages->getItemCount() > 0)
    { 
      g_pModemMessages->itterateStart();
    
      do 
      {
        pModemData = (_sModemData*)g_pModemMessages->getNextItem();
    
        if(pModemData != NULL)
        {
          Serial.print(F("[ModemDataTask] --> Handle queued data - size:"));
          Serial.println(pModemData->nDataLength);
          
          pModemData->pHandler->handleConnData(pModemData->pData, pModemData->nDataLength, pModemData->nRSSI, pModemData->fPacketSNR);
  
          g_pModemMessages->removeItem(pModemData);
  
          delete pModemData->pData;
          delete pModemData;
  
          Serial.println(F("[ModemDataTask] <-- Handle queued data"));
  
          ResetWatchDog();
  
          LLSystemState.lMemFreeModemDataTask = uxTaskGetStackHighWaterMark(NULL);
        };

        ResetWatchDog();

        LLSystemState.lMemFreeModemDataTask = uxTaskGetStackHighWaterMark(NULL);
        
      } while(pModemData != NULL);
    };
    
    ResetWatchDog();
    
    delay(100);
  };
};


#if LORALINK_HARDWARE_GPS == 1
  
  void GpsDataTask(void *pParam)
  {
    //variables
    ///////////
    int  year;
    byte month, day, hour, minute, second, hundredths;
    char c = 0;
    unsigned long age;
    long lDiff;
    unsigned long chars = 0;
    float fLat = 0, fLon = 0, fAlt = 0, fSpeed = 0, fCourse = 0;
    int nSat, nHDOP;
    DateTime dtClock;
    char          szDatabaseFile[50];
    CWSFFileDB    *pDatabase;
    void          *pInsert[TRACKWAYPOINTTABLE_SIZE + 1];
    uint32_t      dwTime;
    
    memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
    sprintf((char*)&szDatabaseFile, TRACKWAYPOINTTABLE_FILE);

    
    while(true)
    {
      if(g_pGPS != NULL)
      {
        while(Serial1.available() > 0)
        {
          chars += 1;
          c      = Serial1.read();
          
          #ifdef SERIAL1DEBUG
            Serial.print(c);
          #endif
          
          g_pGPS->encode(c);
        };

        if(chars > 0)
        {
          #ifdef GPSSTATSDEBUG
            Serial.print(F("[GPS] Chars decoded: "));
            Serial.println(chars);           
          #endif

          memset(LLSystemState.szGpsTime, 0, sizeof(LLSystemState.szGpsTime));
      
          g_pGPS->crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    
          if(age != TinyGPS::GPS_INVALID_AGE)
          {
            LLSystemState.bValidSignal = true;
            
            sprintf_P(LLSystemState.szGpsTime, PSTR("%02d:%02d:%02d"), hour, minute, second);
            
            
            dtClock  = DateTime(year, month, day, hour, minute, second);

            fAlt     = g_pGPS->f_altitude();
            nSat     = g_pGPS->satellites();
            nHDOP    = g_pGPS->hdop();
            fSpeed   = g_pGPS->f_speed_kmph();
            fCourse  = g_pGPS->f_course();

            g_pGPS->f_get_position(&fLat, &fLon, &age);
            
            if((nSat == TinyGPS::GPS_INVALID_SATELLITES) || (nSat < 3))
            {
              LLSystemState.bValidSignal = false;
            };

            if(nHDOP == TinyGPS::GPS_INVALID_HDOP)
            {
              LLSystemState.bValidSignal = false;
            };
            
            if(fAlt == TinyGPS::GPS_INVALID_ALTITUDE)
            {
              LLSystemState.bValidSignal = false;
            };
          };

          
          if(LLSystemState.bValidSignal == false)
          {
            memset(LLSystemState.szGpsTime, 0, sizeof(LLSystemState.szGpsTime));
            strcpy_P(LLSystemState.szGpsTime, PSTR("no fix"));

            //to show values on the display 
            //aquiring a lock can take a huge amount 
            //of time, better to show some values (instead of 0)
            LLSystemState.nNumSat     = nSat;
            LLSystemState.fLatitude   = fLat;
            LLSystemState.fLongitude  = fLon;
          }
          else
          {
            LLSystemState.fAltitude   = fAlt;
            LLSystemState.nNumSat     = nSat;
            LLSystemState.fLatitude   = fLat;
            LLSystemState.fLongitude  = fLon;
            LLSystemState.dwLastValid = millis();
            LLSystemState.fSpeed      = fSpeed;
            LLSystemState.nHDOP       = nHDOP;
            LLSystemState.fCourse     = fCourse;

            lDiff = dtClock.unixtime() - ClockPtr->getUnixTimestamp();

            if(lDiff < 0) 
            {
              lDiff *= -1;
            };

            #ifdef GPSSTATSDEBUG
              Serial.print(F("[GPS] difference to loc clock (sec): "));
              Serial.print(lDiff);
              Serial.print(F(" Lat: "));
              Serial.print(fLat);
              Serial.print(F(" Lon: "));
              Serial.println(fLon);
            #endif
            
            //update clock if not already done, or time is from a remote or unknown source
            //of if the difference to GPS is > 10min
            if(((lDiff / 60) > 10) || (ClockPtr->timeSet() == false) || ((ClockPtr->getUpdateSource() != UPDATE_SOURCE_NTP) && (ClockPtr->getUpdateSource() != UPDATE_SOURCE_EXTERNAL)))
            {
              ClockPtr->SetDateTime(year, month, day, hour, minute, UPDATE_SOURCE_EXTERNAL);
  
              //update schedules after reboot or initial time change
              g_pDbTaskScheduler->rescheduleAfterTimechange();
            };


            //if gps transmission is enabled (set a dev location not 0), and we have a valid signal,
            //use the coordinates from the gps receiver
            if((DeviceConfig.fLocN != 0) && (DeviceConfig.fLocE != 0)) 
            {
              DeviceConfig.fLocN = LLSystemState.fLatitude;
              DeviceConfig.fLocE = LLSystemState.fLongitude;  
            };


            //if track recording is active, write data to table
            if(g_bRecordTrack == true)
            {
              if(g_lTrackRecTimer <= millis())
              {
                //write waypoint every 3min
                g_lTrackRecTimer = millis() + (1000 * 60 * 3);

                pDatabase            = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nTrackWaypointTableDef, TRACKWAYPOINTTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);
                pDatabase->open();
        
                if(pDatabase->isOpen() == true)
                {
                  dwTime     = ClockPtr->getUnixTimestamp();
                  
                  pInsert[0] = (void*)&g_dwRecordTrackID;
                  pInsert[1] = (void*)&LLSystemState.fLatitude;
                  pInsert[2] = (void*)&LLSystemState.fLongitude;
                  pInsert[3] = (void*)&LLSystemState.fAltitude;
                  pInsert[4] = (void*)&dwTime;
    
                  pDatabase->insertData(pInsert);
                };

                delete pDatabase;
              };
            };
          };
        }
        else
        {
          //no data from RX
          //no rx connected or error
          memset(LLSystemState.szGpsTime, 0, sizeof(LLSystemState.szGpsTime));
          strcpy_P(LLSystemState.szGpsTime, PSTR("GPS ERR"));
        };
      };

      ResetWatchDog();
  
      delay(200);
    };
  };

#endif
  

#if LORALINK_HARDWARE_OLED == 1

  void DisplayTask(void *pParam)
  {
    //variables
    ///////////
    int w, h;
    char szText[100];
    int  nIpConns = 0;
    long lTimer = 0;
    int  nInfoCard                = 0;
    int  nMaxCard                 = 4;
    long lInfoCardSwitchTimer     = millis() + INFO_CARD_SWITCH_INTERVAL;
    float fSystemVoltage          = 0.0;

    #if LORALINK_HARDWARE_GPS == 1
      nMaxCard += 1;
    #endif

    esp_task_wdt_init(180, false);
  
    g_display.clearDisplay();
    g_display.setTextSize(1);      // Normal 1:1 pixel scale
    g_display.setTextColor(SSD1306_WHITE); // Draw white text
    g_display.cp437(true);         // Use full 256 char 'Code Page 437' font
    
    while(true)
    {
      LLSystemState.lMemFreeDisplayTask = uxTaskGetStackHighWaterMark(NULL);

      //does the cards need to be switched?
      if(millis() > lInfoCardSwitchTimer)
      {
        lInfoCardSwitchTimer   = millis() + INFO_CARD_SWITCH_INTERVAL;
        nInfoCard             += 1;
        //mem page: nInfoCard             = 1;
        
        if(nInfoCard >= nMaxCard)
        {
          nInfoCard = 0;
        };
      };

      //does the card need to be updated?
      if(lTimer < millis())
      {
        //2 updates / sec
        lTimer = millis() + 500;
        
        g_display.clearDisplay();
        nIpConns = g_pSkyNetConnection->countConnHandlerByType(SKYNET_CONN_TYPE_IP_CLIENT) + g_pSkyNetConnection->countConnHandlerByType(SKYNET_CONN_TYPE_IP_SERVER);
    
        w = 110; h = 0;
        g_display.setCursor(w, h);     // Start at top-left corner
        
        switch(g_pModemTask->GetModemState())
        {
          case MODEM_STATE_TX: { g_display.print("TX"); }; break;
          case MODEM_STATE_RX: { g_display.print("RX"); }; break;
          case MODEM_STATE_WAIT_RX: { g_display.print("WR"); }; break;
          case MODEM_STATE_ERROR: { g_display.print("ER"); }; break;
          case MODEM_STATE_IDLE: { g_display.print("--"); }; break;
          case MODEM_STATE_WAIT_TX: { g_display.print("WT"); }; break;
        };
    
    
        w = 1; h = 0;
        g_display.setCursor(w, h);     // Start at top-left corner
  
        if(ModemConfig.bDisableLoRaModem == false)
        {
          sprintf_P(szText, PSTR("%.3fkHz IP: %i\0"), (float)ModemConfig.lFrequency / 1000000.0, nIpConns); 
        }
        else
        {
          strcpy_P(szText, PSTR("Radio Off\0"));
        };
        
        g_display.print(szText);
    
        w = 0; h = 9;
        g_display.drawLine(w, h, 128, h, SSD1306_WHITE);
        //end of top info
  
        switch(nInfoCard)
        {
          //display DevName & Node ID
          case 0:
          {
            //center screen height
            h = ((64 - (9 + 12)) / 2);
            
      
            sprintf_P(szText, PSTR("%u\0"), DeviceConfig.dwDeviceID);
            
            //center screen width according to text size
            w = (128 - (strlen(szText) * 5)) / 2;
            
            g_display.setCursor(w, h);
            g_display.print(szText);
      
      
            sprintf_P(szText, PSTR("%s\0"), DeviceConfig.szDevName);
            
            w = (128 - (strlen(szText) * 5)) / 2;
            h += 10;
            g_display.setCursor(w, h);
            g_display.print(szText);
          };
          break;
  
          //memory info
          case 1:
          {  
            sprintf_P(szText, PSTR("Free Mem:  %u\nFree Mdm:  %u\nFree Dsp:  %u\nFree Web:  %u\nFree Data: %u\0"), LLSystemState.lMemFreeOverall, LLSystemState.lMemFreeModemTask, LLSystemState.lMemFreeDisplayTask, LLSystemState.lMemFreeWebServerTask, LLSystemState.lMemFreeModemDataTask);
  
            w = 0; h = 12;
            g_display.setCursor(w, h);
            
            g_display.print(szText);
          };
          break;
  
          //misc
          case 2:
          {
            #ifdef LORALINK_HARDWARE_TBEAM
              fSystemVoltage = g_pAxp->getBattVoltage() / 1000.0;
            #else
              #if LORALINK_HARDWARE_BATSENSE == 1
                fSystemVoltage = battery_read();
              #else
                fSystemVoltage = 0.0;
              #endif
            #endif
            
            sprintf_P(szText, PSTR("Battery: %.2fv\nNetwork: %s\nFlash: %llukB\nSD: %lluMB\nTran. Blk: %i\0"), fSystemVoltage, (LLSystemState.bConnected == true ? "Connected" : "Unavailable"), (LLSystemState.lIntSpiBytesAvail - LLSystemState.lIntSpiBytesUsed) / 1024, (LLSystemState.lExtSpiBytesAvail - LLSystemState.lExtSpiBytesUsed) / (1024 * 1024), LLSystemState.lBlocksToTransfer);
       
            w = 0; h = 13;
            g_display.setCursor(w, h);
            
            g_display.print(szText);
          };
          break;
  
  
          //LoRa Msgs
          case 3:
          {
            if(LoRaWiFiApCfg.bWiFiEnabled == true)
            {
              sprintf_P(szText, PSTR("Proto MSGS: %i\nAP IP: %s\nIP: %s\0"), LLSystemState.lOutstandingMsgs, LoRaWiFiApCfg.szDevIP, LoRaWiFiCfg.szDevIP);
            }
            else
            {
              sprintf_P(szText, PSTR("Proto MSGS: %i\nWiFi: OFF\0"), LLSystemState.lOutstandingMsgs);
            };
       
            w = 0; h = 13;
            g_display.setCursor(w, h);
            
            g_display.print(szText);
          };
          break;

          #if LORALINK_HARDWARE_GPS == 1
          
            case 4:
            {
              sprintf_P(szText, PSTR("GPS Time: %s\nLat: %f\nLon: %f\nSat: %i\0"), LLSystemState.szGpsTime, LLSystemState.fLatitude, LLSystemState.fLongitude, LLSystemState.nNumSat);
       
              w = 0; h = 13;
              g_display.setCursor(w, h);
              
              g_display.print(szText);
            };
            break;

          #endif
        };
  
        //bottom info
        w = 0; h = 52;
        g_display.drawLine(w, h, 128, h, SSD1306_WHITE);
        
        w = 6; h = 54;
        g_display.setCursor(w, h);     // Start at top-left corner
        g_display.print(ClockPtr->GetTimeString());
        
        g_display.display();
      };
  
      ResetWatchDog();
  
      delay(100);
    };
  };
#endif


/*
 * this task handles the LoRa modem. Since the used lib is blocking,
 * i had to seperate it from the TaskHandler into an own thread.
 * The library waits for a specified amount of time to receive 
 * a packet from the modem. When the packet is received, it calls
 * the rx Callback, which need to store it in a queue, to directly 
 * start receiving again (if not, you loose packets).
 */
void ModemTask(void *pParam)
{
  esp_task_wdt_init((long)ModemConfig.nTransmissionReceiveTimeout * 10, false);
   
  while(true)
  {
    LLSystemState.lMemFreeModemTask = uxTaskGetStackHighWaterMark(NULL);
    
    if(ModemConfig.bDisableLoRaModem == false)
    {
      g_pModemTask->handleTask();
    }
    else
    {
      delay(100);
    };
    
    ResetWatchDog();
  };
};


/**
 * simple task to handle blinking of multiple leds 
 */
void BlinkTask(void *pParam)
{
  //variables
  ///////////
  long lCheckTimer = 0;
  
  while(true)
  {
    //check connected state every 1sec
    if(lCheckTimer < millis())
    {
      lCheckTimer                         = millis() + 1000;
      
      g_ledblink.SetBlinking(LORALINK_HARDWARE_STATUS_LED_PIN, LLSystemState.bConnected);
    };
    
    ResetWatchDog();

    g_ledblink.DoBlink();

    delay(100);
  };
};



void OnSchedule(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData)
{
  Serial.print(F("[OnSched] ID: "));
  Serial.print(dwScheduleID);
  Serial.print(F(" type: "));
  Serial.print(nScheduleType);
  Serial.print(F(" exec#: "));
  Serial.print(nNumberOfExec);
  Serial.print(F(" max try: "));
  Serial.println(nMaxTries);
  
  switch(nScheduleType)
  {
    #if LORALINK_HARDWARE_GPS == 1
      /*
       * this task will be used to send the device position
       */
      case DBTASK_SEND_POSITION:
      {
        //variables
        ///////////     
        sDBTaskQueryUser  task;  

        memcpy(&task, pData, sizeof(sDBTaskQueryUser));

        //send only when a location is known
        //when never any position was received, 
        //it makes no sense to send any data...
        if(LLSystemState.dwLastValid > 0)
        { 
          Serial.print(F("[OnSched] Send Position to Device: "));
          Serial.print(task.dwDeviceID);
          Serial.print(F(" User: "));
          Serial.println(task.dwUserToInform);
  
          g_pCLoRaProtocol->addPosition(task.dwDeviceID, task.dwUserToInform, task.dwContactID & 0xFF);
        };
      };
      break;
      
    #endif
    
    case DBTASK_QUERYUSER:
    {
      //variables
      ///////////
      sDBTaskQueryUser    task;
      CWSFFileDBRecordset *rs;
      CWSFFileDB          *pDatabase;
      uint32_t            dwDeviceID;
      uint32_t            dwUserID;
      char                szDatabaseFile[50];
      int                 nContactState;
      char                szUserName[30];

      
      memcpy(&task, pData, sizeof(task));

      memset(szUserName, 0, sizeof(szUserName));
      
      memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
      sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, task.dwUserToInform);

      pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);  
      pDatabase->open();
      
      if(pDatabase->isOpen() == true)
      {
        rs = new CWSFFileDBRecordset(pDatabase, task.dwContactID);

        if(rs->haveValidEntry() == true)
        {
          rs->getData(0, (byte*)&dwUserID, sizeof(dwUserID));
          rs->getData(1, (byte*)&dwDeviceID, sizeof(dwDeviceID));
          rs->getData(2, (byte*)&szUserName, sizeof(szUserName));
          rs->getData(4, (byte*)&nContactState, sizeof(nContactState));

          Serial.print(F("[OnSched] UserQuery task state: "));
          Serial.print(nContactState);
          Serial.print(F(" for contact: "));
          Serial.print(task.dwContactID);
          Serial.print(F(" local user: "));
          Serial.print(task.dwUserToInform);
          Serial.print(F(" remote user: "));
          Serial.print(szUserName);
          Serial.print(F("@"));
          Serial.println(dwDeviceID);
          
          if(((dwUserID > 0) && (dwDeviceID > 0)) || (nContactState != 0))
          {
            switch(nContactState)
            {
              case 0: 
              {
                //no result yet
              };
              break;

              case 1:
              {
                //not existing
                g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
              };
              break;

              case 2:
              {
                //ok
                g_pDbTaskScheduler->removeSchedule(dwScheduleID, true);
              };
              break;

              case 3:
              {
                //dev not existing
                g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
              };
              break;
            };
          }
          else 
          {
            if((dwDeviceID > 0) && (dwUserID <= 0))
            {
              Serial.print(F("[OnSched] UserQuery timed out: start query for: "));
              Serial.println(szUserName);
              
              g_pCLoRaProtocol->enqueueUserQuery(szUserName, dwDeviceID, task.dwUserToInform, task.dwContactID, dwScheduleID);
            }
            else 
            {
              Serial.println(F("[OnSched] UserQuery failed, dev unknown!"));

              g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
            };
          };
        }
        else
        {
          g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);

          Serial.println(F("[OnSched] Failed to get data - removed contact?"));
        };
      };

      delete rs;
      delete pDatabase; 
    };
    break;
    
    case DBTASK_QUERYNODE:
    {
      //variables
      ///////////
      sDBTaskQueryNode    task;
      CWSFFileDBRecordset rsNode(g_pNodeTable);
      CWSFFileDB          *pDatabase;
      CWSFFileDBRecordset *pRS;
      uint32_t            dwDeviceID;
      char                szDatabaseFile[50];
      char                szDevName[30];

      memcpy(&task, pData, sizeof(task));

      memset(szDevName, 0, sizeof(szDevName));
      memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
      
      sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, task.dwUserToInform);

      pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, false, DBRESERVE_CONTACT_COUNT);  
      pDatabase->open();
      
      if(pDatabase->isOpen() == true)
      {
        pRS = new CWSFFileDBRecordset(pDatabase, task.dwContactID);

        if(pRS->haveValidEntry() == true)
        {
          //get node name
          pRS->getData(3, (byte*)&szDevName, sizeof(szDevName));
        
          //check if already done and the node is known
          if(FindDeviceByNodeName(&rsNode, szDevName) == true)
          {
            //inform user by updating his contacts...
            //if there are multiple contacts missing the 
            //device ID, they maybe have a schedule running, 
            //so no need to update everything here...
            if(task.dwUserToInform > 0)
            {
              rsNode.getData(0, (byte*)&dwDeviceID, sizeof(dwDeviceID));
      
              pRS->setData(1, (byte*)&dwDeviceID, sizeof(dwDeviceID));

              //set device ID in filetransfer table
              if(g_pDataHeaderTable->isOpen() == true)
              {
                delete pRS;
                
                pRS = new CWSFFileDBRecordset(g_pDataHeaderTable, task.dwFileID);  
        
                //record exist?
                if(pRS->haveValidEntry() == true)
                {
                  pRS->setData(5, (void*)&dwDeviceID, sizeof(dwDeviceID));
                   
                  //remove task
                  g_pDbTaskScheduler->removeSchedule(dwScheduleID, true);
                }
                else
                {
                  //remove task
                  g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
                };
              }
              else
              {
                //remove task
                g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
              };
            }
            else
            {
              //remove task
              g_pDbTaskScheduler->removeSchedule(dwScheduleID, true);
            };
          }
          else
          {
            //send query request to all connections
            g_pCLoRaProtocol->enqueueQueryRequest(task.dwNodeToQuery, (char*)&szDevName, (nNumberOfExec <= 1 ? false : true), dwScheduleID);
          };
        }
        else 
        {
          //remove task
          g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
        };

        delete pRS;
      }
      else
      {
        //remove task
        g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
      };

      delete pDatabase;
    };
    break;


    case DBTASK_SENDMESSAGE:
    {
      //variables
      ///////////
      sDBTaskSendFile     task;
      CWSFFileDBRecordset *rs;
      bool                bExist = false;
      char                szDatabaseFile[50];
      int                 nHeaderTransferConfirmed = 0;
      uint32_t            dwReceiverDeviceID;
      uint32_t            dwLocalUserID = 0;
      char                szSender[33];
      char                szFilename[50];
      int                 nDataType;
      uint32_t            dwSize;
      int                 nBlockSize;
      int                 nTransferEnabled;
      
      memcpy(&task, pData, sizeof(task));

      Serial.print(F("[OnSched] SENDMESSAGE File ID: "));
      Serial.println(task.dwFileID);

      memset(szSender, 0, sizeof(szSender));
      memset(szFilename, 0, sizeof(szFilename));

      
      if(g_pDataHeaderTable->isOpen() == true)
      {
        rs = new CWSFFileDBRecordset(g_pDataHeaderTable, task.dwFileID);

        if(rs->haveValidEntry() == true)
        {
          bExist = true;

          rs->getData(10, (void*)&nTransferEnabled, sizeof(nTransferEnabled));
          rs->getData(12, (void*)&nHeaderTransferConfirmed, sizeof(nHeaderTransferConfirmed));

          //check if header was confirmed - when true, the other dev request the data block by block
          //if not, send the header to the device
          if((nHeaderTransferConfirmed == 0) && (nTransferEnabled == 1))
          {
            rs->getData(0, (void*)&szFilename, sizeof(szFilename));
            rs->getData(1, (void*)&szSender, sizeof(szSender));
            rs->getData(3, (void*)&dwSize, sizeof(dwSize));
            rs->getData(5, (void*)&dwReceiverDeviceID, sizeof(dwReceiverDeviceID));
            rs->getData(7, (void*)&nDataType, sizeof(nDataType));
            rs->getData(13, (void*)&nBlockSize, sizeof(nBlockSize));
            
            if(nDataType == DATATABLE_DATATYPE_MSG)
            {
              dwLocalUserID = GetUserIdByName(szSender);
            };

            Serial.print(F("[OnSched] File ID: "));
            Serial.print(task.dwFileID);
            Serial.print(F(" RCPT: "));
            Serial.print(szFilename);
            Serial.print(F(" Sender: "));
            Serial.print(szSender);
            Serial.print(F(" SenderID: "));
            Serial.print(dwLocalUserID);
            Serial.print(F(" Size: "));
            Serial.print(dwSize);
            Serial.print(F(" DestDev: "));
            Serial.print(dwReceiverDeviceID);
            Serial.print(F(" Type: "));
            Serial.println(nDataType);


            

            if(((dwLocalUserID > 0) && (nDataType == DATATABLE_DATATYPE_MSG)) || (nDataType != DATATABLE_DATATYPE_MSG) && (dwReceiverDeviceID != 0))
            {
              if(g_pCLoRaProtocol->enqueueFileTransfer(dwReceiverDeviceID, dwLocalUserID, (char*)&szFilename, task.dwFileID, (char*)&szSender, dwSize, nDataType, nBlockSize, dwScheduleID) == true)
              {
                dwSize = ClockPtr->getUnixTimestamp();
  
                //set last tx time
                rs->setData(6, (void*)&dwSize, sizeof(dwSize));
              };
            }
            else 
            {
              Serial.println(F("[OnSched] SENDMESSAGE Err local user or remote device not exist!"));

              g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
            };
          }
          else
          {
            Serial.print(F("[OnSched] File ID: "));
            Serial.print(task.dwFileID);
            Serial.print(F(" enabled: "));
            Serial.println(nTransferEnabled);
          };
        };

        delete rs;
      };

      if(bExist == false)
      {
        Serial.println(F("[OnSched] SENDMESSAGE Err file not exist!"));
      
        g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
      };

      if(nHeaderTransferConfirmed == 1)
      {
        Serial.println(F("[OnSched] SENDMESSAGE header transferred"));
      
        g_pDbTaskScheduler->removeSchedule(dwScheduleID, true);
      };
    };
    break;


    
    case DBTASK_REQUESTFILEDATA:
    {
      //variables
      ///////////
      sDBTaskSendFile     task;
      CWSFFileDBRecordset *pRecordset;
      CWSFFileDB          *pDatabase;
      CWSFFileDBRecordset rsNode(g_pNodeTable);
      char                szLocalUser[33];
      char                szRemoteUser[33];
      char                szRemoteDevName[33];
      char                szDatabaseFile[33];
      uint32_t            dwSourceDeviceID;
      int                 nFileComplete;
      int                 nTransferType;
      int                 nTransferEnabled;
      uint32_t            dwSourceDeviceFileID;
      int                 nBlockNumber;
      bool                bRemoveTask = false;
      uint32_t            dwTempID;
      int                 nTemp;
      byte                bData[DATATABLE_BLOCKSIZE + 1];
      int                 nDataLen;
      uint32_t            dwLocalUserID;
      bool                bRequestSent = false;
      char                *szMsg;
      bool                bSuccess = false;
      uint32_t            dwSenderUserID = 0;


      memcpy(&task, pData, sizeof(task));
      
      memset((void*)&szLocalUser, 0, sizeof(szLocalUser));
      memset((void*)&szRemoteUser, 0, sizeof(szRemoteUser));
      memset((void*)&szRemoteDevName, 0, sizeof(szRemoteDevName));
      
      Serial.print(F("[OnSched] REQUESTFILEDATA File ID: "));
      Serial.println(task.dwFileID);

            
      if(g_pDataHeaderTable->isOpen() == true)
      {
        pRecordset = new CWSFFileDBRecordset(g_pDataHeaderTable, task.dwFileID);  

        //record exist?
        if(pRecordset->haveValidEntry() == true)
        {
          memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));

          pRecordset->getData(0, (void*)&szLocalUser, sizeof(szLocalUser));
          pRecordset->getData(1, (void*)&szRemoteUser, sizeof(szRemoteUser));
          pRecordset->getData(4, (void*)&dwSourceDeviceID, sizeof(dwSourceDeviceID));
          pRecordset->getData(7, (void*)&nTransferType, sizeof(nTransferType));
          pRecordset->getData(8, (void*)&nFileComplete, sizeof(nFileComplete));
          pRecordset->getData(9, (void*)&dwSourceDeviceFileID, sizeof(dwSourceDeviceFileID));
          pRecordset->getData(10, (void*)&nTransferEnabled, sizeof(nTransferEnabled));
          pRecordset->getData(15, (void*)&dwSenderUserID, sizeof(dwSenderUserID));

          dwLocalUserID = GetUserIdByName(szLocalUser);

          if((nTransferEnabled == 1) && (nFileComplete == 0))
          {
            delete pRecordset;

            if(g_pDataTable->isOpen() == true)
            { 
              pRecordset = new CWSFFileDBRecordset(g_pDataTable);  

              while(pRecordset->haveValidEntry() == true)
              {
                pRecordset->getData(0, (void*)&dwTempID, sizeof(dwTempID));
                pRecordset->getData(1, (void*)&nBlockNumber, sizeof(nBlockNumber));
                pRecordset->getData(2, (void*)&nTemp, sizeof(nTemp));
                
                if(dwTempID == task.dwFileID)
                {
                  //block transferred?
                  if(nTemp == 0)
                  {
                    Serial.print(F("[OnSched] REQUESTFILEDATA File ID: "));
                    Serial.print(task.dwFileID);
                    Serial.print(F(" req block: "));
                    Serial.println(nBlockNumber);
  
                    //request data
                    g_pCLoRaProtocol->enqueueRequestFileData(dwSourceDeviceID, task.dwFileID, dwSourceDeviceFileID, nBlockNumber, dwScheduleID, dwLocalUserID);
                    
                    bRequestSent = true;
                    
                    break;
                  };
                };

                pRecordset->moveNext();
              };

              delete pRecordset;
            };

            
            //check if all blocks are received, if true, create the 
            //message entry
            if(bRequestSent == false)
            {
              //transfer finished
              Serial.print(F("[OnSched] REQUESTFILEDATA File ID: "));
              Serial.print(task.dwFileID);
              Serial.print(F(" complete, type: "));
              Serial.println(DATATABLE_DATATYPE_MSG);
              
              memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
              sprintf((char*)&szDatabaseFile, DATAHEADERTABLE_FILE);

              switch(nTransferType)
              {
                case DATATABLE_DATATYPE_MSG:
                {
                  if(FindDeviceByNodeID(&rsNode, dwSourceDeviceID) == true)
                  {
                    rsNode.getData(1, (void*)&szRemoteDevName, sizeof(szRemoteDevName));
                    
                    szMsg = new char[LORALINK_MAX_MESSAGE_SIZE + 1];
                    memset(szMsg, 0, LORALINK_MAX_MESSAGE_SIZE);

                    if(assembleMessageData((byte*)szMsg, LORALINK_MAX_MESSAGE_SIZE, task.dwFileID) > 0)
                    {
                      Serial.print(F("Save chat message from: "));
                      Serial.print(szRemoteUser);
                      Serial.print(F("@"));
                      Serial.println(szRemoteDevName);
                      
                      g_pCLoRaProtocol->addMessage(dwLocalUserID, szRemoteUser, szRemoteDevName, szMsg, dwSourceDeviceID, dwSenderUserID, 0, CHAT_DIRECTION_INCOMING);

                      bRemoveTask = true;
                      bSuccess    = true;
                    }
                    else
                    {
                      Serial.print(F("could not assemble File ID: "));
                      Serial.println(task.dwFileID);

                      bRemoveTask = true;
                      bSuccess    = false;
                    };
  
                    delete szMsg;
                  }
                  else
                  {
                    Serial.print(F("Unable to find device name for: "));
                    Serial.println(dwSourceDeviceID);
                    
                    bRemoveTask = true;
                    bSuccess    = false;
                  };
                };
                break;
              };
            };
          }
          else
          {
            delete pRecordset;
          };
        }
        else
        {
          Serial.print(F("Unable to find data for file: "));
          Serial.println(task.dwFileID);
                    
          bRemoveTask = true;

          delete pRecordset;
        };
      }
      else 
      {
        Serial.print(F("Unable to open db for file: "));
        Serial.println(task.dwFileID);
          
        bRemoveTask = true;
      };
      
      if(bRemoveTask == true)
      {
        //remove task
        g_pDbTaskScheduler->removeSchedule(dwScheduleID, bSuccess);
      };
    };
    break;



    case DBTASK_CONFIRMTRANSFER:
    {
      //variables
      ///////////
      sDBTaskSendFile     task;
      CWSFFileDBRecordset *pRecordset;
      CWSFFileDB          *pDatabase;
      char                szDatabaseFile[33];
      uint32_t            dwSourceDeviceID;
      uint32_t            dwSourceDeviceFileID;

      
      memcpy(&task, pData, sizeof(task));

      memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
      sprintf((char*)&szDatabaseFile, DATAHEADERTABLE_FILE);
     
      Serial.print(F("[OnSched] CONFIRMTRANSFER File ID: "));
      Serial.println(task.dwFileID);

      pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nDataHeaderTableDef, DATAHEADERTABLE_SIZE, false, DBRESERVE_DDATA_COUNT);  
      pDatabase->open();
            
      if(pDatabase->isOpen() == true)
      {
        pRecordset = new CWSFFileDBRecordset(pDatabase, task.dwFileID);  

        //record exist?
        if(pRecordset->haveValidEntry() == true)
        {
          pRecordset->getData(4, (void*)&dwSourceDeviceID, sizeof(dwSourceDeviceID));
          pRecordset->getData(9, (void*)&dwSourceDeviceFileID, sizeof(dwSourceDeviceFileID));

          g_pCLoRaProtocol->enqueueFileComplete(dwSourceDeviceID, task.dwFileID, dwSourceDeviceFileID, dwScheduleID);
        }
        else
        {
          Serial.print(F("[OnSched] CONFIRMTRANSFER failed, File ID: "));
          Serial.println(task.dwFileID);
      
          //remove task
          g_pDbTaskScheduler->removeSchedule(dwScheduleID, false);
        };

        delete pRecordset;
      };

      delete pDatabase;
    };
    break;



    case DBTASK_RSTFILETRANSFER:
    {
      //variables
      ///////////
      sDBTaskRestartFileTransfer  task;
      CWSFFileDBRecordset         *pRecordset;
      char                        szUser[70];
      uint32_t                    dwTime;
      char                        szMsg[300];
      int                         nSuccess;
      uint32_t                    dwSender;

      memcpy(&task, pData, sizeof(task));
      

      //check all shout out messages if they are transferred
      //to at least one node, check by count to speed things up
      if(g_nShoutoutCount != g_pShoutOutTable->getRecordCount())
      {
        pRecordset = new CWSFFileDBRecordset(g_pShoutOutTable);
  
        while(pRecordset->haveValidEntry() == true)
        {
          memset(szUser, 0, sizeof(szUser));
          memset(szMsg, 0, sizeof(szMsg));
          
          pRecordset->getData(0, (void*)&szUser, sizeof(szUser));
          pRecordset->getData(1, (void*)&dwTime, sizeof(dwTime));
          pRecordset->getData(2, (void*)&szMsg, sizeof(szMsg));
          pRecordset->getData(3, (void*)&nSuccess, sizeof(nSuccess));
          pRecordset->getData(4, (void*)&dwSender, sizeof(dwSender));
  
          if(nSuccess == 0)
          {
            nSuccess = (g_pCLoRaProtocol->enqueueShoutOut(dwSender, 0, (char*)&szUser, (char*)&szMsg, dwTime) == true ? 1 : 0);
  
            if(nSuccess == 1)
            {
              pRecordset->setData(3, (void*)&nSuccess, sizeof(nSuccess));
            };
          };
  
          pRecordset->moveNext();
        };
  
        delete pRecordset;
      
        g_nShoutoutCount = g_pShoutOutTable->getRecordCount();
      };
    };
    break;


    #if LORALINK_HARDWARE_WIFI == 1

      /**
       * this schedule checks if the webserver is still accessible
       * I ecounterred the problem, that if something in the communication
       * between webserver and browser fail, the comminication stops completly.
       * the memory also goes down to 20k, the device is still able to forward messages,
       * but not serving the website or wlan. This schedule connects to the server and if connect is
       * not possible restarts the esp...
       */
      case DBTASK_CHECKWEBSERVER:
      {
        //variables
        ///////////
        CTCPClient *pNewClient;
        
        Serial.println(F("[OnSched] Webserver check:"));
        
        pNewClient = new CTCPClient(LoRaWiFiApCfg.szDevIP, 80);
  
        if(pNewClient->isConnected() == false)
        {
          if(++g_nWebserverCheck > 2)
          {
            Serial.println(F("[OnSched] ERROR: Internal server does not repond, assume dev in error condition - reboot"));
            delay(10000);
    
            ESP.restart();
          };
        }
        else
        {
          Serial.println(F("[OnSched] Webserver check OK"));

          g_nWebserverCheck = 0;
        };
  
        delete pNewClient;
      };
      break;

    #endif
  };
};



void OnTaskScheduleRemove(uint32_t dwScheduleID, int nScheduleType, uint32_t dwLastExec, int nNumberOfExec, int nMaxTries, byte *pData, bool bSuccess)
{
  Serial.print(F("[OnSchedRemove] ID: "));
  Serial.print(dwScheduleID);
  Serial.print(F(" type: "));
  Serial.print(nScheduleType);
  Serial.print(F(" exec#: "));
  Serial.print(nNumberOfExec);
  Serial.print(F(" max try#: "));
  Serial.print(nMaxTries);
  Serial.print(F(" success: "));
  Serial.println(bSuccess);

  
  switch(nScheduleType)
  {
    #if LORALINK_HARDWARE_GPS == 1
    
      case DBTASK_SEND_POSITION:
      {
        g_bLocationTrackingActive = false;
        g_nLocationTrackingType   = GPS_POSITION_TYPE_NORMAL;
      };
      break;
    
    #endif
    
    case DBTASK_QUERYNODE:
    {
      //variables
      ///////////
      sDBTaskQueryNode    task;
      CWSFFileDB          *pDatabase;
      char                szDatabaseFile[50];
      int                 nContactState = 3;      //node not found
      
      memcpy(&task, pData, sizeof(task));

      memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
      sprintf((char*)&szDatabaseFile, CONTACTSTABLE_FILE, task.dwUserToInform);

      if(bSuccess == true)
      {
        //enable next task
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->enableTask(task.dwReleaseTask);
          g_pDbTaskScheduler->invokeTask(task.dwReleaseTask);
        };
      }
      else
      {
        //remove task
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->removeSchedule(task.dwReleaseTask, false);
        };

        //update contact table
        pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nContactsTableDef, CONTACTSTABLE_SIZE, true, DBRESERVE_CONTACT_COUNT);  
        pDatabase->open();
        
        if(pDatabase->isOpen() == true)
        {
          CWSFFileDBRecordset rs(pDatabase, task.dwContactID);
          rs.setData(4, (byte*)&nContactState, sizeof(nContactState));
        };

        delete pDatabase;
      };
    };
    break;


    case DBTASK_QUERYUSER:
    {
      //variables
      ///////////
      sDBTaskQueryUser    task;
      
      memcpy(&task, pData, sizeof(task));

      if(bSuccess == true)
      {
        //enable next task
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->enableTask(task.dwReleaseTask);
          g_pDbTaskScheduler->invokeTask(task.dwReleaseTask);
        };
      }
      else
      {
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->removeSchedule(task.dwReleaseTask, false);
        };
      };
    };
    break;


    case DBTASK_SENDMESSAGE:
    {
      //variables
      ///////////
      sDBTaskSendFile     task;
      CWSFFileDBRecordset *pRecordset;
      CWSFFileDB          *pDatabase;
      char                szDatabaseFile[33];
      int                 nTransferType = 0;
      uint32_t            dwSenderUserID = 0;
      uint32_t            dwMessageID = 0;
      bool                bHaveData = false;

      memcpy(&task, pData, sizeof(task));

      Serial.print(F("[OnSchedRemove] SENDMESSAGE File ID: "));
      Serial.print(task.dwFileID);
      Serial.print(F(" success: "));
      Serial.print(bSuccess);
      
      if(bSuccess == true)
      {
        //enable next task
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->enableTask(task.dwReleaseTask);
          g_pDbTaskScheduler->invokeTask(task.dwReleaseTask);
        };

        //nothing else to do here, the other device starts 
        //requesting the data block by block, wait for transfer 
        //complete
      }
      else
      {
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->removeSchedule(task.dwReleaseTask, false);
        };

        //set transfer failed, if message
        memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
        sprintf((char*)&szDatabaseFile, DATAHEADERTABLE_FILE);

        pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nDataHeaderTableDef, DATAHEADERTABLE_SIZE, false, DBRESERVE_DDATA_COUNT);  
        pDatabase->open();
              
        if(pDatabase->isOpen() == true)
        {
          pRecordset = new CWSFFileDBRecordset(pDatabase, task.dwFileID);  
  
          //record exist?
          if(pRecordset->haveValidEntry() == true)
          {
            pRecordset->getData(7, (void*)&nTransferType, sizeof(nTransferType));
            pRecordset->getData(14, (void*)&dwMessageID, sizeof(dwMessageID));
            pRecordset->getData(15, (void*)&dwSenderUserID, sizeof(dwSenderUserID));

            bHaveData = true;
          }
          else
          {
            Serial.print(F("have no data for file id: "));
            Serial.println(task.dwFileID);
          };

          delete pRecordset;
        };

        delete pDatabase;

        if(bHaveData == true)
        {
          if(nTransferType == DATATABLE_DATATYPE_MSG)
          {
            Serial.print(F("Mark msg transfer failed for userid: "));
            Serial.print(dwSenderUserID);
            Serial.print(F(" MsgID: "));
            Serial.println(dwMessageID);
            
            //set transfer failed, so it can be shown in the 
            //web UI
            memset((void*)&szDatabaseFile, 0, sizeof(szDatabaseFile));
            sprintf((char*)&szDatabaseFile, CHATMESSAGETABLE_FILE, dwSenderUserID);
          
            pDatabase = new CWSFFileDB(&LORALINK_DATA_FS, (char*)&szDatabaseFile, (int*)&nChatMessageTableDef, CHATMESSAGETABLE_SIZE, true, DBRESERVE_CHATMSG_COUNT);  
            pDatabase->open();
              
            if(pDatabase->isOpen() == true)
            {
              pRecordset = new CWSFFileDBRecordset(pDatabase, dwMessageID);  
  
              if(pRecordset->haveValidEntry() == true)
              {
                //failed
                nTransferType = 2;
  
                pRecordset->setData(2, (void*)&nTransferType, sizeof(nTransferType));
              };
  
              delete pRecordset;
            };
  
            delete pDatabase;
          };
        };
        
        //remove file data from data tables
        removeDataTransferTablesHeader(task.dwFileID);
      };
    };
    break;


    case DBTASK_REQUESTFILEDATA:
    {
      //variables
      ///////////
      sDBTaskSendFile     task;
      CWSFFileDBRecordset *pRecordset;
      CWSFFileDB          *pDatabase;
      char                szDatabaseFile[33];
      int                 nTransferType = 0;
      uint32_t            dwSenderUserID = 0;
      uint32_t            dwMessageID = 0;

      memcpy(&task, pData, sizeof(task));

      Serial.print(F("[OnSchedRemove] REQUESTFILEDATA File ID: "));
      Serial.print(task.dwFileID);
      Serial.print(F(" success: "));
      Serial.print(bSuccess);
      
      if(bSuccess == true)
      {
        //enable next task
        if(task.dwReleaseTask > 0)
        {
          g_pDbTaskScheduler->enableTask(task.dwReleaseTask);
          g_pDbTaskScheduler->invokeTask(task.dwReleaseTask);
        };

        //nothing else to do here, the other device starts 
        //requesting the data block by block, wait for transfer 
        //complete
      }
      else
      {
        //remove file data from data tables
        removeDataTransferTablesHeader(task.dwFileID);

        //remove task
        g_pDbTaskScheduler->removeSchedule(task.dwReleaseTask, false);
      };
    };
    break;
  };    
};


//called from the connection handler, when it receives
//a DATA_IND
void OnLoRaLinkProtocolData(void *pProtocolMsg, byte *pData, int nDataLen)
{
  g_pCLoRaProtocol->handleLoRaLinkProtocolData((_sSkyNetProtocolMessage*)pProtocolMsg, pData, nDataLen);
};
