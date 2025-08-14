//[INCLUDES]
#include "../inc/log.h"
#include <iostream>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <unistd.h>
#include <sstream>

//[NAMESPACE]
namespace Log
{
    std::vector<log_t> logs; // almacenamiento interno

    //Print log
    static void AddLog(LogType type, const char* msg, va_list args)
    {
        // Formatear mensaje
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), msg, args);

        // Obtener timestamp legible HH:MM:SS
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&t, &tm); // en Linux
        char ts[16];
        std::strftime(ts, sizeof(ts), "%H:%M:%S", &tm);

        // Guardar en la lista
        logs.push_back(log_t{
            std::string(buffer),
            type,
            now
        });

        // Determinar tipo
        const char* type_str = "INFO";
        switch(type)
        {
            case LogType::Info: type_str = "INFO"; break;
            case LogType::Error: type_str = "ERROR"; break;
            case LogType::Warning: type_str = "WARN"; break;
            default: break;
        }

        // --- DEBUG: imprimir en consola ---
        printf("[%s][%s]: %s\n", ts, type_str, buffer);
    }


    // Funciones públicas
    void Message(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        AddLog(LogType::Info, msg, args);
        va_end(args);
    }

    void Warning(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        AddLog(LogType::Warning, msg, args);
        va_end(args);
    }

    void Error(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        AddLog(LogType::Error, msg, args);
        va_end(args);
    }

    // Devolver logs (copia)
    std::vector<log_t> GetLogs()
    {
        return logs;
    }

    // O devolver referencia const para no copiar
    const std::vector<log_t>& GetLogsRef()
    {
        return logs;
    }
}
