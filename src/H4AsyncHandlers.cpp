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
#include <H4AsyncWebServer.h>

H4_INT_MAP http_verb_names = { // tidy
    {HTTP_ANY,"*"},
    {HTTP_GET,"GET"},
    {HTTP_HEAD,"HEAD"},
    {HTTP_POST,"POST"},
    {HTTP_PUT,"PUT"},
    {HTTP_DELETE,"DELETE"},
    {HTTP_CONNECT,"CONNECT"},
    {HTTP_OPTIONS,"OPTIONS"},
    {HTTP_TRACE,"TRACE"},
    {HTTP_PATCH,"PATCH"}
};

H4_INT_MAP H4AT_HTTPHandler::_responseCodes{
    {100,"Continue"},
    {101,"Switching Protocols"},
    {200,"OK"},
    {304,"Not Modified"},
    {400,"Bad Request"},
    {401,"Unauthorized"},
    {403,"Forbidden"},
    {404,"Not Found"},
    {408,"Request Time-out"},
    {415,"Unsupported Media Type"},
    {500,"Internal Server Error"},
    {503,"Service Unavailable"}
};

H4T_NVP_MAP H4AT_HTTPHandler::mimeTypes={
  {"html","text/html"},
  {"htm","text/html"},
  {"css","text/css"},
  {"json","application/json"},
  {"js","application/javascript"},
  {"png","image/png"},
  {"jpg","image/jpeg"},
  {"ico","image/x-icon"},
  {"xml","text/xml"},
};
//
//  H4AS_HTTPRequest
//
std::string H4AS_HTTPRequest::_getHeader(H4T_NVP_MAP& m,const char* h){
    std::string uh=uppercase(h);
    return m.count(uh) ? m[uh]:"";
}

void H4AS_HTTPRequest::_paramsFromstring(const std::string& bod){
    std::vector<std::string> prams=split(bod,"&");
    for(auto &p:prams){
        std::vector<std::string> nvp=split(p,"=");
        std::string value=nvp.size()==2 ? nvp[1]:"";
        params[nvp[0]]=urldecode(value);
    }
//    #if H4AT_DEBUG
//    for(auto &p:params) Serial.printf("PARAMS[%s]=%s\n",p.first.data(),p.second.data());
//    #endif
}

void H4AS_HTTPRequest::process(const uint8_t* data,size_t len){
    H4AT_PRINT1("process %p srv=%p\n",this,_server);
    std::vector<std::string> rqst=split(std::string((const char*)data,len),"\r\n");
    std::vector<std::string> sub=split(replaceAll(rqst[0],"HTTP/1.1",""),"?");
    if(sub.size() > 1) _paramsFromstring(sub[1]);
    std::vector<std::string> vparts=split(sub[0]," ");

    H4T_NVP_MAP _rqHeaders;
    for(auto &r:std::vector<std::string>(++rqst.begin(),--rqst.end())){
        std::vector<std::string> rparts=split(r,":");
        _rqHeaders[uppercase(rparts[0])]=trim(rparts[1]);
    }
//    for(auto &r:_rqHeaders) Serial.printf("RQ %s=%s\n",r.first.data(),r.second.data());

    _blen=atoi(_getHeader(_rqHeaders,"Content-length").data());
    if(_blen){ // refactor get
        _body=static_cast<uint8_t*>(malloc(_blen));
        memcpy(_body,data+len-_blen,_blen);
        Serial.printf("SAVING BODY RQ=%p b=%p l=%d\n",this,_body,_blen);
        if(_getHeader(_rqHeaders,"Content-type")=="application/x-www-form-urlencoded"){
            std::string bod((const char*) _body,_blen);
            _paramsFromstring(bod);
        } else Serial.printf("received weird type %s\n",_getHeader(_rqHeaders,"Content-type").data());
    }
    //
    H4AT_PRINT1("CNX %p PCB=%p RQ %s %s bod=%p bl=%d nP=%d chk %d _handlers\n",this,pcb,vparts[0].data(),vparts[1].data(),_body,_blen,params.size(),_server->_handlers.size());
    for(auto h:_server->_handlers){
        for(auto &s:h->_sniffHeader) if(_rqHeaders.count(uppercase(s.first))) h->_sniffHeader[s.first]=_rqHeaders[uppercase(s.first)];
        if(h->_select(this,vparts[0],vparts[1])) break;
    }
}
//
// H4AT_HTTPHandler
//
bool H4AT_HTTPHandler::_execute(){
//    Serial.printf("H4AT_HTTPHandler::_execute <-- 0 %u\n",_HAL_freeHeap());
    if(_f) _f(this);
//    Serial.printf("H4AT_HTTPHandler::_execute --> 0 %u\n",_HAL_freeHeap());
    return true;
}

