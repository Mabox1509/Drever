#include "../inc/compression.h"
#include <zlib.h>
#include <stdexcept>
#include <iostream>

namespace Compression
{
    std::vector<unsigned char> Compress(const std::vector<unsigned char>& input)
    {
        uLong sourceLength = static_cast<uLong>(input.size());
        uLong destLength = compressBound(sourceLength);  // Calcula el tamaño máximo de salida

        std::vector<unsigned char> output(destLength);  // Creamos el vector de salida

        int result = compress(output.data(), &destLength, input.data(), sourceLength);

        if (result != Z_OK) {
            throw std::runtime_error("Error compressing data");
        }

        output.resize(destLength);  // Ajustamos el tamaño al tamaño real comprimido
        return output;
    }

    std::vector<unsigned char> Decompress(const std::vector<unsigned char>& input)
    {
        uLong sourceLength = static_cast<uLong>(input.size());
        uLong destLength = sourceLength * 4;  // Aproximadamente el tamaño máximo de salida al descomprimir

        std::vector<unsigned char> output(destLength);  // Creamos el vector de salida

        int result = uncompress(output.data(), &destLength, input.data(), sourceLength);

        if (result != Z_OK) {
            throw std::runtime_error("Error decompressing data");
        }

        output.resize(destLength);  // Ajustamos el tamaño al tamaño real descomprimido
        return output;
    }
}
