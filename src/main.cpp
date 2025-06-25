//[INCLUDES]
#include <iostream>

#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <signal.h>


#include "../inc/utils/filesys.h"
#include "../inc/network/http_server.h"



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
void Response(const Network::http_request_t& _request, Network::HttpServer* _obj)
{
    if(_request.path == "/")
    {
        // Redirect to the main page
        _obj->ResposeText("Welcome to the HTTP Server!", _request.socket, 200);
        return;
    }
    _obj->ResposeFile("WebPage" + _request.path, _request.socket, 200);
}

//[MAIN]
int main(int argc, char* argv[])
{
    //SETUP APPLICATION
    std::string executable_path = argv[0];
    std::string working_directory = executable_path.substr(0, executable_path.find_last_of("/\\"));
    SetWorkingDirectory(working_directory);
    signal(SIGPIPE, SIG_IGN);

    // START HTTP SERVER
    Network::HttpServer server(8080, 10);
    server.on_request = Response;

    server.Start();

    while (true)
    {
        //Wait for "stop" command
        std::string command;
        std::cout << "Enter command (type 'stop' to exit): ";
        std::getline(std::cin, command);
        if (command == "stop")
        {
            server.Stop();
            std::cout << "Server stopped." << std::endl;
            break;
        }
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    // Clean up and exit
    server.Stop();
    std::cout << "Exiting program." << std::endl;
    

    return 0;
}