bool H4AT_HTTPHandler::_match(const std::string& verb,const std::string& path){
    if(verb==_verbName()){
        if(_path.size() > 1) return path.find(_path,0)==0;
        else return _path==path;
    } else return false;
}

bool H4AT_HTTPHandler::_select(H4AS_HTTPRequest* r,const std::string& verb,const std::string& path){
    _r=r;
    _r->url=urldecode(path);
    if(_match(verb,path)){
//        Serial.printf("H4AT_HTTPHandler::_select <-- 0 %u\n",_HAL_freeHeap());
        bool rv=_execute();
        reset();
//        Serial.printf("H4AT_HTTPHandler::_select --> 0 %u\n",_HAL_freeHeap());
        return rv;
    } else return false;
}

std::string H4AT_HTTPHandler::_verbName(){ return http_verb_names[_verb]; }

void H4AT_HTTPHandler::addCacheAge(){ _headers["Cache-Control"]="max-age="+stringFromInt(_r->_server->_cacheAge); }

void H4AT_HTTPHandler::reset(){
    Serial.printf("H4AT_HTTPHandler::reset() %p _r=%p\n",this,_r);
    _headers.clear();
    _sniffHeader.clear();
    _r->params.clear();
//    if(_r->_body) free(_r->_body);
}

void H4AT_HTTPHandler::send(uint16_t code,const std::string& type,size_t length,const void* _body){
    H4AT_PRINT2("H4AT_HTTPHandler %p send(%d,%s,%d,%p)\n",this,code,type.data(),length,_body);
    std::string status=std::string("HTTP/1.1 ")+stringFromInt(code,"%3d ").append(_responseCodes[code])+"\r\n";
    _headers["Content-Type"]=type;
    if(length) _headers["Content-Length"]=stringFromInt(length);
    for(auto const& h:_headers) status+=h.first+": "+h.second+"\r\n";
    status+="\r\n";
    //
    auto h=status.size();
    auto total=h+length;
    uint8_t* buff=(uint8_t*) malloc(total);
    if(buff){
        memcpy(buff,status.data(),h);
        if(length) memcpy(buff+h,_body,length);
        _r->TX(buff,total);
        free(buff);
    } else Serial.printf("AAAAAAAAARGH H4AT_HTTPHandler::send zero buff\n");
}

void H4AT_HTTPHandler::sendFile(const std::string& fn){ }//H4AT_HTTPHandlerFile::serveFile(this,fn); }

std::string H4AT_HTTPHandler::mimeType(const std::string& fn){
    std::string e = fn.substr(fn.rfind('.')+1);
    return mimeTypes.count(e) ? mimeTypes[e]:"text/plain";
}
//
// H4AT_HTTPHandlerFile match only verb, treat path as static filename
//
bool H4AT_HTTPHandlerFile::_match(const std::string& verb,const std::string& path) {
    _path=path;
    return verb==_verbName();
}

bool H4AT_HTTPHandlerFile::_execute(){
    bool rv=false;
    char crlf[]={'0','\r','\n','\r','\n'};
    size_t force_fit=TCP_MSS - 7; // allow chunk embellishment not to overflow "sensible" buf size hex\r\n ... \r\n == 7
//    Serial.printf("_execute <-- 0 %u\n",_HAL_freeHeap());
    readFileChunks(_path.data(),force_fit,
        [&](const uint8_t* data,size_t len){
            std::string hex=stringFromInt(len,"%03x\r\n");
            size_t total=len+hex.size()+2;
            uint8_t* buff=static_cast<uint8_t*>(malloc(total));
            memcpy(buff,hex.data(),hex.size());
            memcpy(buff+hex.size(),data,len);
            memcpy(buff+hex.size()+len,&crlf[3],2);
            _r->TX(buff,total);
            free(buff);
        },
        [&](size_t size){ 
            addHeader("Transfer-Encoding","chunked");
            addCacheAge();
            send(200,mimeType(_path),0,nullptr);
        },
        [&]{
            H4AT_PRINT1("ALL CHUNKED OUT for %s\n",_path.data());
            _r->TX((const uint8_t*) crlf,5);
            rv=true;
        }
    );
//    Serial.printf("_execute --> 0 %u\n",_HAL_freeHeap());
    return rv;
}
//
// H4AT_HTTPHandler404 match anything 
//
bool H4AT_HTTPHandler404::_execute() {
    H4AT_PRINT1("SENDING 404\n");
    send(404,"text/plain",5,"oops!");
    return true; 
}
//
// H4AT_HTTPHandlerSSE
//
H4AT_HTTPHandlerSSE::H4AT_HTTPHandlerSSE(const std::string& url, size_t backlog,uint32_t timeout):
    _timeout(timeout),
    _bs(backlog),
    H4AT_HTTPHandler(HTTP_GET,url) {
        Serial.printf("SSE HANDLER CTOR %p backlog=%d timeout=%d\n",this,_bs,_timeout);
//        _sniffHeader["last-event-id"]="";
}

