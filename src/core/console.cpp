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
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>


#include "../../inc/compression.h"
#include "../../inc/utils/string_utils.h"



//[DEFINES & VARIABLES]
/*enum class Color
{
    Default,

    // Normal colors
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,

    // Bright colors
    BrightBlack,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    BrightWhite,
};*/
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
std::string FormatSize(size_t bytes) {
    const char* suffixes[] = { "bytes", "KB", "MB", "GB", "TB" };
    int i = 0;
    double size = bytes;

    while (size >= 1024.0 && i < 4) {
        size /= 1024.0;
        ++i;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << suffixes[i];
    return oss.str();
}
// Genera una barra de progreso para representar uso
std::string CreateBar(double percent, int width = 30) {
    int filled = static_cast<int>(percent * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? "█" : "░";
    bar += "] " + std::to_string(static_cast<int>(percent * 100)) + "%";
    return bar;
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
void CmdStatus(const std::vector<std::string>& _args, int _socket, Console::Packet* _packet)
{
    std::string line, ignore;

    // --- RAM Usage ---
    std::ifstream status("/proc/self/status");
    _packet->AddMessage("== Memory Usage ==", 0x03);
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0 || line.find("VmSize:") == 0) {
            _packet->AddMessage(line, 0x00);
        }
    }

    // --- CPU Usage & Info ---
    _packet->AddMessage("", 0);
    _packet->AddMessage("== CPU Info & Usage ==", 0x03);

    // Obtener número de núcleos
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    // Leer /proc/self/stat para tiempos
    std::ifstream stat("/proc/self/stat");
    long utime = 0, stime = 0, starttime = 0;
    for (int i = 0; i < 13; ++i) stat >> ignore;
    stat >> utime >> stime;
    for (int i = 0; i < 4; ++i) stat >> ignore; // saltar campos hasta llegar a starttime
    stat >> starttime;

    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double total_time = (utime + stime) / (double)ticks_per_sec;

    // Leer tiempo desde que se inició el sistema
    std::ifstream uptime_file("/proc/uptime");
    double uptime_seconds = 0.0;
    uptime_file >> uptime_seconds;

    // Calcular tiempo que lleva el proceso corriendo
    double process_start_time = starttime / (double)ticks_per_sec;
    double elapsed_time = uptime_seconds - process_start_time;

    // Calcular uso de CPU en porcentaje
    double cpu_usage_percent = (elapsed_time > 0) ? (total_time / elapsed_time) * 100.0 / num_cpus : 0.0;
    double user_time = utime / (double)ticks_per_sec;
    double system_time = stime / (double)ticks_per_sec;

    // Leer velocidad del CPU
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string cpu_line, cpu_mhz = "N/A", model_name = "Unknown";
    while (std::getline(cpuinfo, cpu_line)) {
        if (cpu_line.find("model name") != std::string::npos && model_name == "Unknown") {
            model_name = cpu_line.substr(cpu_line.find(":") + 2);
        } else if (cpu_line.find("cpu MHz") != std::string::npos && cpu_mhz == "N/A") {
            cpu_mhz = cpu_line.substr(cpu_line.find(":") + 2);
        }
        if (cpu_mhz != "N/A" && model_name != "Unknown") break;
    }
    double ghz = std::stod(cpu_mhz) / 1000.0;

    // Mostrar datos
    _packet->AddMessage("CPU Model: " + model_name, 0x00);
    _packet->AddMessage("CPU Speed: " + std::to_string(ghz) + " GHz", 0x00);
    _packet->AddMessage("CPU Cores: " + std::to_string(num_cpus), 0x00);
    _packet->AddMessage("Process uptime: " + std::to_string(elapsed_time) + " s", 0x00);
    _packet->AddMessage("User time: " + std::to_string(user_time) + " s", 0x00);
    _packet->AddMessage("System time: " + std::to_string(system_time) + " s", 0x00);
    _packet->AddMessage("Total CPU time: " + std::to_string(total_time) + " s", 0x00);
    _packet->AddMessage("CPU usage: " + std::to_string(cpu_usage_percent) + " %", 0x00);
    _packet->AddMessage(CreateBar(cpu_usage_percent / 100.0), 0x00);


    // --- Network Usage ---
    std::ifstream net("/proc/net/dev");
    _packet->AddMessage("", 0);
    _packet->AddMessage("== Network Usage ==", 0x03);
    for (int i = 0; i < 2; ++i) std::getline(net, line); // Skip headers
    while (std::getline(net, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string iface = line.substr(0, pos);
        std::istringstream data(line.substr(pos + 1));
        long rx, tx;
        data >> rx;
        for (int i = 0; i < 7; ++i) data >> ignore;
        data >> tx;
        _packet->AddMessage(iface + " RX: " + FormatSize(rx) + ", TX: " + FormatSize(tx), 0x00);
    }

    // --- Disk Usage (real path) ---
    _packet->AddMessage("", 0);
    _packet->AddMessage("== Disk Usage (Executable Drive) ==", 0x03);

    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';

        // Obtener solo el directorio
        std::string exe_dir(exe_path);
        auto last_slash = exe_dir.find_last_of('/');
        if (last_slash != std::string::npos)
            exe_dir = exe_dir.substr(0, last_slash + 1);

        struct statvfs disk;
        if (statvfs(exe_dir.c_str(), &disk) == 0) {
            unsigned long total = disk.f_blocks * disk.f_frsize;
            unsigned long free  = disk.f_bfree  * disk.f_frsize;
            unsigned long avail = disk.f_bavail * disk.f_frsize;
            unsigned long used  = total - free;

            double used_percent = static_cast<double>(used) / total;

            _packet->AddMessage("Path: " + exe_dir, 0x00);
            _packet->AddMessage("Total: " + FormatSize(total), 0x00);
            _packet->AddMessage("Used : " + FormatSize(used), 0x00);
            _packet->AddMessage("Free : " + FormatSize(avail), 0x00);
            _packet->AddMessage(CreateBar(used_percent), 0x00);
        } else {
            _packet->AddMessage("Error: could not access filesystem stats for " + exe_dir, 0x04);
        }
    }


    _packet->AddMessage("", 0);
}
void CmdStop(const std::vector<std::string>& _args, int _socket, Console::Packet* _packet)
{
    Console::Message("Stopping console server...");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    _packet->AddKick("Server is stopping.");
    console_server.Send2All(_packet->Serialize());

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
            try
            {
                it->second.func(_args, _client.socket, &_packet);
            }
            catch(const std::exception& e)
            {
                _packet.AddMessage("Error executing command: " + std::string(e.what()), 0x02);
            }
            
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
        RegisterCommand("status", "Shows the server status", CmdStatus);

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