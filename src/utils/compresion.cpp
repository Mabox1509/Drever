#include "../../inc/utils/compression.h"
#include <zlib.h>
#include <stdexcept>
#include <iostream>

namespace Compression
{
    std::vector<unsigned char> Compress(const std::vector<unsigned char>& input)
    {
        uLong sourceLength = static_cast<uLong>(input.size());
        uLong destLength = compressBound(sourceLength);  // Tamaño máximo de compresión

        std::vector<unsigned char> output(destLength);

        z_stream stream{};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        // Configurar el stream para compresión
        if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
            throw std::runtime_error("deflateInit failed");

        stream.avail_in = sourceLength;
        stream.next_in = const_cast<Bytef*>(input.data());

        stream.avail_out = destLength;
        stream.next_out = output.data();

        int ret = deflate(&stream, Z_FINISH);
        if (ret != Z_STREAM_END)
        {
            deflateEnd(&stream);
            throw std::runtime_error("deflate failed");
        }

        output.resize(stream.total_out);
        deflateEnd(&stream);
        return output;
    }

    std::vector<unsigned char> Decompress(const std::vector<unsigned char>& input)
    {
        uLong sourceLength = static_cast<uLong>(input.size());
        uLong destLength = sourceLength * 4;  // Estimación inicial

        std::vector<unsigned char> output(destLength);

        z_stream stream{};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        if (inflateInit(&stream) != Z_OK)
            throw std::runtime_error("inflateInit failed");

        stream.avail_in = sourceLength;
        stream.next_in = const_cast<Bytef*>(input.data());

        int ret;
        do {
            stream.avail_out = output.size() - stream.total_out;
            stream.next_out = output.data() + stream.total_out;

            ret = inflate(&stream, Z_NO_FLUSH);

            if (ret == Z_BUF_ERROR || stream.avail_out == 0) {
                // Aumentar el buffer si es necesario
                output.resize(output.size() * 2);
                continue;
            }

            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&stream);
                throw std::runtime_error("inflate failed");
            }

        } while (ret != Z_STREAM_END);

        output.resize(stream.total_out);
        inflateEnd(&stream);
        return output;
}

}
