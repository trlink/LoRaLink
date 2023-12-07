//includes
//////////
#include "LoRaLinkConfig.h"
#include "CFileUtils.h"


#define MAX_CFG_SIZE 2000


//globals
/////////
sLoRaWiFiCfg                  LoRaWiFiApCfg;                   //WiFi Accesspoint config
sLoRaWiFiCfg                  LoRaWiFiCfg;                     //Wifi Client config 
sAdminConfig                  AdminCfg;
sDeviceConfig                 DeviceConfig;
sIpLinkConfig                 IpLinkConfig;
sModemConfig                  ModemConfig;
sDynamicDNSConfig             DynDNSConfig;



void ReadDeviceConfig()
{
  //variables
  ///////////
  DynamicJsonDocument doc(MAX_CFG_SIZE);
  char      *szOutput = new char[MAX_CFG_SIZE];;
  int       nLength;


  
  //read admin user config
  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_ADMIN, doc) == true)
  {
    DeSerializeAdminConfig(doc);
  }
  else
  {
    Serial.print(F("No Admin Config - create: "));
    Serial.println(CONFIG_FILE_ADMIN); 

    Serial.println(F("Login with: Admin & empty password!"));

    strcpy_P(AdminCfg.szUser, PSTR("Admin"));
    strcpy_P(AdminCfg.szPassword, PSTR(""));

    PrepareSerializeAdminConfig(doc);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_ADMIN, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };

    doc.clear();
  };
  


  //read device config
  doc = DynamicJsonDocument(MAX_CFG_SIZE);

  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_DEVICE, doc) == true)
  {
    DeSerializeDeviceConfig(doc);
  }
  else
  {
    Serial.print(F("No Device Config - create: "));
    Serial.println(CONFIG_FILE_DEVICE); 

    strcpy_P(DeviceConfig.szDevName, PSTR("Unknown"));
    strcpy_P(DeviceConfig.szDevOwner, PSTR("Not Set"));
    DeviceConfig.dwDeviceID = 0;
    DeviceConfig.nDeviceType = 3;
    DeviceConfig.bUpdateTimeNTP = true;
    DeviceConfig.cPosOrientation = 'N';
    DeviceConfig.fLocN = 0.0;
    DeviceConfig.fLocE = 0.0;
    DeviceConfig.fBattCorrection = 0.0;
    memset(DeviceConfig.szBlockedNodes, 0, sizeof(DeviceConfig.szBlockedNodes));
    DeviceConfig.nMaxShoutOutEntries = 100;
    
    PrepareSerializeDeviceConfig(doc);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_DEVICE, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };


  doc.clear();
  


  doc = DynamicJsonDocument(MAX_CFG_SIZE);
  

  //read config for WiFi AP
  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_AP, doc) == true)
  {
    DeSerializeWiFiConfig(doc, &LoRaWiFiApCfg);

    Serial.print(F("LoRa Link AP Name: "));
    Serial.println(LoRaWiFiApCfg.szWLANSSID);

    Serial.print(F("LoRa Link IP: "));
    Serial.println(LoRaWiFiApCfg.szDevIP);
  }
  else
  {
    Serial.print(F("No WIFI AP Config - create: "));
    Serial.println(CONFIG_FILE_WIFI_AP);

    strcpy_P(LoRaWiFiApCfg.szWLANSSID, PSTR("LoRaLinkAP"));
    strcpy_P(LoRaWiFiApCfg.szWLANPWD, PSTR(""));
    strcpy_P(LoRaWiFiApCfg.szDevIP, PSTR("192.168.5.1"));
    LoRaWiFiApCfg.bUseDHCP = false;
    LoRaWiFiApCfg.bHideNetwork = false;
    LoRaWiFiApCfg.nChannel = 1;

    PrepareSerializeWiFiConfig(doc, &LoRaWiFiApCfg);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_AP, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };


  doc.clear();
  doc = DynamicJsonDocument(MAX_CFG_SIZE);

  //read config for WiFi Clnt
  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_CLNT, doc) == true)
  {
    DeSerializeWiFiConfig(doc, &LoRaWiFiCfg);

    Serial.print(F("LoRa Link WiFi Client Name: "));
    Serial.println(LoRaWiFiCfg.szWLANSSID);
  }
  else
  {
    Serial.print(F("No WIFI Client Config - create: "));
    Serial.println(CONFIG_FILE_WIFI_CLNT);

    strcpy_P(LoRaWiFiCfg.szWLANSSID, PSTR(""));
    strcpy_P(LoRaWiFiCfg.szWLANPWD, PSTR(""));
    strcpy_P(LoRaWiFiCfg.szDevIP, PSTR(""));
    LoRaWiFiCfg.bUseDHCP = true;
    LoRaWiFiCfg.bHideNetwork = false;
    LoRaWiFiCfg.nChannel = 1;

    PrepareSerializeWiFiConfig(doc, &LoRaWiFiCfg);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_WIFI_CLNT, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };


  doc.clear();
  doc = DynamicJsonDocument(MAX_CFG_SIZE);

  //read ip link config
  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_IPLINK, doc) == true)
  {
    DeSerializeLinkConfig(doc);

    Serial.print(F("LoRa Link Server enabled: "));
    Serial.println(IpLinkConfig.bServerEnabled);
    Serial.print(F("LoRa Link Client enabled: "));
    Serial.println(IpLinkConfig.bClientEnabled);
  }
  else
  {
    Serial.print(F("No Link Config - create: "));
    Serial.println(CONFIG_FILE_IPLINK);

    IpLinkConfig.wServerPort = 9247;
    IpLinkConfig.wClientPort = 9247;
    IpLinkConfig.bServerEnabled = false;
    IpLinkConfig.bClientEnabled = false;
    strcpy_P(IpLinkConfig.szClient, PSTR(""));
    
    PrepareSerializeLinkConfig(doc);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_IPLINK, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };


  doc.clear();
  
  //read modem config
  doc = DynamicJsonDocument(MAX_CFG_SIZE);
 
  //read config for WiFi AP
  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_MODEM, doc) == true)
  {
    DeSerializeModemConfig(doc);
  }
  else
  {
    Serial.print(F("No Modem Config - create: "));
    Serial.println(CONFIG_FILE_MODEM);

    ModemConfig.bDisableLoRaModem             = true;
    ModemConfig.nTxPower                      = 17;
    ModemConfig.nSyncWord                     = LORA_MAC_PRIVATE_SYNCWORD;
    ModemConfig.lMessageTransmissionInterval  = 500;    
    ModemConfig.lFrequency                    = 434000000;
    ModemConfig.lBandwidth                    = 125;
    ModemConfig.nSpreadingFactor              = 12;
    ModemConfig.nCodingRate                   = 4;
    ModemConfig.nPreamble                     = 8;
    ModemConfig.nTransmissionReceiveTimeout   = 60;
    ModemConfig.lModemRxDelay                 = 500;
    ModemConfig.lModemTxDelay                 = 1500;

    PrepareSerializeModemConfig(doc);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_MODEM, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };


  doc.clear();
  doc = DynamicJsonDocument(MAX_CFG_SIZE);

  if(ReadConfig(LORALINK_CONFIG_FS, CONFIG_FILE_DDNS, doc) == true)
  {
    DeSerializeDynDNSConfig(doc);
  }
  else
  {
    Serial.print(F("No DDNS Config - create: "));
    Serial.println(CONFIG_FILE_DDNS);

    memset(DynDNSConfig.szProvider, 0, sizeof(DynDNSConfig.szProvider));
    memset(DynDNSConfig.szDomain, 0, sizeof(DynDNSConfig.szDomain));
    DynDNSConfig.bAuthWithUser = false;
    memset(DynDNSConfig.szUser, 0, sizeof(DynDNSConfig.szUser));
    memset(DynDNSConfig.szPassword, 0, sizeof(DynDNSConfig.szPassword));

    
    PrepareSerializeDynDNSConfig(doc);

    nLength = serializeJson(doc, szOutput, MAX_CFG_SIZE);

    if(CFileUtils::WriteFile(LORALINK_CONFIG_FS, CONFIG_FILE_DDNS, (byte*)szOutput, nLength) == true)
    {
      Serial.println(F("Config written..."));
    };
  };

  doc.clear();
  delete szOutput;
};




