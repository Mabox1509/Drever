//[INCLUDES]
#include "../../inc/core/console.h"

#include <cstdarg>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>

#include "../../inc/compression.h"
#include "../../inc/utils/string_utils.h"



//[DEFINES & VARIABLES]
typedef struct log_t
{
    std::string message;
    char color;
} log_t;

#define CONSOLE_PORT 5090
Network::TcpServer console_server(CONSOLE_PORT);

std::vector<log_t> logs;
std::mutex logs_mutex;



//[PRIVATE FUNCTIONS]
std::string GetCurrentTime()
{
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    char buffer[9];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", now);
    return std::string(buffer);
}
void PrintLog(const char* _type, char _color, const char* _msg, va_list _args)
{
    // Formatear el mensaje
    char formatted_msg[1024];
    vsnprintf(formatted_msg, sizeof(formatted_msg), _msg, _args);

    // Obtener la hora actual
    std::string _time_str = GetCurrentTime();

    std::ostringstream _out;
    _out << "[" << _time_str << "] [" << _type << "]: " << formatted_msg;

    // Crear log
    log_t log;
    log.message = _out.str();
    log.color = _color;

    {
        std::lock_guard<std::mutex> lock(logs_mutex);
        logs.push_back(log);
    }

    Console::Packet _packet;
    _packet.AddMessage(log.message, log.color);
    auto _serialized_data = _packet.Serialize();
    console_server.Send2All(_serialized_data);
}

void CmdHelp(const std::vector<std::string>& _args, int _socket, Console::Packet* _packet)
{
    _packet->AddMessage("[COMMANDS LIST]", 0x03);
    for(const auto& _cmd : Console::commands)
    {
        _packet->AddMessage(_cmd.first + " - " + _cmd.second.help, 0x00);
    }
    _packet->AddMessage(" ", 0);
}
void CmdStop(const std::vector<std::string>& _args, int _socket, Console::Packet* _packet)
{
    Console::Message("Stopping console server...");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    _packet->AddKick("Server is stopping.");
    console_server.Send(_packet->Serialize(), _socket);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    console_server.Stop();
    

    exit(0); // Exit the application
}

void OnJoin(const Network::tcp_cllient_t& _client, Network::TcpServer* _server)
{
    //std::cout << "Creating packet" << std::endl;
    Console::Packet _packet;

    {
        //std::cout << "Adding logs" << std::endl;
        std::lock_guard<std::mutex> lock(logs_mutex);
        for (const auto& log : logs)
        {
            _packet.AddMessage(log.message, log.color);
        }
    }
    _packet.AddMessage("Welcome to the console server!", 0x00);
    _packet.AddMessage("Type 'help' to see available commands.", 0x00);
    _packet.AddMessage("Type 'exit' to disconnect.", 0x00);
    _packet.AddAllowInput();
   

    //std::cout << "Serializing packet" << std::endl;
    auto _data = _packet.Serialize();

    //std::cout << "Sending packet to client" << std::endl;
    _server->Send(_data, _client.socket);
}
void OnLeave(const Network::tcp_cllient_t& _client, Network::TcpServer* _server)
{}
void OnData(const Network::tcp_cllient_t& _client, char* _data, size_t _size, Network::TcpServer* _server)
{
    Console::Packet _packet;

    //READ INPUT
    std::string _input(_data, _size-1);
    auto _args = StringUtils::Split(_input, ' ');

    //Check if the command exists
    if (_args.empty())
    {
        _packet.AddMessage("No command entered.", 0x02);
    }
    else
    {
        std::string _command = StringUtils::ToLower(_args[0]);
        //_command.pop_back();
        _args.erase(_args.begin());


        auto it = Console::commands.find(_command);
        if (it != Console::commands.end())
        {
            // Execute the command
            it->second.func(_args, _client.socket, &_packet);
        }
        else
        {
            _packet.AddMessage("Unknown command: " + _command + "\n", 0x02);
        }
    }
    _packet.AddAllowInput();

    console_server.Send(_packet.Serialize(), _client.socket);
}


