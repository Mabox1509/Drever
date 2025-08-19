//[INCLUDES]
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <signal.h>
#include <fstream>


#include "../inc/core/console_handler.h"
#include "../inc/core/request_handler.h"
#include "../inc/utils/compression.h"
#include "../inc/core/user_manager.h"
#include "../inc/utils/filesys.h"
#include "../inc/log.h"

//[DEFINES]


//[VARIABLES]




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
    Log::Message("---------------------------------------------------------------------");

    
    //SETUP APPLICATION
    std::string executable_path = argv[0];
    std::string working_directory = executable_path.substr(0, executable_path.find_last_of("/\\"));
    SetWorkingDirectory(working_directory);
    signal(SIGPIPE, SIG_IGN);


    // START SYSTEMS
    UserManager::Init();
    RequestHandler::Init();
    
    ConsoleHandler::Init();


    // MANTAIN APPLICATION
    ConsoleHandler::Await();
    

    return 0;
}