//reads dev config
bool ReadConfig(fs::FS &fs, char *szConfigFile, DynamicJsonDocument &doc)
{
  //variables
  ///////////
  File fConfigFile;

  Serial.print(F("--> ReadConfig(): "));
  Serial.println(szConfigFile);
  
  fConfigFile = LORALINK_CONFIG_FS.open(szConfigFile, "r");

  DeserializationError error = deserializeJson(doc, fConfigFile);
  
  if(error) 
  {
    Serial.print(F("Error: deserializeJson: "));
    Serial.println(error.c_str());

    return false;
  };
  
  Serial.print(F("serializeJson = "));
  serializeJson(doc, Serial);
  Serial.println("");

  fConfigFile.close();
    
  Serial.println(F("<-- ReadConfig()"));

  return true;
};




void DeSerializeDynDNSConfig(DynamicJsonDocument &doc) 
{
  if(doc.containsKey(F("szProvider"))) 
  {
    strcpy(DynDNSConfig.szProvider, doc[F("szProvider")]);
  };

  if(doc.containsKey(F("szDomain"))) 
  {
    strcpy(DynDNSConfig.szDomain, doc[F("szDomain")]);
  };
   
  if(doc.containsKey(F("szUser"))) 
  {
    strcpy(DynDNSConfig.szUser, doc[F("szUser")]);
  };

  if(doc.containsKey(F("szPassword"))) 
  {
    strcpy(DynDNSConfig.szPassword, doc[F("szPassword")]);
  };

  if(doc[F("bAuthWithUser")]) 
  {
    DynDNSConfig.bAuthWithUser = doc[F("bAuthWithUser")];
  };
};