H4AT_HTTPHandlerSSE::~H4AT_HTTPHandlerSSE(){
    Serial.printf("SSE HANDLER DTOR %p\n",this);
    reset();
}

bool H4AT_HTTPHandlerSSE::_execute(){
    _clients.insert(_r);
    auto c=_r;
    Serial.printf("SSE EXE %p\n",c);
    c->onDisconnect([=](){
        dumpClients();
        _clients.erase(c);
        Serial.printf("SSE DCX %p LEAVES %d clients\n",c,_clients.size());
        dumpClients();
        if(!_clients.size()) {
            Serial.printf("%p PRE-CLOSE state A:\n",this);
            Serial.printf("c=%p nH=%d snif=%d BL=%d id=%d\n",c,_headers.size(),_sniffHeader.size(),_backlog.size(),_nextID);            
            reset();
            Serial.printf("%p PRE-CLOSE state B:\n",this);
            Serial.printf("c=%p nH=%d snif=%d BL=%d id=%d\n",c,_headers.size(),_sniffHeader.size(),_backlog.size(),_nextID);            
            _cbConnect(0); // notify all gone
        }
    });
//    dumpClients();
    auto lid=atoi(_sniffHeader["last-event-id"].data());
    if(lid){
        H4AT_PRINT3("It's a reconnect! lid=%d\n",lid);
        for(auto b:_backlog){
            if(b.first > lid) c->TX((const uint8_t *) b.second.data(),b.second.size());
        }
    } else H4AT_PRINT1("New SSE Client %p\n",c);
    addHeader("Cache-Control","no-cache");
    H4AT_HTTPHandler::send(200,"text/event-stream",0,nullptr); // explicitly send zero!
    h4.queueFunction([=]{ 
        H4AT_PRINT1("SSE CLIENT %p set timeout %d\n",c,_timeout);
        std::string retry("retry: ");
        retry.append(stringFromInt(_timeout)).append("\n\n");
        c->TX((const uint8_t *) retry.data(),retry.size());
        _cbConnect(_clients.size());
    });
    h4.every((_timeout * 2) / 3,[=]{ send(":"); },nullptr,H4AS_SSE_KA_ID,true); // name it
    return true;
}

void H4AT_HTTPHandlerSSE::reset() { 
    Serial.printf("%p H4AT_HTTPHandlerSSE::reset 1\n",this);
    H4AT_HTTPHandler::reset();
    Serial.printf("%p H4AT_HTTPHandlerSSE::reset 2\n",this);
    _backlog.clear();
    Serial.printf("%p H4AT_HTTPHandlerSSE::reset 3\n",this);
    for(auto &c:_clients) c->_lastSeen=0;
    Serial.printf("%p H4AT_HTTPHandlerSSE::reset 4\n",this);
    h4.cancelSingleton(H4AS_SSE_KA_ID); // needed ?
    Serial.printf("%p H4AT_HTTPHandlerSSE::reset 5\n",this);
    _nextID=0;
}

void H4AT_HTTPHandlerSSE::saveBacklog(const std::string& m){
    _backlog[_nextID]=m;
    if(_backlog.size() > _bs) _backlog.erase(_nextID - _bs);
}

void H4AT_HTTPHandlerSSE::send(const std::string& message, const std::string& event){
    char buf[16];
    std::string rv;
    if(message[0]==':') rv=message+"\n";
    else {
        rv.append("id: ").append(itoa(++_nextID,buf,10)).append("\n");
        if(event.size()) rv+="event: "+event+"\n";
        std::vector<std::string> data;
        char *token = strtok(const_cast<char*>(message.data()), "\n");
        while (token != nullptr){
            data.push_back(std::string(token));
            token = strtok(nullptr, "\n");
        }
        for(auto &d:data) rv+="data: "+d+"\n";
    }
    rv+="\n";
    for(auto &c:_clients) {
        Serial.printf("c=%p closing=%d\n",c,c->_closing);
        c->TX((const uint8_t *) rv.data(),rv.size());
    }
    if(_bs) saveBacklog(rv);
}