//[NAMESPACE]
namespace Console
{
    #pragma region Packet
    Packet::~Packet()
    {
        for (auto& instruction : instructions)
        {
            delete[] instruction.data;
        }
    }

    void Packet::AddMessage(const std::string& _message, char _color)
    {
        instruction_t _instruction;
        _instruction.opcode = 0x01;
        _instruction.data_size = _message.size() + 2; // +1 for null terminator
        _instruction.data = new char[_instruction.data_size];

        _instruction.data[0] = _color; // Store color as the first byte
        std::strcpy(_instruction.data+1, _message.c_str());
        _instruction.data[_instruction.data_size - 1] = '\0'; // Null terminator

        instructions.push_back(_instruction);
    }
    void Packet::AddAllowInput()
    {
        instruction_t _instruction;
        _instruction.opcode = 0x02;
        _instruction.data_size = 0;
        _instruction.data = nullptr; // No data needed for this instruction
        instructions.push_back(_instruction);
    }
    void Packet::AddKick(const std::string& _reason)
    {
        instruction_t _instruction;
        _instruction.opcode = 0x03;
        _instruction.data_size = _reason.size() + 1; // +1 for null terminator
        _instruction.data = new char[_instruction.data_size];
        std::strcpy(_instruction.data, _reason.c_str());
        instructions.push_back(_instruction);
    }

    std::vector<unsigned char> Packet::Serialize() const
    {
        // Calcular el tamaño total: solo opcode + data (sin data_size ni delimitadores)
        size_t _buffer_size = sizeof(size_t);  // espacio para instrucciones count
        for (const auto& instruction : instructions)
        {
            _buffer_size += sizeof(instruction.opcode) + instruction.data_size;
        }

        // Usamos vector directamente
        std::vector<unsigned char> buffer(_buffer_size);
        size_t offset = 0;

        // Escribimos la cantidad de instrucciones
        size_t instructions_count = instructions.size();
        std::memcpy(buffer.data() + offset, &instructions_count, sizeof(size_t));
        offset += sizeof(size_t);

        // Escribimos las instrucciones: opcode + data
        for (const auto& instruction : instructions)
        {
            buffer[offset++] = instruction.opcode;

            if (instruction.data_size > 0 && instruction.data != nullptr)
            {
                std::memcpy(buffer.data() + offset, instruction.data, instruction.data_size);
                offset += instruction.data_size;
            }
        }

        // Comprimir y retornar
        return Compression::Compress(buffer);
    }


    #pragma endregion

    //[VARIABLES]
    std::map<std::string, cmd_t> commands;


    //[FUNCTIONS]
    void Message(const char* _msg, ...)
    {
        va_list args;
        va_start(args, _msg);
        PrintLog("MESSAGE", 0x00, _msg, args);
        va_end(args);
    }
    void Warning(const char* _msg, ...)
    {
        va_list args;
        va_start(args, _msg);
        PrintLog("MESSAGE", 0x04, _msg, args);
        va_end(args);
    }
    void Error(const char* _msg, ...)
    {
        va_list args;
        va_start(args, _msg);
        PrintLog("MESSAGE", 0x02, _msg, args);
        va_end(args);
    }


    void Init()
    {
        console_server.on_join = OnJoin;
        console_server.on_leave = OnLeave;
        console_server.on_data = OnData;

        // Register default commands
        RegisterCommand("help", "Shows a list of commands", CmdHelp);
        RegisterCommand("stop", "Stop the server", CmdStop);

        console_server.Start();
    }
    void RegisterCommand(const std::string& command, const std::string& help, const cmd_func_t func)
    {
        cmd_t _cmd;
        _cmd.help = help;
        _cmd.func = func;

        commands[command] = _cmd;
        
    }
    void Stop(){}
}