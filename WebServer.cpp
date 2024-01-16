//includes
//////////
#include "WebServer.h"
#include "LoRaLinkConfig.h"
#include "HardwareConfig.h"
#include <memory>
#include "helper.h"
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>

 

//globals
/////////
HTTPServer              *g_insecureServer   = NULL;
WebServerOnPostRequest   g_apiCallbackHandler;


//function predecl
//////////////////
void handleRoot(HTTPRequest * req, HTTPResponse * res);
void handle404(HTTPRequest * req, HTTPResponse * res);
void handleFile(HTTPRequest * req, HTTPResponse * res);
void handleAPI(HTTPRequest * req, HTTPResponse * res);
void handleRedirect(HTTPRequest * req, HTTPResponse * res);
void handleUpload(HTTPRequest * req, HTTPResponse * res);
String getContentType(String strFile);


void StartWebservers(WebServerOnPostRequest apiHandler)
{
  g_apiCallbackHandler = apiHandler;

  
  // Additionally, we create an HTTPServer for unencrypted traffic
  g_insecureServer = new HTTPServer(80, MAX_WEBSERVER_CONNECTIONS, 0);

  //setup handler
  ResourceNode *nodeRoot      = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode *node404       = new ResourceNode("", "", &handleFile);
  ResourceNode *nodeUpload    = new ResourceNode("/upload", "POST", &handleUpload);
  ResourceNode *nodeAPI       = new ResourceNode("/api/api.json", "POST", &handleAPI);

  
  g_insecureServer->registerNode(nodeAPI);
  g_insecureServer->registerNode(nodeRoot);
  g_insecureServer->registerNode(nodeUpload);
  g_insecureServer->setDefaultNode(node404);
  
  g_insecureServer->start();
};




void HandleWebserver()
{  
  g_insecureServer->loop();
};



void handleUpload(HTTPRequest * req, HTTPResponse * res) 
{
  //variables
  ///////////
  HTTPBodyParser  *parser;
  std::string     contentType = req->getHeader("Content-Type");
  bool            didwrite = false;
  auto            params = req->getParams();
  std::string     folder;
  std::string     filename;
  std::string     mimeType;
  std::string     name;
  size_t          semicolonPos;

  #ifdef WEBSERVERDEBUG
    Serial.println(F("--> handleFormUpload()"));
  #endif

  //get folder from query string
  params->getQueryParameter("folder", folder);

  if(folder.length() <= 0)
  {
    #ifdef WEBSERVERDEBUG
      Serial.println(F("Missing folder"));
    #endif
    
    folder="/";
  };
  
  // The content type may have additional properties after a semicolon, for exampel:
  // Content-Type: text/html;charset=utf-8
  // Content-Type: multipart/form-data;boundary=------s0m3w31rdch4r4c73rs
  // As we're interested only in the actual mime _type_, we strip everything after the
  // first semicolon, if one exists:
  semicolonPos = contentType.find(";");
  
  if(semicolonPos != std::string::npos) 
  {
    contentType = contentType.substr(0, semicolonPos);
  };

  // Now, we can decide based on the content type:
  if (contentType == "multipart/form-data") 
  {
    parser = new HTTPMultipartBodyParser(req);
  } 
  else 
  {
    //Serial.printf("Unknown POST Content-Type: %s\n", contentType.c_str());
    return;
  };

  while((parser->nextField() == true) && (parser->endOfField() == false)) 
  {
    // For Multipart data, each field has three properties:
    // The name ("name" value of the <input> tag)
    // The filename (If it was a <input type="file">, this is the filename on the machine of the
    //   user uploading it)
    // The mime type (It is determined by the client. So do not trust this value and blindly start
    //   parsing files only if the type matches)
    name = parser->getFieldName();
    
    // Double check that it is what we expect
    if(name == "file") 
    {
      filename = folder + "/" + parser->getFieldFilename();
      mimeType = parser->getFieldMimeType();

      #ifdef WEBSERVERDEBUG
        // We log all three values, so that you can observe the upload on the serial monitor:
        Serial.printf("handleFormUpload: field name='%s', filename='%s', mimetype='%s'\n", name.c_str(), filename.c_str(), mimeType.c_str());
      #endif

      if(filename.length() > 0)
      {
        LORALINK_WEBAPP_FS.remove(filename.c_str());
        
        // Create a new file on spiffs to stream the data into
        File file = LORALINK_WEBAPP_FS.open(filename.c_str(), "w");
        byte *buf = new byte[MAX_FILE_RESP_BUFF_SIZE + 1];
        size_t readLength;

        if(file)
        {
          didwrite = true;
          
          // With endOfField you can check whether the end of field has been reached or if there's
          // still data pending. With multipart bodies, you cannot know the field size in advance.
          while (!parser->endOfField()) 
          {
            readLength = parser->read(buf, MAX_FILE_RESP_BUFF_SIZE);
    
            if(readLength > 0)
            {
              #ifdef WEBSERVERDEBUGX
                // We log all three values, so that you can observe the upload on the serial monitor:
                Serial.printf("handleFormUpload: filename='%s', write %i bytes\n", filename.c_str(), readLength);
              #endif
              
              file.write(buf, readLength);
            };
          };
          
          file.close();
    
          res->setStatusCode(200);
        }
        else
        {
          #ifdef WEBSERVERDEBUG
            Serial.println(F("Unable to open upload file..."));
          #endif
        };
  
        delete buf;
  
        break;
      };
    };
  };

  if (!didwrite) {
    res->setStatusCode(500);
  };
  
  //res->println("</body></html>");
  delete parser;

  #ifdef WEBSERVERDEBUG
    // We log all three values, so that you can observe the upload on the serial monitor:
    Serial.println(F("<-- handleFormUpload()"));
  #endif
};




