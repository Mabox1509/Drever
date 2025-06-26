//[INCLUDES]
#include "../../inc/network/http_server.h"
#include <stdio.h> 
#include <netdb.h> 
#include <cmath>

#include <iostream>
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdexcept> 
#include <sstream>
#include <cstring>


#include "../../inc/utils/filesys.h"
#include "../../inc/core/console.h"


//#include "../inc/log.h"

//[PRIVATE DATA]
static std::unordered_map<int, std::string> http_status_messages = {
    {200, "OK"}, {400, "Bad Request"}, {404, "Not Found"},
    {413, "Payload Too Large"}, {431, "Request Header Fields Too Large"},
    {500, "Internal Server Error"}
};

//[PRIVATE FUNCTIONS]
bool SendAll(int _socket, const char* _data, size_t _length)
{
    size_t total_sent = 0;
    while (total_sent < _length)
    {
        ssize_t sent = send(_socket, _data + total_sent, _length - total_sent, 0);
        if (sent <= 0)
        {
            // Error o desconexión
            return false;
        }
        total_sent += sent;
    }
    return true;
}


//[CLASS - FUNCTIONS]
namespace Network
{
    HttpServer::HttpServer(uint16_t _port, uint16_t _max_clients) : active_clients(0)
    {
        running = false;
        port = _port;
        server_socket = -1;

        max_post_size = pow(1024, 2) * 10; // 10 MB
        max_clients = _max_clients;
    }
    HttpServer::~HttpServer()
    {
        if (running)
        {
            Stop();
        }
    }
    
