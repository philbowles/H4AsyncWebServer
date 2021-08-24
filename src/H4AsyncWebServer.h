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
//
//   H4AS_Request< T > + derivatives
//
template<typename T>
class H4AS_Request: public H4AsyncClient {
    public:
        T*          _server=nullptr;
        H4AS_Request(tcp_pcb* p,T* server=nullptr): _server(server), H4AsyncClient(p){
            H4AT_PRINT1("H4AS_Request CTOR %p pcb=%p srv=%p\n",this,p,server);
            onRX([=](const uint8_t* data,size_t len){ process(data,len); });
        }
        virtual ~H4AS_Request(){ H4AT_PRINT1("H4AS_Request DTOR %p\n",this); }
        virtual void            process(const uint8_t* data,size_t len)=0;
};

class H4AS_HTTPRequest: public H4AS_Request<H4AsyncWebServer> {
    protected:
        static  std::string     _getHeader(H4T_NVP_MAP& m,const char* h);
                void            _paramsFromstring(const std::string& bod);
    public:
                std::string     url;
                uint8_t*        _body=nullptr;
                size_t          _blen=0;
                H4T_NVP_MAP    params;

        H4AS_HTTPRequest(tcp_pcb* p,H4AsyncWebServer* server): H4AS_Request<H4AsyncWebServer>(p,server){ H4AT_PRINT1("H4AS_HTTPRequest CTOR %p\n",this);}
        virtual ~H4AS_HTTPRequest(){ 
            Serial.printf("H4AS_HTTPRequest DTOR %p body=%p\n",this,_body);
            if(_body){
                Serial.printf("H4AS_HTTPRequest DTOR %p body=%p\n",this,_body);
                free(_body);
            }
        }

        virtual void            process(const uint8_t* data,size_t len);
};
//
//   H4AT_HTTPHandler + derivatives
//
class H4AT_HTTPHandler {
    protected:
                H4AS_RQ_HANDLER     _f=nullptr;
        static  H4_INT_MAP          _responseCodes;

        virtual bool                _execute();
        virtual bool                _match(const std::string& verb,const std::string& path);

    public:
            H4T_NVP_MAP            _headers; // hoist!

                std::string         _verbName();
                int                 _verb;
                std::string         _path;
                H4AS_HTTPRequest*   _r=nullptr;
                H4T_NVP_MAP         _sniffHeader;

        static  H4T_NVP_MAP         mimeTypes;

        H4AT_HTTPHandler(int verb,const std::string& path,H4AS_RQ_HANDLER f=nullptr): _verb(verb),_path(path),_f(f){
            H4AT_PRINT1("H4AT_HTTPHandler CTOR %p v=%d vn=%s p=%s\n",this,_verb,_verbName().data(),path.data());
        }
        virtual ~H4AT_HTTPHandler(){ 
            H4AT_PRINT1("H4AT_HTTPHandler DTOR %p\n",this);
            reset(); // prob don't need this
        }

        inline  void                addCacheAge();
                void                addHeader(const std::string& name,const std::string& value){ _headers[name]=value; }
                uint8_t*            bodyData(){ return _r->_body; }
                size_t              bodySize(){ return _r->_blen; } // tidy these
        static  std::string         mimeType(const std::string& fn);
        virtual void                reset();
        virtual void                send(uint16_t code,const std::string& type,size_t length=0,const void* _body=nullptr);
        virtual void                sendFile(const std::string& fn);
        virtual void                sendstring(const std::string& type,const std::string& data){ send(200,type,data.size(),(const void*) data.data()); }
                std::string         url(){ return _r->url; } // tidy these
//      don't call
                bool                _select(H4AS_HTTPRequest* r,const std::string& verb,const std::string& path);
};

class H4AT_HTTPHandlerFile: public H4AT_HTTPHandler {
    protected:
        virtual bool    _execute() override;
        virtual bool    _match(const std::string& verb,const std::string& path) override;
    public:
        H4AT_HTTPHandlerFile(): H4AT_HTTPHandler(HTTP_GET,"*"){};
        virtual void    reset() override { _path="*"; } // not REALLY needed
};

class H4AT_HTTPHandler404: public H4AT_HTTPHandler {
    protected:
        virtual bool    _execute() override;
        virtual bool    _match(const std::string& verb,const std::string& path) override { return true; }
    public:
        H4AT_HTTPHandler404(): H4AT_HTTPHandler(HTTP_ANY,"404"){}
};

class H4AT_HTTPHandlerSSE;

using H4AS_HANDLER_LIST  = std::vector<H4AT_HTTPHandler*>;
using H4AS_EVT_HANDLER   = std::function<void(size_t)>;
using H4AS_SSE_LIST      = std::unordered_set<H4AsyncClient*>;

class H4AT_HTTPHandlerSSE: public H4AT_HTTPHandler {
            H4AS_SSE_LIST                       _clients;
            H4AS_EVT_HANDLER                    _cbConnect;
            uint32_t                            _timeout;
            std::map<size_t,std::string>        _backlog;
            size_t                              _bs;
    protected:
        virtual bool                _execute() override;
    public:
            size_t                              _nextID=0;

        H4AT_HTTPHandlerSSE(const std::string& url,size_t backlog=0,uint32_t timeout=H4AS_SCAVENGE_FREQ);
        ~H4AT_HTTPHandlerSSE();

                size_t              size(){ return _clients.size(); }
                void                onConnect(H4AS_EVT_HANDLER cb){ _cbConnect=cb; }
                void                reset() override;
        virtual void                saveBacklog(const std::string& msg);
                void                send(const std::string& message, const std::string& event="");
                void                dumpClients(){ for(auto const& c:_clients) Serial.printf("   SSE CLIENT %p\n",c); }
};
//
//  H4AsyncWebServer
//
class H4AsyncWebServer: public H4AsyncServer {
    public:
            size_t                  _cacheAge;
            H4AS_HANDLER_LIST       _handlers;

        H4AsyncWebServer(uint16_t port,size_t cacheAge=H4AS_CACHE_AGE): _cacheAge(cacheAge),H4AsyncServer(port){}
        virtual ~H4AsyncWebServer(){}

        virtual void            addDefaultHandlers();
                void            addHandler(H4AT_HTTPHandler* h){ _handlers.push_back(h); }
                void            begin() override;
                void            on(const char* path,int verb,H4AS_RQ_HANDLER f);

        virtual void            reset();
// don't call
                void            _incoming(tcp_pcb* p) override;
};