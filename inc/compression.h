#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <vector>
#include <zlib.h>  // Usamos zlib para la compresión y descompresión, puedes cambiar la librería si lo prefieres

//[NAMESPACE]
namespace Compression
{
    // Función para comprimir un array de bytes
    std::vector<unsigned char> Compress(const std::vector<unsigned char>& input);

    // Función para descomprimir un array de bytes
    std::vector<unsigned char> Decompress(const std::vector<unsigned char>& input);
}

#endif // _COMPRESSION_H
