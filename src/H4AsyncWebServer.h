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
#pragma once

#include<H4AsyncTCP.h>
#include"h4asws_config.h"

#if H4AS_DEBUG
    template<int I, typename... Args>
    void H4AS_PRINT(const char* fmt, Args... args) {
        #ifdef ARDUINO_ARCH_ESP32
        if (H4AS_DEBUG >= I) Serial.printf(std::string(std::string("H4AS:%d: H=%u M=%u S=%u ")+fmt).c_str(),I,_HAL_freeHeap(),_HAL_maxHeapBlock(),uxTaskGetStackHighWaterMark(NULL),args...);
        #else
        if (H4AS_DEBUG >= I) Serial.printf(std::string(std::string("H4AS:%d: H=%u M=%u ")+fmt).c_str(),I,_HAL_freeHeap(),_HAL_maxHeapBlock(),args...);
        #endif
    }
    #define H4AS_PRINT1(...) H4AS_PRINT<1>(__VA_ARGS__)
    #define H4AS_PRINT2(...) H4AS_PRINT<2>(__VA_ARGS__)
    #define H4AS_PRINT3(...) H4AS_PRINT<3>(__VA_ARGS__)
    #define H4AS_PRINT4(...) H4AS_PRINT<4>(__VA_ARGS__)

    template<int I>
    void H4AS_dump(const uint8_t* p, size_t len) { if (H4AS_DEBUG >= I) dumphex(p,len); }
    #define H4AS_DUMP1(p,l) H4AS_dump<1>((p),l)
    #define H4AS_DUMP2(p,l) H4AS_dump<2>((p),l)
    #define H4AS_DUMP3(p,l) H4AS_dump<3>((p),l)
    #define H4AS_DUMP4(p,l) H4AS_dump<4>((p),l)
#else
    #define H4AS_PRINT1(...)
    #define H4AS_PRINT2(...)
    #define H4AS_PRINT3(...)
    #define H4AS_PRINT4(...)

    #define H4AS_DUMP2(...)
    #define H4AS_DUMP3(...)
    #define H4AS_DUMP4(...)
#endif

enum {
    HTTP_ANY,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_CONNECT,
    HTTP_OPTIONS,
    HTTP_TRACE,
    HTTP_PATCH
};

enum {
    WS_TEXT=1,
    WS_BINARY,
    WS_CLOSE=8,
    WS_PING
};

class H4AsyncWebServer;
class H4AS_HTTPRequest: public H4AsyncClient {
    public:
                uint8_t*        _body=nullptr;
                size_t          _blen=0;
                H4T_NVP_MAP     params;
                std::string     url;

        H4AS_HTTPRequest(tcp_pcb* p): H4AsyncClient(p){ H4AS_PRINT1("H4AS_HTTPRequest CTOR %p\n",this);}
        virtual ~H4AS_HTTPRequest(){ 
            H4AS_PRINT1("H4AS_HTTPRequest DTOR %p body=%p\n",this,_body);
            if(_body){
                H4AS_PRINT1("H4AS_HTTPRequest DTOR %p body=%p\n",this,_body);
                free(_body);
            }
        }
// for sockets
                void            sendBinary(const uint8_t* data,size_t len);
                template<typename... Args>
                void            sendText(const char*,Args&&...);
                void            sendText(const std::string& s);
// don't call
        static  std::string     _getHeader(H4T_NVP_MAP& m,const char* h);
                void            _paramsFromstring(const std::string& bod);
};
//
//   H4AT_HTTPHandler + derivatives
//
class H4AT_HTTPHandler;
using H4AS_RQ_HANDLER    =std::function<void(H4AT_HTTPHandler*)>;

class H4AT_HTTPHandler {
    protected:
                H4AS_RQ_HANDLER     _f=nullptr;
        static  H4_INT_MAP          _responseCodes;

        virtual bool                _execute();
        virtual bool                _match(const std::string& verb,const std::string& path);

    public:
                H4T_NVP_MAP         _headers; // hoist!
                H4AsyncWebServer*   _srv;

                std::string         _verbName();
                int                 _verb;
                std::string         _path;
                H4AS_HTTPRequest*   _r=nullptr;
                H4T_NVP_MAP         _sniffHeader; // thinl: tidy??

        static  H4T_NVP_MAP         mimeTypes;

        H4AT_HTTPHandler(int verb,const std::string& path,H4AS_RQ_HANDLER f=nullptr): _verb(verb),_path(path),_f(f){
            H4AS_PRINT1("H4AT_HTTPHandler CTOR %p v=%d vn=%s p=%s\n",this,_verb,_verbName().data(),path.data());
        }
        virtual ~H4AT_HTTPHandler(){ 
            H4AS_PRINT1("H4AT_HTTPHandler DTOR %p\n",this);
            reset(); // prob don't need this
        }
                void                addHeader(const std::string& name,const std::string& value){ _headers[name]=value; }
                uint8_t*            bodyData(){ return _r->_body; }
                size_t              bodySize(){ return _r->_blen; } // tidy these
        static  std::string         mimeType(const char* fn);
                H4T_NVP_MAP&        params(){ return _r->params; }
                void                redirect(const char* fn);
        virtual void                reset();
        virtual void                send(uint16_t code,const std::string& type,size_t length=0,const void* _body=nullptr);
        virtual void                sendFile(const char* fn){ _serveFile(fn); }
        virtual void                sendFileParams(const char* fn,H4T_FN_LOOKUP f);
                void                sendOK(){ send(200,mimeType("txt"),0,nullptr); }
                // send404?
        virtual void                sendstring(const std::string& type,const std::string& data){ send(200,type,data.size(),(const void*) data.data()); }
                std::string         url(){ return _r->url; } // tidy these
//      don't call
                bool                _notFound();
        virtual void                _init(H4AsyncWebServer* srv){ _srv=srv; }
                bool                _select(H4AS_HTTPRequest* r,const std::string& verb,const std::string& path);
                bool                _serveFile(const char* fn);
};

