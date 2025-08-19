//[INCLUDES]
#include "../../inc/core/console_handler.h"

#include <map>
#include <thread>
#include <chrono>

#include "../../inc/log.h"


//[VARIABLES]
#define PORT 42100
const char app_key[4] = {'D','R','M','S'};
std::map<std::string,ConsoleHandler::Handler> requests;


//[NAMESPACE]
namespace ConsoleHandler
{
    //[VARIABLES]
    Network::TcpServer server(PORT, app_key);


    //[FUNCTIONS]
    void AddCommands(const std::string& _key, Handler _handler)
    {
        requests[_key] = _handler;
    }
    void Init()
    {
        server.Start();
        Log::Message("Control port opended: %d", PORT);

    }

    void Stop()
    {
        Log::Message("Stoping thread");
        server.Stop();
    }
    void Await()
    {
        while (server.IsAwake())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    }

    
}
