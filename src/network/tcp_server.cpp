//[INCLUDES]
#include "../../inc/network/tcp_server.h"
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
#include <iomanip>
#include <algorithm>


#include "../../inc/utils/filesys.h"


//#include "../inc/log.h"

//[PRIVATE DATA]
#define PACKAGE_SIZE 2048

//[PRIVATE FUNCTIONS]


//[CLASS - FUNCTIONS]
namespace Network
{
    //[CONSTRUCTORS]
    TcpServer::TcpServer(uint16_t _port)
    {
        running = false;
        port = _port;
        server_socket = -1;

    }
    TcpServer::~TcpServer()
    {
        if (running)
        {
            Stop();
        }
    }

    //[FUNCTIONS]

    void TcpServer::Loop()
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
                continue;

            

            auto client = std::make_shared<tcp_client_t>();
            client->socket = _socket;

            // Guardar en lista antes de lanzar el hilo
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.push_back(client);
            }

            client->thread = std::thread(&TcpServer::ClientLoop, this, client);
            client->thread.detach(); // Desacoplar el hilo para que no bloquee





            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Esperar un poco para cerrar conexiones activas
        std::this_thread::sleep_for(std::chrono::seconds(2));
        close(server_socket);
    }
    void TcpServer::ClientLoop(std::shared_ptr<tcp_client_t> _client)
    {
        //std::cout << "New client connected: " << _client->socket << std::endl;
        char buffer[PACKAGE_SIZE];

        if (on_join) on_join(*_client, this); // Evento de conexión

        
        while (true)
        {
            size_t _bytes = recv(_client->socket, buffer, sizeof(buffer), 0);
            if(_bytes <= 0)
            {
                break; // Salir del bucle si no hay datos o error
            }

            //std::cout << "Received data from client: " << _client->socket << std::endl;
            on_data(*_client, buffer, _bytes, this);

            if (!running) break;
        }

        //std::cout << "Client disconnected: " << _client->socket << std::endl;

        if (on_leave) on_leave(*_client, this);

        //std::cout << "Closing client socket: " << _client->socket << std::endl;
        close(_client->socket);

        
        //std::cout << "Removing client from list: " << _client->socket << std::endl;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(std::remove_if(clients.begin(), clients.end(),
                [&](const std::shared_ptr<tcp_client_t>& c) {
                    return c->socket == _client->socket;
                }), clients.end());
        }
        //std::cout << "Client thread ended: " << _client->socket << std::endl;

    }


    void TcpServer::Start()
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

        main_thread = std::thread(&TcpServer::Loop, this);
    }
    void TcpServer::Stop()
    {
        if (!running) return;

        running = false;
        if (main_thread.joinable())
            main_thread.join();
    }

    bool TcpServer::IsAwake()
    {
        return running;
    }

    void TcpServer::Send(const std::vector<unsigned char>& _data, int _socket)
    {
        if (_socket < 0) return;
        send(_socket, _data.data(), _data.size(), 0);
    }
    void TcpServer::Send2All(const std::vector<unsigned char>& _data)
    {
        std::lock_guard<std::mutex> lock(clients_mutex);

        for (auto& client : clients)
        {
            if (client->socket != -1)
            {
                send(client->socket, _data.data(), _data.size(), 0);
            }
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