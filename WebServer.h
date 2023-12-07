#ifndef __WEBSERVER__
#define __WEBSERVER__


//includes
//////////
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <HTTPSServer.hpp>

using namespace httpsserver;



//to get the webserver running
//#include <esp32/sha.h> in HTTPConnection.hpp
//https://github.com/hardkernel/ODROID-GO/issues/24
//use the lib from the github, remove logging from the websocket handler 





//defines
/////////
#define WEBSERVERDEBUG
//#define WEBSERVERDEBUGX
#define MAX_FILE_RESP_BUFF_SIZE       2048

typedef void(*WebServerOnPostRequest)(void *req, void *res, char *pData, int nDataLength);

void StartWebservers(WebServerOnPostRequest apiHandler);

void HandleWebserver();


void sendStringResponse(HTTPResponse *res, int nResult, char *szResponseType, char *szText);
void sendJsonResponse(HTTPResponse *res, int nResult, DynamicJsonDocument &pJsonResponse);


#endif
