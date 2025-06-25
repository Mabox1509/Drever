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
    bool fix_command = false;

    // Función para obtener la hora actual en formato [HH:MM:ss]
    std::string GetCurrentTime()
    {
        std::time_t t = std::time(nullptr);
        std::tm* now = std::localtime(&t);

        char buffer[9];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", now);
        return std::string(buffer);
    }

    // Función para formatear mensajes con \n bien alineados
    std::string FormatMessage(const std::string& message, size_t header_size)
    {
        std::stringstream formatted;
        size_t pos = 0;

        for (char c : message)
        {
            formatted << c;
            if (c == '\n') // Si hay un salto de línea, agregar espacios extra
            {
                formatted << std::string(header_size, ' '); 
            }
        }

        return formatted.str();
    }

    // Función base para imprimir mensajes con formato
    void PrintLog(const char* type, const char* colorCode, const char* msg, va_list args)
    {
        // Obtener la hora actual
        std::string timeStr = GetCurrentTime();

        // Formatear el mensaje con printf-style
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), msg, args);

        // Crear la cabecera
        std::string header = "\033[0m[" + timeStr + "]" + colorCode + "[" + type + "]: ";
        size_t header_size = header.length();

        // Formatear el mensaje para alinear \n correctamente
        std::string formattedMessage = FormatMessage(buffer, header_size);

        // Imprimir
        printf("\033[G"); // Mueve el cursor a X=0
        printf("%s%s\033[0m\n", header.c_str(), formattedMessage.c_str());

        // Si fix_command está activado, imprime "> " en la siguiente línea
        if (fix_command)
        {
            printf("> ");
            fflush(stdout);
        }
    }

    // Funciones públicas
    void Message(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        PrintLog("MESSAGE", "\033[0m", msg, args);
        va_end(args);
    }

    void Warning(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        PrintLog("WARNING", "\033[33m", msg, args); // Amarillo
        va_end(args);
    }

    void Error(const char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        PrintLog("ERROR", "\033[31m", msg, args); // Rojo
        va_end(args);
    }
}
