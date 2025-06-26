#ifndef _CONSOLE_H
#define _CONSOLE_H
//[INCLUDES]
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "../network/tcp_server.h"


//[PRIVATE FUNCTIONS]

//[NAMESPACE]
namespace Console
{
    //[TYPES]
    

    typedef struct instruction_t
    {
        char opcode;
        char* data;
        size_t data_size;
    } instruction_t;

    class Packet
    {
        private:
            std::vector<instruction_t> instructions;

        public:
            Packet() = default;
            ~Packet();

        void AddMessage(const std::string& _message, char _color);
        void AddAllowInput();
        void AddKick(const std::string& _reason);

        std::vector<unsigned char> Serialize() const;
    };

    typedef void (*cmd_func_t)(const std::vector<std::string>&, int, Packet*);
    typedef struct cmd_t
    {
        std::string help;
        cmd_func_t func;
    } cmd_t;

    //[VARIABLES]
    extern std::map<std::string, cmd_t> commands;


    //[FUNCTIONS]
    void Message(const char* _msg, ...);
    void Warning(const char* _msg, ...);
    void Error(const char* _msg, ...);

    void Init();
    void RegisterCommand(const std::string& command, const std::string& help, const cmd_func_t func);
    void Stop();
}


#endif