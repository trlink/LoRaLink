#ifndef CSPIFFSUTILS
#define CSPIFFSUTILS


//includes
//////////
#include <Arduino.h>
#include "HardwareConfig.h"
#include <FS.h>



class CFileUtils 
{

  public:
  
    static int listDir(fs::FS &fs, const char * dirname, uint8_t levels)
    {
      //variables
      ///////////
      int nFileCount = 0;

      
      Serial.print(F("Listing directory: "));
      Serial.println(dirname);
  
      File root = fs.open(dirname);
      
      if(!root)
      {
          Serial.println(" - failed to open directory");
          return -1;
      };
      
      if(!root.isDirectory())
      {
          Serial.println(" - not a directory");
          return -1;
      };
  
      File file = root.openNextFile();
      
      while(file)
      {
        if(file.isDirectory())
        {
          Serial.print(F("  DIR : "));
          Serial.println(file.name());
          
          if(levels > 0)
          {
            nFileCount = listDir(fs, file.name(), levels - 1);
          };
        } 
        else 
        {
          Serial.print(F("  FILE: "));
          Serial.print(file.name());
          Serial.print(F("\tSIZE: "));
          Serial.println(file.size());

          nFileCount += 1;
        };
        
        file = root.openNextFile();
      };

      return nFileCount;
    };



    static int GetFileNameByIndex(fs::FS &fs, const char *dirname, int nIndex, char *szFile)
    {
      //variables
      ///////////
      int nFileCount = 0;

      File root = fs.open(dirname);
      
      if(!root)
      {
          return -1;
      };
      
      if(!root.isDirectory())
      {
          return -1;
      };
  
      File file = root.openNextFile();
      
      while(file)
      {
        if(file.isDirectory() == true)
        {
          return -1;
        } 
        else 
        {
          if(nIndex == nFileCount)
          {
            strcpy(szFile, file.name());
            return nFileCount; 
          };

          nFileCount += 1;
        };
        
        file = root.openNextFile();
      };

      return -1;
    };



    static long GetFileSize(fs::FS &fs, const char *szFile)
    {
      //variables
      ///////////
      File file = fs.open(szFile);
      long lSize = 0;
      

      if(!file) 
      {
        Serial.println(F("GetFileSize() - failed to open file for reading"));
        
        return -1;
      }
      else 
      {
        lSize = file.size();
        file.close();
      };

      Serial.print(F("GetFileSize() - return: "));
      Serial.println(lSize);
      
      return lSize;
    };


    //this method reads a file byte by byte into pData
    //pData must be great enough to hold the file content
    static int ReadFile(fs::FS &fs, const char* szFile, byte *pData) 
    {
      //variables
      ///////////
      File file;
      int  nBytes = 0;
      
      Serial.print(F("Reading file: "));
      Serial.println(szFile);
  
      file = fs.open(szFile);
      
      if(!file)
      {
        Serial.println(F("readFile() - failed to open file for reading"));
        return -1;
      };

      if(file.isDirectory() == true)
      {
        Serial.println(F("readFile() - failed to open file for reading (is dir)"));
        return -1;        
      };
  
      while(file.available() > 0) 
      {
        Serial.print(pData[nBytes]);
        nBytes += 1;
      };

      Serial.print(F("readFile() - read from file returned "));
      Serial.print(nBytes);
      Serial.println(F(" bytes"));
  
      return nBytes;
    };
    

    static String ReadFile(fs::FS &fs, const char* szFile) 
    {
      //variables
      ///////////
      long lSize = 0;
      byte *pData;
      String strData = "";

      lSize = CFileUtils::GetFileSize(fs, szFile);

      if(lSize > 0) 
      {
        pData = new byte[lSize + 1];

        if(CFileUtils::ReadFile(fs, szFile, pData) > 0)
        {
          pData[lSize] = 0;
  
          strData = String((char*)pData);
       };
             
        delete pData;
      };

      return strData;
    };


    static bool WriteFile(fs::FS &fs, const char *szFile, byte *pData, int nLength) 
    {
      Serial.print(F("Writing file: "));
      Serial.print(szFile);

      if(nLength > 0)
      {
        File file = fs.open(szFile, FILE_WRITE);
        
        if(!file)
        {
            Serial.println(F(" - failed to open file for writing"));
            return false;
        }
        
        if(file.write(pData, nLength))
        {
            Serial.println(F(" - file written"));
  
            return true;
        } 
        else 
        {
            Serial.println(F(" - write failed"));
  
            return false;
        };
      }
      else
      {
        Serial.println(F(" size 0 - write failed"));
  
        return false;
      };
    };


    static bool WriteFile(fs::FS &fs, const char * szFile, const char * message)
    {
      return CFileUtils::WriteFile(fs, szFile, (unsigned char*)message, strlen(message));
    };



    void appendFile(fs::FS &fs, const char * path, const char * message)
    {
        Serial.printf("Appending to file: %s\r\n", path);
    
        File file = fs.open(path, FILE_APPEND);
        
        if(!file)
        {
            Serial.println(F("- failed to open file for appending"));
            return;
        }
        if(file.print(message))
        {
            Serial.println(F("- message appended"));
        } 
        else 
        {
            Serial.println(F("- append failed"));
        }
    }
    
    void renameFile(fs::FS &fs, const char * path1, const char * path2)
    {
        Serial.printf("Renaming file %s to %s\r\n", path1, path2);
        
        if (fs.rename(path1, path2)) {
            Serial.println(F("- file renamed"));
        } else {
            Serial.println(F("- rename failed"));
        }
    }
    
    static void deleteFile(fs::FS &fs, const char * path)
    {
      Serial.print(F("Deleting file: "));
      Serial.print(path);
      
      if(fs.remove(path))
      {
          Serial.println(F("- file deleted"));
      }
      else 
      {
          Serial.println(F("- delete failed"));
      };
    };

    
};


#endif