class H4AT_HTTPHandlerFile: public H4AT_HTTPHandler {
    protected:
        virtual bool    _execute() override { return _serveFile(_path.data()); }
        virtual bool    _match(const std::string& verb,const std::string& path) override;
    public:
        H4AT_HTTPHandlerFile(): H4AT_HTTPHandler(HTTP_GET,"*"){};
        virtual void    reset() override { _path="*"; } // not REALLY needed
};

class H4AT_HTTPHandler404: public H4AT_HTTPHandler {
    protected:
        virtual bool    _execute() override { return _notFound(); }
        virtual bool    _match(const std::string& verb,const std::string& path) override { return true; }
    public:
        H4AT_HTTPHandler404(): H4AT_HTTPHandler(HTTP_ANY,"404"){}
};

using H4AS_WS_LIST       = std::unordered_set<H4AS_HTTPRequest*>;
using H4AS_FN_WSEVENT    = std::function<void(H4AS_HTTPRequest*)>;
using H4AS_FN_WSTXT      = std::function<void(H4AS_HTTPRequest*,const std::string&)>;
using H4AS_FN_WSBIN      = std::function<void(H4AS_HTTPRequest*,const uint8_t*,size_t)>;

class H4AT_HTTPHandlerWS: public H4AT_HTTPHandler {
                H4AS_WS_LIST        _clients;
                H4AS_FN_WSEVENT     _cbOpen=nullptr;
                H4AS_FN_WSEVENT     _cbClose=nullptr;
                H4AS_FN_WSBIN       _cbBin=nullptr;
                H4AS_FN_WSTXT       _cbTxt=nullptr;

                void                 _socketMessage(H4AS_HTTPRequest* r,const uint8_t* data,uint16_t len);
    protected:
        virtual bool                _execute() override;
    public:

        H4AT_HTTPHandlerWS(const std::string& url);
        ~H4AT_HTTPHandlerWS();

                void                broadcastBinary(const uint8_t* data,size_t len);

                template<typename... Args>
                void                broadcastText(const char* fmt, Args... args){ for(auto const& c:_clients) c->sendText(fmt, args...); }

                void                onBinaryMessage(H4AS_FN_WSBIN cb){ _cbBin=cb; }
                void                onClose(H4AS_FN_WSEVENT cb){ _cbClose=cb; }
                void                onOpen(H4AS_FN_WSEVENT cb){ _cbOpen=cb; }
                void                onTextMessage(H4AS_FN_WSTXT cb){ _cbTxt=cb; }
                void                reset() override;
                size_t              size(){ return _clients.size(); }
//
                void                dumpClients(){ for(auto const& c:_clients) Serial.printf("   WS CLIENT %p\n",c); }
// just dont...
        static  void                _sendFrame(H4AS_HTTPRequest* r,uint8_t opcode,const uint8_t* data=nullptr,uint16_t len=0);
};

using H4AS_EVT_HANDLER   = std::function<void(size_t)>;
using H4AS_SSE_LIST      = std::unordered_set<H4AsyncClient*>;

class H4AT_HTTPHandlerSSE: public H4AT_HTTPHandler {
            H4AS_SSE_LIST                       _clients;
            H4AS_EVT_HANDLER                    _cbConnect;
            std::map<size_t,std::string>        _backlog;
            size_t                              _bs;
    protected:
        virtual bool                _execute() override;
    public:
            size_t                              _nextID=0;

        H4AT_HTTPHandlerSSE(const std::string& url,size_t backlog=0);
        ~H4AT_HTTPHandlerSSE();

                void                onConnect(H4AS_EVT_HANDLER cb){ _cbConnect=cb; }
                void                reset() override;
        virtual void                saveBacklog(const std::string& msg);
                void                send(const std::string& message, const std::string& event="");
                size_t              size(){ return _clients.size(); }
//
//                void                dumpClients(){
//                    Serial.printf("CLIENTS:\n");
//                    for(auto const& c:_clients) Serial.printf("   SSE CLIENT %p\n",c); 
//                }
};

using H4AS_HANDLER_LIST  = std::vector<H4AT_HTTPHandler*>;

class H4AsyncWebServer: public H4AsyncServer {
        static  void                _scavenge();
    public:
            size_t                  _cacheAge;
            H4AS_HANDLER_LIST       _handlers;

        H4AsyncWebServer(uint16_t port,size_t cacheAge=H4AS_CACHE_AGE): _cacheAge(cacheAge),H4AsyncServer(port){}
        virtual ~H4AsyncWebServer(){}

                void            addHandler(H4AT_HTTPHandler* h);
                void            begin() override;
                void            on(const char* path,int verb,H4AS_RQ_HANDLER f);

        virtual void            reset();
// don't call
                H4AsyncClient*  _instantiateRequest(struct tcp_pcb* p) override { return reinterpret_cast<H4AsyncClient*>(new H4AS_HTTPRequest(p)); }
                void            route(void* c,const uint8_t* data,size_t len) override;
};

template<typename... Args>
void H4AS_HTTPRequest::sendText(const char* fmt, Args&&... args){ // variadic T<>
    char* buff=static_cast<char*>(malloc(256+1));
    snprintf(buff,256,fmt,args...);
    H4AT_HTTPHandlerWS::_sendFrame(this,WS_TEXT,(const uint8_t*) buff,strlen(buff));
    free(buff);
}

constexpr const char* txtContentLength(){ return "Content-Length"; }
constexpr const char* txtSecWebsocketKey(){ return "Sec-Websocket-Key"; }