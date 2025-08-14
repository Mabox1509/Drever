#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

//[INCLUDES]
#include <memory>
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
    typedef struct tcp_client_t
    {
        int socket;
        struct sockaddr_in servaddr, cli;
        std::thread thread;

        bool is_sending = false;
        std::mutex send_mutex;
        std::condition_variable send_cv;

        size_t inbox_expect;
        std::vector<char> inbox;
    } tcp_cllient_t;


    class TcpServer
    {
        //[EVENTS]
        public:
        std::function<void(const tcp_cllient_t&, TcpServer*)> on_join;
        std::function<void(const tcp_cllient_t&, TcpServer*)> on_leave;
        std::function<void(const tcp_cllient_t&, char*, size_t, TcpServer*)> on_data;


        //[VARIABLES]
        private:
        bool running;

        uint16_t port;
        int server_socket;

        char pkg_key[4];

        std::thread main_thread;
        struct sockaddr_in servaddr, cli;


        std::mutex clients_mutex;
        std::vector<std::shared_ptr<tcp_client_t>> clients;



        //[CONSTRUCTORS]
        public:
        TcpServer(uint16_t _port, const char _key[4]);
        ~TcpServer();

        //[FUNCTIONS]
        private: 
        void Loop();
        void ClientLoop(std::shared_ptr<tcp_client_t> _client);

        public:
        void Start();
        void Stop();

        bool IsAwake();

        void Send(const std::vector<unsigned char>& _data, int _socket);
        void Send2All(const std::vector<unsigned char>& _data);
    };
}

#endif