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
#include<H4AsyncWebServer.h>

void H4AsyncWebServer::_incoming(tcp_pcb* p){ 
    H4AT_PRINT1("INCOMING <-- PCB=%p\n",p);
    newConnection(p,new H4AS_HTTPRequest(p,this));
    H4AT_PRINT1("INCOMING --> PCB=%p s/o=%u\n",p,sizeof(H4AS_HTTPRequest));
}

void H4AsyncWebServer::addDefaultHandlers(){
//    H4AT_PRINT1("H4AsyncWebServer::addDefaultHandlers\n");
    _handlers.push_back(new H4AT_HTTPHandlerFile());
    _handlers.push_back(new H4AT_HTTPHandler404());
}

void H4AsyncWebServer::begin(){
//    H4AT_PRINT1("BEGIN %p\n",this);
    H4AsyncServer::begin();
    onError([=](int e,int i){ Serial.printf("H4AsyncWebServer ERROR %d %d\n",e,i); });
}

void H4AsyncWebServer::on(const char* path,int verb,H4AS_RQ_HANDLER f){_handlers.push_back(new H4AT_HTTPHandler{verb,path,f}); }

void H4AsyncWebServer::reset(){
//    H4AT_PRINT1("H4AsyncWebServer::reset()\n");
    for(auto &h:_handlers){
        H4AT_PRINT1("reset handler %s %s\n",h->_verbName().data(),h->_path.data());
        h->reset();
        delete h;
    }
    _handlers.clear();
//    H4AT_PRINT1("H4AsyncWebServer::reset() handlers cleared\n");
}