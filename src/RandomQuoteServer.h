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

std::vector<std::string> wisdom={
    "Don't eat yellow snow",
    "A dead Yak never flies at night",
    "Click to subscribe to the <a href=\"https://www.youtube.com/channel/UCYi-Ko76_3p9hBUtleZRY6g\">YouTube channel!</a>",
    "The roses are pink in Moscow today",
    "47 muscles to frown, only 12 to pull the trigger on a sniper rifle",
    "You have the luxury of not knowing what I know",
    "Support me on Patreon! <div class=\"lnk\"><a href=\"https://patreon.com/esparto\" rel=\"noopener\" target=\"_blank\"><img src=\"/patreon.jpg\"></image></a></div></div>",
    "Never put off till tomorrow what will keep till the day after",
    "There is no problem so big you can't walk away from it",
    "Money cannot buy happiness, but its more comfortable to cry in a Mercedes than on a bicycle",
    "Forgive your enemy, but remember the bastard's name",
    "Help someone when they are in trouble, and they will remember you when they are in trouble again",
    "It is better to stay silent and be thought a fool, than to open one's mouth and immediately remove all doubt",
    "Many people are alive today only because it's illegal to shoot them",
    "Alcohol does not solve any problems, but neither does milk"
};

class RandomQuoteServer: public H4AsyncWebServer {
  public:
    RandomQuoteServer(uint16_t port): H4AsyncWebServer(port){
        on("/",HTTP_GET,[=](H4AW_HTTPHandler* h){
            std::string html="<HTML><BODY><CENTER><H1>"+wisdom[random(0,wisdom.size())]+"</H1></CENTER></BODY></HTML>";
            h->sendstring(H4AW_HTTPHandler::mimeTypes["htm"],html);
        });
    }
};