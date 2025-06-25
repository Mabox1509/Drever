#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

//[INCLUDES]
#include <unordered_map>
#include <functional>
#include <cstddef>
#include <vector>
#include <thread>
#include <netinet/in.h> 
#include <atomic>
#include <mutex>
#include <string>
#include <condition_variable>


#define SA struct sockaddr



//[CLASS]
namespace Network
{
    typedef struct http_request_t 
    {
        int socket;

        // Línea de inicio de la petición
        std::string method;    // GET, POST, etc.
        std::string path;      // /ruta/sin/parametros
        std::string version;   // HTTP/1.1

        // Parámetros en la URL: /ruta?clave=valor&otro=valor
        std::unordered_map<std::string, std::string> query_params;

        // Headers HTTP: Host, User-Agent, etc.
        std::unordered_map<std::string, std::string> headers;

        std::vector<char> body; // Cuerpo de la petición (para POST, PUT, etc.)

        // Utilidad para obtener valores fácilmente
        std::string GetQueryParam(const std::string& key) const {
            auto it = query_params.find(key);
            return (it != query_params.end()) ? it->second : "";
        }

        std::string GetHeader(const std::string& key) const {
            auto it = headers.find(key);
            return (it != headers.end()) ? it->second : "";
        }
    } http_request_t;

    class HttpServer
    {
        //[EVENTS]
        public:
        std::function<void(const http_request_t&, HttpServer*)> on_request;


        //[VARIABLES]
        private:
        bool running;

        uint16_t port;
        int server_socket;

        std::thread main_thread;
        struct sockaddr_in servaddr, cli;

        uint16_t max_clients;

        std::atomic<int> active_clients;
        std::mutex client_mutex;
        std::condition_variable client_cv;

        public:
        uint32_t max_post_size;

        //[CONSTRUCTORS]
        public:
        HttpServer(uint16_t _port, uint16_t _max_clients);
        ~HttpServer();

        //[FUNCTIONS]
        private: 
        void Loop();
        void HandleClient(int _socket);

        public:
        void Start();
        void Stop();

        bool IsAwake();

        void ResposeText(const std::string& _text, int _socket, int _code = 200);
        void ResposeFile(const std::string& _file_path, int _socket, int _code = 200);
    };
}

#endif