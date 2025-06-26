//[INCLUDES]
#include <iostream>

#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <signal.h>


#include "../inc/utils/filesys.h"
#include "../inc/core/console.h"


//[DEFINES]


//[VARIABLES]
std::atomic<bool> running = true;


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
    Console::Message(" /$$$$$$$  /$$$$$$$  /$$$$$$$$ /$$    /$$ /$$$$$$$$ /$$$$$$$ ");
    Console::Message("| $$__  $$| $$__  $$| $$_____/| $$   | $$| $$_____/| $$__  $$");
    Console::Message("| $$  \\ $$| $$  \\ $$| $$      | $$   | $$| $$      | $$  \\ $$");
    Console::Message("| $$  | $$| $$$$$$$/| $$$$$   |  $$ / $$/| $$$$$   | $$$$$$$/");
    Console::Message("| $$  | $$| $$__  $$| $$__/    \\  $$ $$/ | $$__/   | $$__  $$");
    Console::Message("| $$  | $$| $$  \\ $$| $$        \\  $$$/  | $$      | $$  \\ $$");
    Console::Message("| $$$$$$$/| $$  | $$| $$$$$$$$   \\  $/   | $$$$$$$$| $$  | $$");
    Console::Message("|_______/ |__/  |__/|________/    \\_/    |________/|__/  |__/");
    Console::Message("By Mabox1509");
    Console::Message("Version 0.1.0");
    Console::Message("---------------------------------------------------------------------\n");

    //SETUP APPLICATION
    std::string executable_path = argv[0];
    std::string working_directory = executable_path.substr(0, executable_path.find_last_of("/\\"));
    SetWorkingDirectory(working_directory);
    signal(SIGPIPE, SIG_IGN);


    // START SYSTEMS
    Console::Init();




    //MANTAIN APPLICATION
    while (running)
    {
        // Main loop can be used for other tasks or just to keep the program running
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    // Clean up and exit
    //main_server.Stop();
    std::cout << "Exiting program." << std::endl;
    

    return 0;
}