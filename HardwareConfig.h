#ifndef __LORALINK_HW_CFG__
#define __LORALINK_HW_CFG__

  //general config
  ////////////////
  #define LORALINK_VERSION_STRING               "v2.2"
  #define LORALINK_MAX_MESSAGE_SIZE             1500
  #define LORALINK_HARDWARE_MAX_FILES           12
  #define LORALINK_HARDWARE_SDCARD              1       //device has an sdcard as storage
  #define LORALINK_HARDWARE_SPIFFS              0       //device has an internal spi ffs (not longer supported)
  #define LORALINK_HARDWARE_BATSENSE            0       //device has a power sensor (Resistor at analog port)
  #define LORALINK_HARDWARE_LED                 1       //Device has LEDs
  #define LORALINK_HARDWARE_GPS                 1       //Device has GPS
  #define LORALINK_HARDWARE_WIFI                1       //device has WiFi
  #define LORALINK_HARDWARE_OLED                1       //device has a display
  #define LORALINK_STACKSIZE_WEBSERVER          18000  
  #define LORALINK_STACKSIZE_MODEM              8000
  #define LORALINK_STACKSIZE_MODEM_DATA         6000
  #define LORALINK_STACKSIZE_BLINK              1000
  #define LORALINK_STACKSIZE_DISPLAY            2200
  #define LORALINK_STACKSIZE_GPS_DATA           3000

  
  //behaviour
  ///////////
  #define INFO_CARD_SWITCH_INTERVAL             7000    //time in ms after the info display will be changed
  #define LORALINK_POSITION_INTERVAL_SECONDS    15



  //hardware
  //////////
  
  //for t-beam devices uncomment this line
  #define LORALINK_HARDWARE_TBEAM

  //for heltec
  //#define LORALINK_HARDWARE_ESP32V3
  
  //select ESP32 Dev Module and set the following:
  //Flash Size: 4MB
  //Partition Scheme: Default 4MB SPIFFS, 1.2MB APP
  //#define LORALINK_HARDWARE_ESP32

  //without modem (ip only nodes)
  //#define LORALINK_HARDWARE_ESP32_NOMODEM
    


  
  //filesystem config
  //can be SD or SPIFFS
  /////////////////////
  #define LORALINK_CONFIG_FS      SD
  #define LORALINK_CONFIG_ROOT    "/config"
  #define LORALINK_WEBAPP_FS      SD
  #define LORALINK_WEBAPP_ROOT    ""
  #define LORALINK_DATA_FS        SD
  #define LORALINK_DATA_ROOT      "/data"
  #define LORALINK_FIRMWARE_FS    SD
  

  #ifdef LORALINK_HARDWARE_ESP32V3

    //includes
    //////////
    #include "Arduino.h"
    #include <driver/adc.h>
    #include <esp_task_wdt.h>
    #include <SPI.h>
    #include <SX126XLT.h>
    #if LORALINK_HARDWARE_WIFI == 1
      #include <DNSServer.h>
      #include <EasyDDNS.h>
    #endif
    #if LORALINK_HARDWARE_SDCARD == 1
      #include <SD.h>
    #endif
    #if LORALINK_HARDWARE_SPIFFS == 1
      #include <SPIFFS.h>  
    #endif

    
    #define LORALINK_HARDWARE_NAME  F("Heltec ESP32 (SX1262) WiFi LoRa 433MHz")
    #define LORALINK_FIRMWARE_FILE  "/HeltecV3.bin"
    #define LORALINK_HARDWARE_LORA  1       //device has a lora modem
    
    //general hardware options
    //////////////////////////
    //#define LORALINK_MODEM_SX127x
    #define LORALINK_MODEM_SX126x
    

    //utility defines
    #define ResetWatchDog           esp_task_wdt_reset


    //includes
    //////////
    #if LORALINK_HARDWARE_GPS == 1
      #include <TinyGPS.h>
      
      #define GPS_RX_PIN                      34
      #define GPS_TX_PIN                      12
    #endif


    //sdcard config
    #if LORALINK_HARDWARE_SDCARD == 1
      #define LORALINK_HARDWARE_SDCARD_MISO 22
      #define LORALINK_HARDWARE_SDCARD_MOSI 21
      #define LORALINK_HARDWARE_SDCARD_SCK  13
      #define LORALINK_HARDWARE_SDCARD_CS   17
    #endif

    #define USER_BUTTON                      38
    #define LORALINK_HARDWARE_STATUS_LED_PIN LED
    
    #if LORALINK_HARDWARE_LED == 1
      #define LORALINK_HARDWARE_TX_LED_PIN      32
    #endif
  
    #if LORALINK_HARDWARE_BATSENSE == 1
      #define LORALINK_HARDWARE_BATSENSE_PIN    0
    #endif
  
    #if LORALINK_HARDWARE_OLED == 1
      #include <Wire.h>
      #include <Adafruit_GFX.h>
      #include <Adafruit_SSD1306.h>

      #define SCREEN_WIDTH 128 // OLED display width, in pixels
      #define SCREEN_HEIGHT 64 // OLED display height, in pixels
      #define LORALINK_HARDWARE_OLED_RESET      RST_OLED // Reset pin # (or -1 if sharing Arduino reset pin)
      #define LORALINK_HARDWARE_SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
      #define LORALINK_HARDWARE_OLED_SDA        SDA_OLED
      #define LORALINK_HARDWARE_OLED_SCL        SCL_OLED
      extern Adafruit_SSD1306 g_display;
    #endif

    #if LORALINK_HARDWARE_LORA == 1
      //LoRa Modem defines
      #define LORALINK_MODEM_TYPE              DEVICE_SX1262
      #define LORALINK_MODEM_SCK               SCK        
      #define LORALINK_MODEM_MISO              MISO       
      #define LORALINK_MODEM_MOSI              MOSI       
      #define LORALINK_MODEM_SS                SS         
      #define LORALINK_MODEM_RST               RST_LoRa   
      #define LORALINK_MODEM_DI0               DIO0
      #define LORALINK_MODEM_BUSY              BUSY_LoRa 
    #endif
  #endif



  

  #ifdef LORALINK_HARDWARE_ESP32
  
    //includes
    //////////
    #include "Arduino.h"
    #include <driver/adc.h>
    #include <esp_task_wdt.h>
    #include <SPI.h>
    #include <SX127XLT.h>
    #if LORALINK_HARDWARE_WIFI == 1
      #include <DNSServer.h>
      #include <EasyDDNS.h>
    #endif
    #if LORALINK_HARDWARE_SDCARD == 1
      #include <SD.h>
    #endif
    #if LORALINK_HARDWARE_SPIFFS == 1
      #include <SPIFFS.h>  
    #endif
    
    //general hardware options
    //////////////////////////        
    #define LORALINK_MODEM_SX127x
    //#define LORALINK_MODEM_SX126x
    


    #define LORALINK_HARDWARE_NAME  F("ESP32 WiFi LoRa (SX1278) 433MHz")
    #define LORALINK_FIRMWARE_FILE  "/ESP.bin"
    #define LORALINK_HARDWARE_LORA  1       //device has a lora modem

    //utility defines
    #define ResetWatchDog           esp_task_wdt_reset

    //includes
    //////////
    #if LORALINK_HARDWARE_GPS == 1
      #include <TinyGPS.h>
      
      #define GPS_RX_PIN                      34
      #define GPS_TX_PIN                      12
    #endif

    //sdcard config
    #if LORALINK_HARDWARE_SDCARD == 1
      #define LORALINK_HARDWARE_SDCARD_MISO 22
      #define LORALINK_HARDWARE_SDCARD_MOSI 21
      #define LORALINK_HARDWARE_SDCARD_SCK  17
      #define LORALINK_HARDWARE_SDCARD_CS   13
    #endif

    #define USER_BUTTON                      38
    #define LORALINK_HARDWARE_STATUS_LED_PIN 32

    #if LORALINK_HARDWARE_LED == 1
      #define LORALINK_HARDWARE_TX_LED_PIN 33
    #endif
  
    #if LORALINK_HARDWARE_BATSENSE == 1
      #define LORALINK_HARDWARE_BATSENSE_PIN 0
    #endif
  

    #if LORALINK_HARDWARE_OLED == 1
      #include <Wire.h>
      #include <Adafruit_GFX.h>
      #include <Adafruit_SSD1306.h>

      #define SCREEN_WIDTH 128 // OLED display width, in pixels
      #define SCREEN_HEIGHT 64 // OLED display height, in pixels
      #define LORALINK_HARDWARE_OLED_RESET      16 // Reset pin # (or -1 if sharing Arduino reset pin)
      #define LORALINK_HARDWARE_SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
      #define LORALINK_HARDWARE_OLED_SDA        4
      #define LORALINK_HARDWARE_OLED_SCL        15
      extern Adafruit_SSD1306 g_display;
    #endif       

    #if LORALINK_HARDWARE_LORA == 1
      //LoRa Modem defines
      #define LORALINK_MODEM_TYPE              DEVICE_SX1278
      #define LORALINK_MODEM_SCK               5        
      #define LORALINK_MODEM_MISO              19       
      #define LORALINK_MODEM_MOSI              27       
      #define LORALINK_MODEM_SS                18         
      #define LORALINK_MODEM_RST               14   
      #define LORALINK_MODEM_DI0               -1
      #define LORALINK_MODEM_BUSY              26
    #endif
  #endif




  #ifdef LORALINK_HARDWARE_ESP32_NOMODEM
  
    //includes
    //////////
    #include "Arduino.h"
    #include <driver/adc.h>
    #include <esp_task_wdt.h>
    #include <SPI.h>
    #include <SX127XLT.h>
    #if LORALINK_HARDWARE_WIFI == 1
      #include <DNSServer.h>
      #include <EasyDDNS.h>
    #endif
    #if LORALINK_HARDWARE_SDCARD == 1
      #include <SD.h>
    #endif
    #if LORALINK_HARDWARE_SPIFFS == 1
      #include <SPIFFS.h>  
    #endif
    
    //general hardware options
    //////////////////////////        
    #define LORALINK_MODEM_SX127x
    //#define LORALINK_MODEM_SX126x
    


    #define LORALINK_HARDWARE_NAME  F("ESP32 WiFi LoRa (SX1278) 433MHz")
    #define LORALINK_FIRMWARE_FILE  "/ESP.bin"
    #define LORALINK_HARDWARE_LORA  0       //device has no lora modem

    //utility defines
    #define ResetWatchDog           esp_task_wdt_reset

    //includes
    //////////
    #if LORALINK_HARDWARE_GPS == 1
      #include <TinyGPS.h>
      
      #define GPS_RX_PIN                      34
      #define GPS_TX_PIN                      12
    #endif

    //sdcard config
    #if LORALINK_HARDWARE_SDCARD == 1
      #define LORALINK_HARDWARE_SDCARD_MISO 22
      #define LORALINK_HARDWARE_SDCARD_MOSI 21
      #define LORALINK_HARDWARE_SDCARD_SCK  17
      #define LORALINK_HARDWARE_SDCARD_CS   13
    #endif

    #define USER_BUTTON                      38
    #define LORALINK_HARDWARE_STATUS_LED_PIN 32

    #if LORALINK_HARDWARE_LED == 1
      #define LORALINK_HARDWARE_TX_LED_PIN 33
    #endif
  
    #if LORALINK_HARDWARE_BATSENSE == 1
      #define LORALINK_HARDWARE_BATSENSE_PIN 0
    #endif
  

    #if LORALINK_HARDWARE_OLED == 1
      #include <Wire.h>
      #include <Adafruit_GFX.h>
      #include <Adafruit_SSD1306.h>

      #define SCREEN_WIDTH 128 // OLED display width, in pixels
      #define SCREEN_HEIGHT 64 // OLED display height, in pixels
      #define LORALINK_HARDWARE_OLED_RESET      16 // Reset pin # (or -1 if sharing Arduino reset pin)
      #define LORALINK_HARDWARE_SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
      #define LORALINK_HARDWARE_OLED_SDA        4
      #define LORALINK_HARDWARE_OLED_SCL        15
      extern Adafruit_SSD1306 g_display;
    #endif       

    #if LORALINK_HARDWARE_LORA == 1
      //LoRa Modem defines
      #define LORALINK_MODEM_TYPE              DEVICE_SX1278
      #define LORALINK_MODEM_SCK               5        
      #define LORALINK_MODEM_MISO              19       
      #define LORALINK_MODEM_MOSI              27       
      #define LORALINK_MODEM_SS                18         
      #define LORALINK_MODEM_RST               14   
      #define LORALINK_MODEM_DI0               -1
      #define LORALINK_MODEM_BUSY              26
    #endif
  #endif






  #ifdef LORALINK_HARDWARE_TBEAM
  
    //includes
    //////////
    #include "Arduino.h"
    #include <driver/adc.h>
    #include <esp_task_wdt.h>
    #include <SPI.h>
    #include <SX127XLT.h>
    #if LORALINK_HARDWARE_WIFI == 1
      #include <DNSServer.h>
      #include <EasyDDNS.h>
    #endif
    #if LORALINK_HARDWARE_SDCARD == 1
      #include <SD.h>
    #endif
    #if LORALINK_HARDWARE_SPIFFS == 1
      #include <SPIFFS.h>  
    #endif
    #include <axp20x.h>             //https://github.com/lewisxhe/AXP202X_Library


    
    //general hardware options
    //////////////////////////
    #define LORALINK_MODEM_SX127x
    //#define LORALINK_MODEM_SX126x
    


    #define LORALINK_HARDWARE_NAME  F("T-Beam WiFi LoRa (SX1276) 433MHz")
    #define LORALINK_FIRMWARE_FILE  "/TBeam.bin"
    #define LORALINK_HARDWARE_LORA  1       //device has a lora modem

    
    //utility defines
    #define ResetWatchDog           esp_task_wdt_reset


    //variables
    ///////////
    extern AXP20X_Class *g_pAxp;

    //includes
    //////////
    #if LORALINK_HARDWARE_GPS == 1
      #include <TinyGPS.h>
      
      #define GPS_RX_PIN                      34
      #define GPS_TX_PIN                      12
    #endif

    //sdcard config
    #if LORALINK_HARDWARE_SDCARD == 1
      #define LORALINK_HARDWARE_SDCARD_MISO 4
      #define LORALINK_HARDWARE_SDCARD_MOSI 25
      #define LORALINK_HARDWARE_SDCARD_SCK  13
      #define LORALINK_HARDWARE_SDCARD_CS   2
    #endif



    //defines
    /////////
    #define PMU_IRQ_PIN                       35
    #define USER_BUTTON                       38
    #define LORALINK_HARDWARE_STATUS_LED_PIN  LED_BUILTIN

    #if LORALINK_HARDWARE_LED == 1
      #define LORALINK_HARDWARE_TX_LED_PIN 33
    #endif
  
    
    #if LORALINK_HARDWARE_OLED == 1
      #include <Wire.h>
      #include <Adafruit_GFX.h>
      #include <Adafruit_SSD1306.h>

      #define SCREEN_WIDTH 128 // OLED display width, in pixels
      #define SCREEN_HEIGHT 64 // OLED display height, in pixels
      #define LORALINK_HARDWARE_OLED_RESET      -1   // Reset pin # (or -1 if sharing Arduino reset pin)
      #define LORALINK_HARDWARE_SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
      #define LORALINK_HARDWARE_OLED_SDA        SDA
      #define LORALINK_HARDWARE_OLED_SCL        SCL
      extern Adafruit_SSD1306                   g_display;
    #endif       

    

    
    #if LORALINK_HARDWARE_LORA == 1
      //LoRa Modem defines
      #define LORALINK_MODEM_TYPE              DEVICE_SX1278
      #define LORALINK_MODEM_SCK               LORA_SCK        
      #define LORALINK_MODEM_MISO              LORA_MISO       
      #define LORALINK_MODEM_MOSI              LORA_MOSI       
      #define LORALINK_MODEM_SS                LORA_CS         
      #define LORALINK_MODEM_RST               LORA_RST   
      #define LORALINK_MODEM_DI0               -1
      #define LORALINK_MODEM_BUSY              LORA_IRQ
    #endif
  #endif

#endif
