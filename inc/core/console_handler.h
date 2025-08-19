#ifndef _CONSOLE_HANDLER_H
#define _CONSOLE_HANDLER_H
//[INCLUDES]
#include "../network/tcp_server.h"




//[NAMESPACE]
namespace ConsoleHandler
{
    //[TYPES]
    using Handler = void(*)(Network::tcp_cllient_t*, const std::vector<std::string>);


    //[VARIABLES]
    extern Network::TcpServer server;

    //[FUNCTIONS]
    void AddCommands(const std::string& _key, Handler _handler);
    void Init();

    void Await();
}
#endif //!_CONSOLE_HANDLER_H