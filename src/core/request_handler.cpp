//[INCLUDES]
#include "../../inc/core/request_handler.h"


#include "../../inc/core/console_handler.h"
#include "../../inc/log.h"

//[VARIABLES]
#define PORT 5000


//[HANDLER]
void OnRequest(const Network::http_request_t& _request, Network::HttpServer* _object)
{
    Log::Message("http request: %s", _request.path.c_str());
    auto _entry = RequestHandler::routes.find(_request.path);

    if(_entry == RequestHandler::routes.end())
    {
        _object->ResposeText("mal", _request.socket);
    }
    else
    {
        _entry->second.handler(_request, _object);
    }
}

//[REQUEST]
void ReqIcon(const Network::http_request_t& _request, Network::HttpServer* _object)
{
    _object->ResposeFile("res/icon.ico", _request.socket);
}



//[NAMESPACE]
namespace RequestHandler
{
    //[VARIABLES]
    Network::HttpServer server(PORT, 16);
    std::map<std::string, http_route_t> routes;

    //[FUNCTIONS]
    void AddRoute(const std::string& _route, HttpCallback _callback, HttpMethod _method)
    {
        // Verificar que empiece con "/"
        if (_route.empty() || _route[0] != '/')
        {
            Log::Error("Invalid route: must start with '/' => %s", _route.c_str());
            exit(1);
        }

        // Revisar si ya existe
        if (routes.find(_route) != routes.end())
        {
            Log::Error("Duplicate route detected: %s", _route.c_str());
            exit(1);
        }

        // Insertar
        routes[_route] = http_route_t{_method, _callback};
    }
    void Init()
    {
        //ROUTES
        AddRoute("/favicon.ico", ReqIcon, HttpMethod::GET);

        //SETUP
        server.on_request = OnRequest;

        //START
        server.Start();
        Log::Message("HTTP port opended:  localhost:%d", PORT);
    }
}
