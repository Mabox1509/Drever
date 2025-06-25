#ifndef _LOG_H
#define _LOG_H
//[INCLUDES]



//[NAMESPACE]
namespace Log
{
    //[TYPES]

    
    //[VARIABLES]
    extern bool fix_command;

    //[FUNCTIONS]
    void Message(const char* _msg, ...);
    void Warning(const char* _msg, ...);
    void Error(const char* _msg, ...);
}
#endif