void PrepareSerializeDynDNSConfig(DynamicJsonDocument &doc)
{
  doc[F("szProvider")] = DynDNSConfig.szProvider;
  doc[F("bAuthWithUser")] = DynDNSConfig.bAuthWithUser;
  doc[F("szUser")] = DynDNSConfig.szUser;
  doc[F("szPassword")] = DynDNSConfig.szPassword;
};








void DeSerializeModemConfig(DynamicJsonDocument &doc) 
{
  if(doc.containsKey(F("DisableLoRa")))
  {
    ModemConfig.bDisableLoRaModem = doc[F("DisableLoRa")];
  }
  else 
  {
    ModemConfig.bDisableLoRaModem = true;
  };

  if(doc[F("Freq")]) 
  {
    ModemConfig.lFrequency = doc[F("Freq")];
  };

  if(doc[F("BW")]) 
  {
    ModemConfig.lBandwidth = doc[F("BW")];
  };

  if(doc[F("Power")]) 
  {
    ModemConfig.nTxPower = doc[F("Power")];
  };

  if(doc[F("Preamble")]) 
  {
    ModemConfig.nPreamble = doc[F("Preamble")];
  };

  if(doc[F("SW")]) 
  {
    ModemConfig.nSyncWord = doc[F("SW")];
  };

  if(doc[F("SF")]) 
  {
    ModemConfig.nSpreadingFactor = doc[F("SF")];
  };

  if(doc[F("CR")]) 
  {
    ModemConfig.nCodingRate = doc[F("CR")];
  };

  if(doc[F("MsgTxInt")]) 
  {
    ModemConfig.lMessageTransmissionInterval = doc[F("MsgTxInt")];
  };

  if(doc[F("nTransmissionReceiveTimeout")]) 
  {
    ModemConfig.nTransmissionReceiveTimeout = doc[F("nTransmissionReceiveTimeout")];
  };

  if(doc[F("lModemTxDelay")]) 
  {
    ModemConfig.lModemTxDelay = doc[F("lModemTxDelay")];
  };

  if(doc[F("lModemRxDelay")]) 
  {
    ModemConfig.lModemRxDelay = doc[F("lModemRxDelay")];
  };
  
};