void handleFile(HTTPRequest * req, HTTPResponse * res) 
{
  //variables
  ///////////
  File file;
  byte *pData = new byte[MAX_FILE_RESP_BUFF_SIZE];
  int  nLen = 0;
  String strFile = req->getRequestString().c_str();

  //remove query string from filename
  if(strFile.indexOf("?") > 0)
  {
    strFile = strFile.substring(0, strFile.indexOf("?"));
  };

  //replace blanks in files&folders (uri decode)
  strFile.replace("%20", " ");
  
  #ifdef WEBSERVERDEBUG
    Serial.print(F("Static file handler: "));
    Serial.println((char*)strFile.c_str());
  #endif

  if(strFile.length() > 0)
  {
    if(LORALINK_WEBAPP_FS.exists((char*)strFile.c_str()) == true)
    {
      file = LORALINK_WEBAPP_FS.open((char*)strFile.c_str(), "r");
  
      if(file)
      {
        res->setHeader("Content-Type", getContentType((char*)strFile.c_str()).c_str());
        res->setStatusCode(200);
        
        do
        {
          memset(pData, 0, MAX_FILE_RESP_BUFF_SIZE);
          nLen = file.read(pData, MAX_FILE_RESP_BUFF_SIZE - 1);

          if(nLen > 0)
          {
            res->write(pData, nLen);
          };
  
          #ifdef WEBSERVERDEBUGX 
            Serial.print(F("Static file handler: send: "));
            Serial.println(nLen);
          #endif
          
          ResetWatchDog();
        } while(nLen == MAX_FILE_RESP_BUFF_SIZE - 1);
    
        file.close();
      }
      else
      {
        file.close();
        
        handle404(req, res);
      };
    }
    else
    {
      handle404(req, res);
    };
  }
  else
  {
    handle404(req, res);
  };

  delete pData;
};


