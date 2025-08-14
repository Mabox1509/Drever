//[INCLUDES]
#include "../../inc/utils/encrypt.h"
#include "../../inc/utils/compression.h"
#include <string>
#include <stdexcept>

//[DEFINES]
const std::string master_key = "48efff723cc07vHi2qMZ\"7M0G*<\"|@>d-itwN(|q=*D{QIx2~t15sC9DaEZwQLxhhWZUbC34FYWwE9679fb94285c7bbd1602d995bdffbe18d63aa818486967e6f9b27b159d73a93d82ab6f6c98c4912e00B(#R,{g?BY*Z'^;D+i)#<iEUZ,]/SB/g20286a2dc1ee0ba61c082b42ae035e34c";

//[NAMESPACE]
namespace Encrypt
{
    std::vector<unsigned char> Encrypt(const std::vector<unsigned char>& _input)
    {
        // Comprimir los datos
        std::vector<unsigned char> _compress = Compression::Compress(_input);

        // Reservar salida codificada (cada byte -> int16_t = 2 bytes)
        std::vector<unsigned char> _output;
        _output.reserve(_compress.size() * 2);

        for (size_t i = 0; i < _compress.size(); ++i)
        {
            int16_t val = static_cast<int16_t>(_compress[i]);
            char key = master_key[i % master_key.length()];
            val -= static_cast<int16_t>(key);

            // Almacenar el int16_t como 2 bytes en little endian
            _output.push_back(static_cast<unsigned char>(val & 0xFF));
            _output.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        }

        return _output;
    }

    std::vector<unsigned char> Decrypt(const std::vector<unsigned char>& _input)
    {
        // Asegúrate de que el input es par (2 bytes por valor)
        if (_input.size() % 2 != 0)
            throw std::runtime_error("Encrypted data has invalid size");

        size_t compressed_size = _input.size() / 2;
        std::vector<unsigned char> _compressed;
        _compressed.reserve(compressed_size);

        for (size_t i = 0; i < compressed_size; ++i)
        {
            // Leer int16_t desde dos bytes (little endian)
            int16_t val = static_cast<int16_t>(_input[i * 2] | (_input[i * 2 + 1] << 8));
            char key = master_key[i % master_key.length()];
            val += static_cast<int16_t>(key); // Revertimos el offset
            _compressed.push_back(static_cast<unsigned char>(val));
        }

        // Descomprimir los datos
        return Compression::Decompress(_compressed);
    }

}