void PrepareSerializeModemConfig(DynamicJsonDocument &doc)
{
  doc[F("DisableLoRa")] = ModemConfig.bDisableLoRaModem;
  doc[F("Freq")] = ModemConfig.lFrequency;
  doc[F("BW")] = ModemConfig.lBandwidth;
  doc[F("Power")] = ModemConfig.nTxPower;
  doc[F("Preamble")] = ModemConfig.nPreamble;
  doc[F("SW")] = ModemConfig.nSyncWord;
  doc[F("SF")] = ModemConfig.nSpreadingFactor;
  doc[F("CR")] = ModemConfig.nCodingRate;
  doc[F("MsgTxInt")] = ModemConfig.lMessageTransmissionInterval;  
  doc[F("nTransmissionReceiveTimeout")] = ModemConfig.nTransmissionReceiveTimeout;  
  doc[F("lModemTxDelay")] = ModemConfig.lModemTxDelay;  
  doc[F("lModemRxDelay")] = ModemConfig.lModemRxDelay;  
};




void DeSerializeDeviceConfig(DynamicJsonDocument &doc)
{
  if(doc.containsKey(F("fBattCorrection")))
  {
    DeviceConfig.fBattCorrection = doc[F("fBattCorrection")];
  }
  else
  {
    DeviceConfig.fBattCorrection = 0.0;
  };
  
  if(doc.containsKey(F("szDevOwner"))) 
  {
    strcpy(DeviceConfig.szDevOwner, doc[F("szDevOwner")]);
  };
  
  if(doc.containsKey(F("szDevName"))) 
  {
    strcpy(DeviceConfig.szDevName, doc[F("szDevName")]);
  };

  if(doc.containsKey(F("szBlockedNodes"))) 
  {
    strcpy(DeviceConfig.szBlockedNodes, doc[F("szBlockedNodes")]);
  };
  
  if(doc.containsKey(F("dwDeviceID")))
  {
    DeviceConfig.dwDeviceID = doc[F("dwDeviceID")];
  };

  if(doc.containsKey(F("nDeviceType")))
  {
    DeviceConfig.nDeviceType = doc[F("nDeviceType")];
  };

  if(doc.containsKey(F("bUpdateTimeNTP")))
  {
    DeviceConfig.bUpdateTimeNTP = doc[F("bUpdateTimeNTP")];
  };

  if(doc.containsKey(F("cPosOrientation")))
  {
    DeviceConfig.cPosOrientation = doc[F("cPosOrientation")];
  };

  if(doc.containsKey(F("fLocN")))
  {
    DeviceConfig.fLocN = doc[F("fLocN")];
  };

  if(doc.containsKey(F("fLocE")))
  {
    DeviceConfig.fLocE = doc[F("fLocE")];
  };

  if(doc.containsKey(F("nMaxShoutOutEntries")))
  {
    DeviceConfig.nMaxShoutOutEntries = doc[F("nMaxShoutOutEntries")];
  };
};




void PrepareSerializeDeviceConfig(DynamicJsonDocument &doc) 
{
  doc[F("fBattCorrection")] = DeviceConfig.fBattCorrection;
  doc[F("dwDeviceID")] = DeviceConfig.dwDeviceID;
  doc[F("szDevName")] = DeviceConfig.szDevName;
  doc[F("szDevOwner")] = DeviceConfig.szDevOwner;
  doc[F("nDeviceType")] = DeviceConfig.nDeviceType;
  doc[F("bUpdateTimeNTP")] = DeviceConfig.bUpdateTimeNTP;
  doc[F("cPosOrientation")] = DeviceConfig.cPosOrientation;
  doc[F("fLocN")] = DeviceConfig.fLocN;
  doc[F("fLocE")] = DeviceConfig.fLocE;
  doc[F("szBlockedNodes")] = DeviceConfig.szBlockedNodes;
  doc[F("nMaxShoutOutEntries")] = DeviceConfig.nMaxShoutOutEntries;
};