String getContentType(String strFile)
{
  if (strFile.endsWith(".html")) return String(F("text/html"));
  else if (strFile.endsWith(".dat")) return String(F("application/x-ns-proxy-autoconfig"));
  else if (strFile.endsWith(".htm")) return String(F("text/html"));
  else if (strFile.endsWith(".css")) return String(F("text/css"));
  else if (strFile.endsWith(".json")) return String(F("application/json"));
  else if (strFile.endsWith(".js")) return String(F("application/javascript"));
  else if (strFile.endsWith(".png")) return String(F("image/png"));
  else if (strFile.endsWith(".gif")) return String(F("image/gif"));
  else if (strFile.endsWith(".jpg")) return String(F("image/jpeg"));
  else if (strFile.endsWith(".ico")) return String(F("image/x-icon"));
  else if (strFile.endsWith(".svg")) return String(F("image/svg+xml"));
  else if (strFile.endsWith(".eot")) return String(F("font/eot"));
  else if (strFile.endsWith(".woff")) return String(F("font/woff"));
  else if (strFile.endsWith(".woff2")) return String(F("font/woff2"));
  else if (strFile.endsWith(".ttf")) return String(F("font/ttf"));
  else if (strFile.endsWith(".xml")) return String(F("text/xml"));
  else if (strFile.endsWith(".pdf")) return String(F("application/pdf"));
  else if (strFile.endsWith(".zip")) return String(F("application/zip"));
  else if(strFile.endsWith(".gz")) return String(F("application/x-gzip"));
  else return String(F("text/plain"));
};


void handleRedirect(HTTPRequest * req, HTTPResponse * res)
{
  res->setHeader("Content-Type", "text/html");
  res->setStatusCode(302);
  res->setHeader("Location", "/index.html");
};


void handleRoot(HTTPRequest * req, HTTPResponse * res) 
{
  handleRedirect(req, res);
};


void sendStringResponse(HTTPResponse *res, int nResult, char *szResponseType, char *szText)
{
  #ifdef WEBSERVERDEBUG
    Serial.print(F("sendStringResponse: "));
    Serial.println(szText);
  #endif
  
  res->setStatusCode(nResult);
  res->setHeader("Content-Type", szResponseType);
  res->print(szText);
};



void sendJsonResponse(HTTPResponse *res, int nResult, DynamicJsonDocument &pJsonResponse)
{
  //variables
  ///////////
  char *szJson = new char[pJsonResponse.capacity() + 1];

  // Write JSON document
  memset(szJson, 0, pJsonResponse.capacity());
  serializeJson(pJsonResponse, szJson, pJsonResponse.capacity());

  #ifdef WEBSERVERDEBUG
    Serial.print(F("sendJsonResponse: size: "));
    Serial.print(pJsonResponse.capacity());
    Serial.print(F(" response: "));
    Serial.println(szJson);
  #endif

  sendStringResponse(res, nResult, "application/json", szJson);
  
  delete szJson;
};





void handleAPI(HTTPRequest * req, HTTPResponse * res)
{
  //variables
  ///////////
  int   nLen        = req->getContentLength();
  char *szContent   = new char[nLen + 2];
  long  lTimeout    = millis() + 5000;
  int   idx         = 0;

  memset(szContent, 0, nLen + 1);
  
  while((req->requestComplete() == false) && (lTimeout > millis()) && (idx < nLen))
  {
    if(idx < nLen)
    {
      idx += req->readChars(szContent + idx, nLen - idx);
      
      delay(100);
    }
    else 
    {
      break;  
    };
    
    ResetWatchDog();
  };

  if(req->requestComplete() == true)
  {
    #ifdef WEBSERVERDEBUG 
      Serial.print(F("handleAPI: call cb handler: "));
      Serial.println(szContent);
    #endif
    
    g_apiCallbackHandler(req, res, szContent, nLen);
  }
  else
  {
    #ifdef WEBSERVERDEBUG 
      Serial.println(F("handleAPI: request incomplete after timeout"));
    #endif

    res->setStatusCode(413);
    res->setStatusText("Request entity too large");
  };

  delete szContent;
};



void handle404(HTTPRequest * req, HTTPResponse * res) 
{
  #ifdef WEBSERVERDEBUG 
    Serial.print(F("404 file handler: "));
    Serial.println((char*)req->getRequestString().c_str());
  #endif

  
  req->discardRequestBody();
  res->setStatusCode(404);
  res->setStatusText("Not Found");
  res->setHeader("Content-Type", "text/html");
};
