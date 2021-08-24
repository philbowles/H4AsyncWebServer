/*
Creative Commons: Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

You are free to:

Share — copy and redistribute the material in any medium or format
Adapt — remix, transform, and build upon the material

The licensor cannot revoke these freedoms as long as you follow the license terms. Under the following terms:

Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

NonCommercial — You may not use the material for commercial purposes.

ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions 
under the same license as the original.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others 
from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use is 
permitted by an applicable exception or limitation. To discuss an exception, contact the author:

philbowles2012@gmail.com

No warranties are given. The license may not give you all of the permissions necessary for your intended use. 
For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.
*/
#ifdef ARDUINO_ARCH_ESP8266
  #include<ESP8266WiFi.h>
  #include<LittleFS.h>
  #define HAL_FS LittleFS
  #define MY_BOARD "Wemos D1 Mini"
#else
  #include<WiFi.h>
  #include<FS.h>
  #include<SPIFFS.h>
  #define HAL_FS SPIFFS
  #define MY_BOARD "ESP32 Dev Board"
#endif

#define USE_ESPASWS  0

#include <H4.h>
#include <H4Tools.h>
H4 h4(115200);

#define DEVICE "H4AT_VS ESP"
  
#if USE_ESPASWS
  #include<ESPAsyncWebServer.h>
  AsyncWebServer s(80);
  AsyncWebServer s2(8080);
  AsyncEventSource* _evts=nullptr;
  #define LIB "ESPAsyncWebServer"
  String replacers(const String& var){
    return var=="device" ? DEVICE:"????";
  }
#else
  #include<H4AsyncWebServer.h>
  #include<EchoServer.h>
  #include<RandomQuoteServer.h>
  H4AsyncWebServer s(80);
  EchoServer echo(8080);
  RandomQuoteServer rqs(8888);
  H4AT_HTTPHandlerSSE* _evts=nullptr;
  #define LIB "H4AsyncTCP"
  H4AT_NVP_MAP replacers={
      {"device",DEVICE}
  };
#endif

static int nrcx=0;

#define UI_UPDATER 42

void h4setup(){
  Serial.printf("MIMIC H4P to get FS started on pesky ESP!\n");
  HAL_FS.begin(); // ESP32 cannot call this from constructor, so... mimic H4Plugins :)  

  WiFi.mode(WIFI_STA);
  WiFi.begin("XXXXXXXX", "XXXXXXXX");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("\nIP: %s\n",WiFi.localIP().toString().c_str());

  Serial.printf("Cry 'Havoc!' and let receive the packets of War!\n\n");

#if USE_ESPASWS 
  _evts=new AsyncEventSource("/evt"); 
  _evts->onConnect([=](AsyncEventSourceClient *client){
#else
  _evts=new H4AT_HTTPHandlerSSE("/evt");//,10); // backlog = 10
  _evts->onConnect([=](H4AT_SSEClient* client){
#endif
      Serial.printf("_evts->onConnect client=0x%08x N=%d\n",client,_evts->count());
      if(client){
        // do setup stuff, start timers etc
        h4.every(1000,[=]{
          static size_t _nextID=0;
          static bool led=false;
          _evts->send(stringFromInt(millis()).data(),"millis");
          _evts->send(stringFromInt(_HAL_freeHeap()).data(),"heap");
          _evts->send(stringFromInt(_HAL_maxHeapBlock()).data(),"maxblock");
          _evts->send(stringFromInt(nrcx).data(),"nrcx");
          _evts->send(stringFromInt(led).data(),"LED");led=!led;
          #if USE_ESPASWS
          _evts->send(stringFromInt(_nextID++).data(),"id");
          #else
          _evts->send(stringFromInt(_evts->_nextID).data(),"id");
          #endif
        },nullptr,UI_UPDATER,true);
        
        if(!client->lastId()){ // first-timer
          //Serial.printf("Newbie: set up his UI\n",client->lastID);

          // send UI seup stuff
          // statics
          client->send("library,0,s,0,0,"LIB,"ui");
          client->send("device,0,s,0,0,"DEVICE,"ui");
          client->send((std::string("chip,0,s,0,0,")+_HAL_uniqueName("ESP_")).data(),"ui");
          client->send("board,0,s,0,0,"MY_BOARD,"ui");
          client->send("Son1,0,s,0,0,Huey","ui");
          client->send("Son2,0,s,0,0,Louie","ui");
          client->send("Son3,0,s,0,0,Dewey","ui");
          // dynamics
          client->send("millis,0,s,0,0,0","ui");
          client->send("heap,0,s,0,0,0","ui");
          client->send("maxblock,0,s,0,0,0","ui");          
          client->send("nrcx,0,s,0,0,0","ui");          
          client->send("id,0,s,0,0,0","ui"); 
          client->send("LED,1,g,0,5,0","ui");   
        } 
        else {
          Serial.printf("Its a reconnect from ID %d\n",client->lastId());
          nrcx++;
        }
      }
      else { // client == 0 means all clients gone away
        if(_evts->count()) Serial.printf("SANITY CHECK FAIL: nClients non-zero!\n");
        else {
          Serial.printf("No more clients, stop some stuff!\n");
          h4.cancelSingleton(UI_UPDATER);
        }
      }
  });

#if USE_ESPASWS
  s.on("/",HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(HAL_FS, "/sta.htm", String(), false, replacers);
  });
  s.addHandler(_evts);
  s.serveStatic("/", HAL_FS, "/").setCacheControl("max-age=31536000"); // not required in H4
  // ASWS also requires explicit 404 handler
#else
  rqs.begin();
  echo.begin();
  
  s.on("/",HTTP_GET,[](H4AT_HTTPHandler* h){
    h->sendFileParams("/sta.htm",replacers);
  });   
  s.on("/upnp",HTTP_POST,[](H4AT_HTTPHandler* h){
    h->sendstring("text/plain","gotcha");
  });  
  s.addHandler(_evts);

#endif
  Serial.printf("START SERVER 1\n");
  s.begin();

/*
  h4.every(50,[=]{
    static size_t fast=0;
    if(_evts) {
     _evts->send(stringFromInt(fast++).data(),"fast");
    }
  });
*/
  int tick=30000;
  h4.every(tick,[=]{ 
    Serial.printf("FH=%u tick %d\n",_HAL_freeHeap(),tick);
    if(_evts) _evts->send(wisdom[random(0,wisdom.size())].data());
  });
}