void DeSerializeLinkConfig(DynamicJsonDocument &doc)
{
  if(doc.containsKey(F("szClient"))) 
  {
    strcpy(IpLinkConfig.szClient, doc[F("szClient")]);
  };

  if(doc.containsKey(F("wServerPort")))
  {
    IpLinkConfig.wServerPort = doc[F("wServerPort")];
  };

  if(doc.containsKey(F("bServerEnabled")))
  {
    IpLinkConfig.bServerEnabled = doc[F("bServerEnabled")];
  };

  if(doc.containsKey(F("wClientPort")))
  {
    IpLinkConfig.wClientPort = doc[F("wClientPort")];
  };

  if(doc.containsKey(F("bClientEnabled")))
  {
    IpLinkConfig.bClientEnabled = doc[F("bClientEnabled")];
  };
};




void PrepareSerializeLinkConfig(DynamicJsonDocument &doc) 
{
  doc[F("wServerPort")] = IpLinkConfig.wServerPort;
  doc[F("bServerEnabled")] = IpLinkConfig.bServerEnabled;
  doc[F("szClient")] = IpLinkConfig.szClient;
  doc[F("wClientPort")] = IpLinkConfig.wClientPort;
  doc[F("bClientEnabled")] = IpLinkConfig.bClientEnabled;
};




void DeSerializeAdminConfig(DynamicJsonDocument &doc)
{
  if(doc.containsKey(F("szUser"))) 
  {
    strcpy(AdminCfg.szUser, doc[F("szUser")]);
  };

  if(doc.containsKey(F("szPassword")))
  {
    strcpy(AdminCfg.szPassword, doc[F("szPassword")]);
  };
};


void PrepareSerializeAdminConfig(DynamicJsonDocument &doc) 
{
  doc[F("szUser")] = AdminCfg.szUser;
  doc[F("szPassword")] = AdminCfg.szPassword;
};





void DeSerializeWiFiConfig(DynamicJsonDocument &doc, sLoRaWiFiCfg *pWiFiCfg) 
{ 
  if(doc[F("szWLANSSID")]) 
  {
    strcpy(pWiFiCfg->szWLANSSID, doc[F("szWLANSSID")]);
  };

  if(doc[F("szWLANPWD")]) 
  {
    strcpy(pWiFiCfg->szWLANPWD, doc[F("szWLANPWD")]);
  };

  if(doc[F("szDevIP")]) 
  {
    strcpy(pWiFiCfg->szDevIP, doc[F("szDevIP")]);
  };

  if(doc.containsKey(F("bUseDHCP"))) 
  {
    pWiFiCfg->bUseDHCP = doc[F("bUseDHCP")];
  }
  else
  {
    pWiFiCfg->bUseDHCP = false;
  }

  if(doc.containsKey(F("bHideNetwork"))) 
  {
    pWiFiCfg->bHideNetwork = doc[F("bHideNetwork")];
  }
  else
  {
    pWiFiCfg->bHideNetwork = false;
  };
  
  if(doc.containsKey(F("nChannel"))) 
  {
    pWiFiCfg->nChannel = doc[F("nChannel")];
  }
  else
  {
    pWiFiCfg->nChannel = 1;
  };  
};



void PrepareSerializeWiFiConfig(DynamicJsonDocument &doc, sLoRaWiFiCfg *pWiFiCfg) 
{
  doc[F("szWLANSSID")] = pWiFiCfg->szWLANSSID;
  doc[F("szWLANPWD")] = pWiFiCfg->szWLANPWD;
  doc[F("szDevIP")] = pWiFiCfg->szDevIP;
  doc[F("bUseDHCP")] = pWiFiCfg->bUseDHCP;
  doc[F("nChannel")] = pWiFiCfg->nChannel;
  doc[F("bHideNetwork")] = pWiFiCfg->bHideNetwork;
};



  
