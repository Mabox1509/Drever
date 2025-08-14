#ifndef _LOG_H
#define _LOG_H
//[INCLUDES]
#include <chrono>
#include <vector>
#include <string>

//[NAMESPACE]
namespace Log
{
    //[TYPES]
    enum class LogType : uint8_t
    { // uint8_t para que ocupe solo 1 byte
        Info,
        Warning,
        Error
    };

    typedef struct log_t
    {
        std::string message;
        LogType type;
        std::chrono::system_clock::time_point timestamp; // fecha/hora
    } log_t;
    
    //[VARIABLES]
    extern std::vector<log_t> logs;


    //[FUNCTIONS]
    void Message(const char* _msg, ...);
    void Warning(const char* _msg, ...);
    void Error(const char* _msg, ...);
}
#endif