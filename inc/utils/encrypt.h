#ifndef _ENCRYPT_H
#define _ENCRYPT_H
//[INCLUDES]
#include <vector>

//[NAMESPACE]
namespace Encrypt
{
    std::vector<unsigned char> Encrypt(const std::vector<unsigned char>& _input);
    std::vector<unsigned char> Decrypt(const std::vector<unsigned char>& _input);
}
#endif