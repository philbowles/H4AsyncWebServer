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
  H4T_FN_LOOKUP lookup=[](const std::string& n){ return replacers.count(n) ? replacers[n]:"%"+n+"%"; };
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
  _evts->onConnect([=](size_t nClients){
#endif
      Serial.printf("_evts->onConnect N=%d\n",nClients);
      if(nClients){
        // do setup stuff, start timers etc
        h4.every(500,[=]{
          static bool led=false;
          _evts->send(stringFromInt(millis()).data(),"millis");
          _evts->send(stringFromInt(_HAL_freeHeap()).data(),"heap");
          _evts->send(stringFromInt(_HAL_maxHeapBlock()).data(),"maxblock");
          _evts->send(stringFromInt(nrcx).data(),"nrcx");
          _evts->send(stringFromInt(led).data(),"LED");led=!led;
        },nullptr,UI_UPDATER,true);
        
        // send UI seup stuff
        // statics
        _evts->send("library,0,s,0,0,"LIB,"ui");
        _evts->send("device,0,s,0,0,"DEVICE,"ui");
        _evts->send((std::string("chip,0,s,0,0,")+_HAL_uniqueName("ESP_")).data(),"ui");
        _evts->send("board,0,s,0,0,"MY_BOARD,"ui");
        _evts->send("Son1,0,s,0,0,Huey","ui");
        _evts->send("Son2,0,s,0,0,Louie","ui");
        _evts->send("Son3,0,s,0,0,Dewey","ui");
        // dynamics
        _evts->send("millis,0,s,0,0,0","ui");
        _evts->send("heap,0,s,0,0,0","ui");
        _evts->send("maxblock,0,s,0,0,0","ui");          
        _evts->send("nrcx,0,s,0,0,0","ui");          
        _evts->send("id,0,s,0,0,0","ui"); 
        _evts->send("LED,1,g,0,5,0","ui");   
      }
      else { // client == 0 means all clients gone away
        Serial.printf("No more clients, stop some stuff!\n");
        h4.cancelSingleton(UI_UPDATER);
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

  
  s.on("/",HTTP_GET,[](H4AT_HTTPHandler* h){
    h->sendFileParams("/sta.htm",lookup); // refac
  });   
  s.on("/upnp",HTTP_POST,[](H4AT_HTTPHandler* h){
    h->sendstring("text/plain","gotcha");
  });  
  s.addHandler(_evts);
  
  rqs.begin();
  echo.begin();
#endif
  s.onError([](int e,int i){
    Serial.printf("USER: SRV ERROR %d[%s] info=%d\n",e,H4AsyncClient::errorstring(e).data(),i);
  });
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
    //if(_evts) _evts->send(wisdom[random(0,wisdom.size())].data());
  });
}