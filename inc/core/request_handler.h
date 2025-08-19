#ifndef _REQUEST_HANDLER_H
#define _REQUEST_HANDLER_H
//[INCLUDES]
#include "../network/http_server.h"

#include <map>


//[NAMESPACE]
namespace RequestHandler
{
    enum class HttpMethod { GET, POST, PUT, DELETE };
    using HttpCallback = void(*)(const Network::http_request_t& _request, Network::HttpServer* _object);

    //TYPES
    typedef struct http_route_t
    {
        HttpMethod method;
        HttpCallback handler;
    } http_route_t;

    //[VARIABLES]
    extern Network::HttpServer server;
    extern std::map<std::string, http_route_t> routes;

    //[FUNCTIONS]
    void AddRoute(const std::string& _route, HttpCallback _callback, HttpMethod _method);
    void Init();
}
#endif //!_REQUEST_HANDLER_H