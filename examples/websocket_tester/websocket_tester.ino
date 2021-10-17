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

#include <H4.h>
#include<H4AsyncWebServer.h>
H4 h4(115200);

#define TICK  1000
#define DEVICE "WEBSOCKET_TEST"

H4_TIMER  ticker;

H4AsyncWebServer s(80);
H4AT_HTTPHandlerWS* _ws=nullptr;
#define LIB "H4AsyncTCP"

void h4setup(){
  HAL_FS.begin(); // ESP32 cannot call this from constructor, so... mimic H4Plugins :)  
  WiFi.mode(WIFI_STA);
  WiFi.begin("XXXXXXXX", "XXXXXXXX");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("\nIP: %s\n",WiFi.localIP().toString().c_str());

  _ws=new H4AT_HTTPHandlerWS("/ws");
  _ws->onOpen([](H4AS_HTTPRequest* skt){
    h4.queueFunction([skt,_ws]{ 
      skt->sendText("lib,%s",LIB);
      skt->sendText("board,%s",MY_BOARD);
      skt->sendText("default,You are No. %d",_ws->size());
    });
    ticker=h4.every(TICK,[=]{
      _ws->broadcastText("millis,%u",millis());
      _ws->broadcastText("heap,%u",_HAL_freeHeap());
      _ws->broadcastText("mhb,%u",_HAL_maxHeapBlock());
      _ws->broadcastText("rnd1,%u",random(0,1000));
      _ws->broadcastText("rnd2,%u",random(0,2000));
      _ws->broadcastText("rnd3,%u",random(0,3000));
      _ws->broadcastText("rnd4,%u",random(0,4000));
      _ws->broadcastText("rnd5,%u",random(0,5000));
      _ws->broadcastText("rnd6,%u",random(0,6000));
      _ws->broadcastText("n,%d",_ws->size());
    });
  });
  
  _ws->onTextMessage([](H4AS_HTTPRequest* skt,const std::string& msg){
    // normally you would parse the message and probably do a big
    // switch statement to perform a different function for each different message type
    // In this simple example, we only have the one type
    if(msg=="mark") skt->sendText("rnd1,#0a0");
    else Serial.printf("We got us a client! %p\n",skt);
  });
  
  _ws->onClose([](H4AS_HTTPRequest* skt){ if(!_ws->size()) h4.cancel(ticker); });

  s.on("/",HTTP_GET,[](H4AT_HTTPHandler* h){ h->sendFile("ws.htm"); });   
  s.addHandler(_ws);  
  s.begin();
}