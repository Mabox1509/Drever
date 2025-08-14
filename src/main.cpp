//[INCLUDES]
#include <iostream>

#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <signal.h>

#include "../inc/network/tcp_server.h"
#include "../inc/utils/filesys.h"
#include "../inc/log.h"

//[DEFINES]


//[VARIABLES]
const char app_key[4] = {'D','R','M','S'};
Network::TcpServer server(32100, app_key);

//[FUNCTIONS]
void SetWorkingDirectory(const std::string& path)
{
    //#include <unistd.h> // For chdir
    // Set the working directory to the specified path
    if (chdir(path.c_str()) != 0)
    {
        std::cerr << "Error setting working directory to: " << path << std::endl;
    }
}

void ServerJoin(const Network::tcp_cllient_t& _client, Network::TcpServer* _server)
{
    Log::Message("New client :D");
}
void ServerData(const Network::tcp_cllient_t& _client, char* _data, size_t _size, Network::TcpServer* _server)
{

    std::cout << _data << std::endl;
}
void ServerLeave(const Network::tcp_cllient_t& _client, Network::TcpServer* _server)
{
    Log::Message("Goodbye client D:");
}

//[MAIN]
int main(int argc, char* argv[])
{
    //DRAW LOGO
    std::system("clear");
    Log::Message(" /$$$$$$$  /$$$$$$$  /$$$$$$$$ /$$    /$$ /$$$$$$$$ /$$$$$$$ ");
    Log::Message("| $$__  $$| $$__  $$| $$_____/| $$   | $$| $$_____/| $$__  $$");
    Log::Message("| $$  \\ $$| $$  \\ $$| $$      | $$   | $$| $$      | $$  \\ $$");
    Log::Message("| $$  | $$| $$$$$$$/| $$$$$   |  $$ / $$/| $$$$$   | $$$$$$$/");
    Log::Message("| $$  | $$| $$__  $$| $$__/    \\  $$ $$/ | $$__/   | $$__  $$");
    Log::Message("| $$  | $$| $$  \\ $$| $$        \\  $$$/  | $$      | $$  \\ $$");
    Log::Message("| $$$$$$$/| $$  | $$| $$$$$$$$   \\  $/   | $$$$$$$$| $$  | $$");
    Log::Message("|_______/ |__/  |__/|________/    \\_/    |________/|__/  |__/");
    Log::Message("By Mabox1509");
    Log::Message("Version 0.1.0");
    Log::Message("---------------------------------------------------------------------\n");

    //SETUP APPLICATION
    std::string executable_path = argv[0];
    std::string working_directory = executable_path.substr(0, executable_path.find_last_of("/\\"));
    SetWorkingDirectory(working_directory);
    signal(SIGPIPE, SIG_IGN);


    // START SYSTEMS
    server.on_join = ServerJoin;
    server.on_data = ServerData;
    server.on_leave = ServerLeave;
    server.Start();



    //MANTAIN APPLICATION
    while (server.IsAwake())
    {
        /* code */
    }
    

    

    return 0;
}