    void HttpServer::Loop()
    {
        socklen_t _len = sizeof(cli);

        while (running)
        {
            fd_set _readfds;
            struct timeval _timeout;

            FD_ZERO(&_readfds);
            FD_SET(server_socket, &_readfds);

            _timeout.tv_sec = 0;
            _timeout.tv_usec = 500000; // 500 ms

            int _activity = select(server_socket + 1, &_readfds, nullptr, nullptr, &_timeout);

            if (_activity < 0 && running)
            {
                throw std::runtime_error("Error in select()");
                break;
            }

            if (_activity == 0)
            {
                continue;
            }


            
            int _socket = accept(server_socket, (SA*)&cli, &_len); 
            if (_socket < 0)
                break;

            
            // Wait for a empty slot
            {
                std::unique_lock<std::mutex> lock(client_mutex);
                client_cv.wait(lock, [&]() {
                    return active_clients < max_clients;
                });
            }

            //HANDLE CLIENT
            active_clients++;
            std::thread(&HttpServer::HandleClient, this, _socket).detach();


            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        //CLOSE CONNECTIONS
        std::this_thread::sleep_for(std::chrono::seconds(2)); //Wait 2 clients thread end
        close(server_socket);
    }
    void HttpServer::HandleClient(int _socket)
    {
        bool _valid = true;

        //GET DATA
        char _buffer[1024];
        ssize_t _bytes_received;
        ssize_t _body_received = 0;

        std::string _header;
        bool _headers_complete = false;

        http_request_t _request;
        _request.socket = _socket;
        
        while ((_bytes_received = recv(_socket, _buffer, sizeof(_buffer), 0)) > 0)
        {
            if(_bytes_received < 0)
            {
                
                _valid = false;
                break;
            }

            if(!_headers_complete)
            {
                _header.append(_buffer, _bytes_received);

                // Busca el final de los headers: "\r\n\r\n"
                if (_header.find("\r\n\r\n") != std::string::npos)
                {
                    _headers_complete = true;

                    //PARSE HEADER
                    
                    std::istringstream _header_stream(_header);
                    std::string _line;

                    // 2. Primera línea (método, path, versión)
                    if (std::getline(_header_stream, _line))
                    {
                        std::istringstream first_line(_line);
                        first_line >> _request.method >> _request.path >> _request.version;
                    }

                    // 3. Extrae parámetros de la URL si hay
                    size_t qpos = _request.path.find('?');
                    if (qpos != std::string::npos)
                    {
                        std::string query_str = _request.path.substr(qpos + 1);
                        _request.path = _request.path.substr(0, qpos);

                        std::istringstream qs(query_str);
                        std::string pair;
                        while (std::getline(qs, pair, '&'))
                        {
                            size_t eq = pair.find('=');
                            if (eq != std::string::npos)
                            {
                                std::string key = pair.substr(0, eq);
                                std::string val = pair.substr(eq + 1);
                                _request.query_params[key] = val;
                            }
                        }
                    }

                    // 4. Parse headers línea por línea
                    while (std::getline(_header_stream, _line))
                    {
                        if (_line == "\r" || _line.empty()) break; // Fin de headers

                        size_t colon = _line.find(':');
                        if (colon != std::string::npos)
                        {
                            std::string key = _line.substr(0, colon);
                            std::string value = _line.substr(colon + 1);

                            // Limpia espacios y \r\n
                            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
                            while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();

                            _request.headers[key] = value;
                        }
                    }


                    //If the method is not POST or PUT, break the loop
                    if (_request.method != "POST" && _request.method != "PUT")
                    {
                        //Stop read loop
                        break;
                    }
                    else
                    {
                        //Search for the Content-Length header
                        auto it = _request.headers.find("Content-Length");
                        if (it != _request.headers.end())
                        {
                            // Si hay Content-Length, leer el cuerpo
                            size_t content_length = std::stoul(it->second);
                            if (content_length > max_post_size)
                            {
                                // Si el tamaño del cuerpo es mayor al máximo permitido, cerrar conexión
                                //SEND ERROR CODE (NOT IMPLEMENTED YET)
                                
                                ResposeText("Payload too large", _socket, 413);
                                _valid = false;
                                break;
                            }

                            // Leer el cuerpo de la petición
                            _request.body.resize(content_length);
                        }
                        else
                        {
                            // Si no hay Content-Length, cerrar conexión
                            //SEND ERROR CODE (NOT IMPLEMENTED YET)

                            ResposeText("Content-Length header missing", _socket, 400);
                            _valid = false;
                            break;
                        }
                    }

                }

                // Si se pasan de cierto límite sin headers válidos => posible ataque
                if (_header.size() > 8192)
                {
                    //SEND ERROR CODE (NOT IMPLEMENTED YET)
                    ResposeText("Headers too large", _socket, 431);
                    _valid = false;
                    break;
                }
            }
            else
            {
                //Receive body data if needed -- this part is only executed if the headers are complete
                if (_request.body.size() > 0)
                {
                    size_t remaining = _request.body.size() - _body_received;
                    size_t to_copy = std::min(remaining, (size_t)_bytes_received);

                    std::memcpy(_request.body.data() + _body_received, _buffer, to_copy);
                    _body_received += to_copy;

                    // Si ya terminamos de leer todo el body, salimos del bucle
                    if (_body_received >= _request.body.size())
                        break;
                }
                else
                {
                    // No hay cuerpo, simplemente salimos del bucle
                    break;
                }
            }
        }


        //HANDLE REQUEST
        if(_valid)
        {
            on_request(_request, this);
        }

        //CLOSE CONECTION
        close(_socket);

        active_clients--;
        client_cv.notify_one();

    }


    void HttpServer::Start()
    {
        if(running)
        {
            throw std::runtime_error("Network is already initialized");
            return;
        }
        running = true;


        
        // socket create and verification 
        server_socket = socket(AF_INET, SOCK_STREAM, 0); 
        if (server_socket == -1)
        { 
            throw std::runtime_error("Error creating socket");
            return;
        } 


        bzero(&servaddr, sizeof(servaddr)); 
        
        // assign IP, PORT 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
        servaddr.sin_port = htons(port); 
        
        // Binding newly created socket to given IP and verification 
        if ((bind(server_socket, (SA*)&servaddr, sizeof(servaddr))) != 0)
        { 
            throw std::runtime_error("Socket bind failed");
            return;
        } 

        
        // Now server is ready to listen and verification 
        if ((listen(server_socket, 5)) != 0)
        { 
            throw std::runtime_error("Listen failed");
            return;
        }

        main_thread = std::thread(&HttpServer::Loop, this);
    }
    void HttpServer::Stop()
    {
        if (!running) return;

        running = false;
        if (main_thread.joinable())
            main_thread.join();
    }

    bool HttpServer::IsAwake()
    {
        return running;
    }


    void HttpServer::ResposeText(const std::string& _text, int _socket, int _code)
    {
        std::ostringstream response;
        std::string status_text = http_status_messages.count(_code) ? http_status_messages[_code] : "Unknown";
        response << "HTTP/1.1 " << _code << " " << status_text << "\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << _text.size() << "\r\n";
        response << "Connection: close\r\n"; // ← AÑADIR
        response << "\r\n"; // End of headers
        response << _text;

        std::string res_str = response.str();
        SendAll(_socket, res_str.c_str(), res_str.size());
    }
    void HttpServer::ResposeFile(const std::string& _file_path, int _socket, int _code)
    {
        std::cout << "ResposeFile: " << _file_path << std::endl;
        if (!FileSys::FileExists(_file_path))
        {
            
            ResposeText("File not found", _socket, 404);
            return;
        }

        auto _file = FileSys::ReadBinary(_file_path);
        std::string _ext = _file_path.substr(_file_path.find_last_of('.') + 1);
        std::string content_type;

        // Mapa de extensiones a tipos MIME
        static const std::unordered_map<std::string, std::string> mime_types =
        {
            // Documentos
            {"html", "text/html"},
            {"htm",  "text/html"},
            {"txt",  "text/plain"},
            {"csv",  "text/csv"},
            {"xml",  "application/xml"},
            {"pdf",  "application/pdf"},
            {"doc",  "application/msword"},
            {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {"xls",  "application/vnd.ms-excel"},
            {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {"ppt",  "application/vnd.ms-powerpoint"},
            {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

            // Imágenes
            {"png",  "image/png"},
            {"jpg",  "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"gif",  "image/gif"},
            {"bmp",  "image/bmp"},
            {"webp", "image/webp"},
            {"svg",  "image/svg+xml"},
            {"ico",  "image/x-icon"},
            {"tiff", "image/tiff"},
            {"tif",  "image/tiff"},

            // Audio
            {"mp3",  "audio/mpeg"},
            {"wav",  "audio/wav"},
            {"ogg",  "audio/ogg"},
            {"m4a",  "audio/mp4"},
            {"flac", "audio/flac"},
            {"aac",  "audio/aac"},
            {"weba", "audio/webm"},

            // Video
            {"mp4",  "video/mp4"},
            {"webm", "video/webm"},
            {"ogv",  "video/ogg"},
            {"mov",  "video/quicktime"},
            {"avi",  "video/x-msvideo"},
            {"mkv",  "video/x-matroska"},
            {"mpeg", "video/mpeg"},

            // Fuentes
            {"woff",  "font/woff"},
            {"woff2", "font/woff2"},
            {"ttf",   "font/ttf"},
            {"otf",   "font/otf"},
            {"eot",   "application/vnd.ms-fontobject"},

            // Archivos comprimidos
            {"zip",  "application/zip"},
            {"rar",  "application/vnd.rar"},
            {"gz",   "application/gzip"},
            {"tar",  "application/x-tar"},
            {"7z",   "application/x-7z-compressed"},

            // Código y datos
            {"js",    "application/javascript"},
            {"json",  "application/json"},
            {"css",   "text/css"},
            {"c",     "text/x-c"},
            {"cpp",   "text/x-c++"},
            {"h",     "text/x-c"},
            {"hpp",   "text/x-c++"},
            {"py",    "text/x-python"},
            {"sh",    "application/x-sh"},
            {"php",   "application/x-httpd-php"},
            {"java",  "text/x-java-source"},
            {"lua",   "text/x-lua"},

            // Otros binarios
            {"exe",  "application/octet-stream"},
            {"bin",  "application/octet-stream"},
            {"wasm", "application/wasm"}
        };

        // Convertir extensión a minúsculas
        std::transform(_ext.begin(), _ext.end(), _ext.begin(), ::tolower);

        auto it = mime_types.find(_ext);
        content_type = (it != mime_types.end()) ? it->second : "application/octet-stream";

        // Enviar encabezados
        std::ostringstream response;
        response << "HTTP/1.1 " << _code << " OK\r\n";
        response << "Content-Type: " << content_type << "\r\n";
        response << "Content-Length: " << _file.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";

        std::string header_str = response.str();
        if (!SendAll(_socket, header_str.c_str(), header_str.size()))
            return;

        if(!SendAll(_socket, _file.data(), _file.size()))
        {
            Console::Error("Error sending file body to client: %s", _file_path.c_str());
        }
    }


}
/*
void ClientLoop(int _socket)
    {
        //GET DATA
        client_t* _client = clients[_socket];
        if (!_client) return;

        Log::Message("New client connected: %s:%d", _client->ip, _client->port);
        if(on_connect) on_connect(_client);


        while (running)
        {
            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(_client->socket, &readfds);

            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 500 ms

            int activity = select(_client->socket + 1, &readfds, nullptr, nullptr, &timeout);
            if (activity < 0 && running)
            {
                Log::Error("Error in select() for client %s:%d", _client->ip, _client->port);
                break;
            }

            if (activity == 0)
            continue;

            // Leer datos
            ssize_t bytes_received = recv(_client->socket, _client->recv, sizeof(_client->recv) - 1, 0);
            if (bytes_received <= 0)
            {
                break;
            }

            _client->recv[bytes_received] = '\0'; // Asegurar terminación de string

            Log::Message("Received data from %s:%d: %s", _client->ip, _client->port, _client->recv);

            // Aquí podrías llamar a `on_data(client, package_t{})` si ya tienes manejo de paquetes
        }

        close(_client->socket);
        clients.erase(_socket);
        Log::Message("Client disconected %s:%d", _client->ip, _client->port);
        if(on_disconnect) on_disconnect(_client);

        delete _client;
    }
    void ServerLoop()
    {
        Log::Message("Listening on port %d", NETWORK_PORT);

        socklen_t _len = sizeof(cli);

        while (running)
        {
            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(main_socket, &readfds);

            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 500 ms

            int activity = select(main_socket + 1, &readfds, nullptr, nullptr, &timeout);

            if (activity < 0 && running)
            {
                Log::Error("Error in select()");
                break;
            }

            if (activity == 0)
            {
                continue;
            }

            int _socket = accept(main_socket, (SA*)&cli, &_len); 
            
            if (_socket < 0)
            break;


            //CREATE NEW CLIENT
            auto _client = new client_t();
            _client->socket = _socket;
            inet_ntop(AF_INET, &cli.sin_addr, _client->ip, sizeof(_client->ip));
            _client->port = ntohs(cli.sin_port);

            // Opcional: Hacer el socket no bloqueante
            //fcntl(_socket, F_SETFL, O_NONBLOCK);
            clients[_socket] = _client;


            
            _client->thread = std::thread(ClientLoop, _socket);
            _client->thread.detach();
        }

        //CLOSE CONNECTIONS
        std::this_thread::sleep_for(std::chrono::seconds(1)); //Wait 2 clients thread end
        close(main_socket);
    }


    //[FUNCTIONS]
    void Start()
    {
        if(running)
        {
            Log::Error("Network is alredy initialized");
            return;
        }
        running = true;

        Log::Message("Starting server...");
    
        
    
        // socket create and verification 
        main_socket = socket(AF_INET, SOCK_STREAM, 0); 
        if (main_socket == -1)
        { 
            Log::Error("Error creating socket");
            exit(1); 
        } 


        bzero(&servaddr, sizeof(servaddr)); 
    
        // assign IP, PORT 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
        servaddr.sin_port = htons(NETWORK_PORT); 
    
        // Binding newly created socket to given IP and verification 
        if ((bind(main_socket, (SA*)&servaddr, sizeof(servaddr))) != 0)
        { 
            Log::Error("Socket bind failed..."); 
            exit(1); 
        } 

    
        // Now server is ready to listen and verification 
        if ((listen(main_socket, 5)) != 0)
        { 
            Log::Error("Listen failed...\n"); 
            exit(1); 
        }

        main_thread = std::thread(ServerLoop);
    }
    void Stop()
    {
        Log::Message("Stoping server...");
        running = false;
        main_thread.join();
    }

    bool IsAwake()
    {
        return running;
    }

    void SendData(int _socket, const package_t& _pk)
    {

        

        //GENERATE FINNAL

    }
    void Kick(int _socket, const char* _reason){}*/