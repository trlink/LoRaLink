#ifndef __LORA_LINK_CFG__
#define __LORA_LINK_CFG__

//includes
//////////
#include <Arduino.h>
#include <ArduinoJson.h>
#include "helper.h"



//defines
/////////
#define CONFIG_FILE_WIFI_AP         "/wifiAP.jsn"
#define CONFIG_FILE_WIFI_CLNT       "/wifiClnt.jsn"
#define CONFIG_FILE_ADMIN           "/admin.jsn"
#define CONFIG_FILE_DEVICE          "/device.jsn"
#define CONFIG_FILE_IPLINK          "/iplink.jsn"
#define CONFIG_FILE_MODEM           "/modem.jsn"
#define CONFIG_FILE_DDNS            "/ddns.jsn"


//possible device types
#define DEVICE_TYPE_UNKNOWN                 0
#define DEVICE_TYPE_PERSONAL                1
#define DEVICE_TYPE_PERSONAL_REPEATER       2
#define DEVICE_TYPE_PUBLIC_REPEATER         3
#define DEVICE_TYPE_REPEATER                4



struct sDynamicDNSConfig
{
  char szProvider[20];
  bool bAuthWithUser;
  char szDomain[255];
  char szUser[255];
  char szPassword[100];
};


struct sAdminConfig
{
  char szUser[21];
  char szPassword[50];
};


struct sLoRaWiFiCfg 
{
  char szWLANSSID[31];
  char szWLANPWD[31];
  char szDevIP[20];
  bool bUseDHCP;
  bool bHideNetwork;
  int  nChannel;
  //non volatile (helper variables)
  int  nAvailNetworks;
  char **szNetwork;
  bool bWiFiEnabled;
};


struct sDeviceConfig
{
  uint32_t dwDeviceID;
  char     szDevName[26];
  char     szDevOwner[201];
  int      nDeviceType;
  bool     bUpdateTimeNTP;
  float    fLocN;
  float    fLocE;
  char     cPosOrientation;
  char     szBlockedNodes[50];
  int      nMaxShoutOutEntries;
  float    fBattCorrection;
};


struct sIpLinkConfig
{
  uint16_t  wServerPort;
  bool      bServerEnabled;
  char      szClient[100];
  uint16_t  wClientPort;
  bool      bClientEnabled;
};



struct sModemConfig 
{
  bool bDisableLoRaModem;               //if true the modem is disabled
  long lFrequency;
  long lBandwidth;
  int  nTxPower;
  int  nPreamble;
  int  nSyncWord;
  int  nSpreadingFactor;
  int  nCodingRate;
  long lMessageTransmissionInterval;    //interval in ms, we check the data queue
  int  nTransmissionReceiveTimeout; 
  long lModemTxDelay;
  long lModemRxDelay;
};





//globals
/////////
extern sAdminConfig                 AdminCfg;
extern sLoRaWiFiCfg                 LoRaWiFiApCfg;
extern sLoRaWiFiCfg                 LoRaWiFiCfg;
extern sDeviceConfig                DeviceConfig;
extern sIpLinkConfig                IpLinkConfig;
extern sModemConfig                 ModemConfig;
extern sDynamicDNSConfig            DynDNSConfig;

void ReadDeviceConfig();

void DeSerializeDynDNSConfig(DynamicJsonDocument &doc); 
void PrepareSerializeDynDNSConfig(DynamicJsonDocument &doc);


void DeSerializeModemConfig(DynamicJsonDocument &doc); 
void PrepareSerializeModemConfig(DynamicJsonDocument &doc);


void DeSerializeLinkConfig(DynamicJsonDocument &doc);
void PrepareSerializeLinkConfig(DynamicJsonDocument &doc);


void DeSerializeDeviceConfig(DynamicJsonDocument &doc);
void PrepareSerializeDeviceConfig(DynamicJsonDocument &doc);


void DeSerializeAdminConfig(DynamicJsonDocument &doc);
void PrepareSerializeAdminConfig(DynamicJsonDocument &doc);


//reads the doc into the config struct
void DeSerializeWiFiConfig(DynamicJsonDocument &doc, sLoRaWiFiCfg *pWiFiCfg);  
void PrepareSerializeWiFiConfig(DynamicJsonDocument &doc, sLoRaWiFiCfg *pWiFiCfg); 


//read any config into the DynamicJsonDocument
bool ReadConfig(fs::FS &fs, char *szConfigFile, DynamicJsonDocument &doc);

//write the config 
bool WriteConfig(fs::FS &fs, char *szConfigFile, DynamicJsonDocument &doc); 








#endif
