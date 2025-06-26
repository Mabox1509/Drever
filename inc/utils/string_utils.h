// string_utils.h
#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#include <string>
#include <vector>

namespace StringUtils {

    bool StartsWith(const std::string& str, const std::string& prefix);
    bool EndsWith(const std::string& str, const std::string& suffix);

    std::string ToLower(const std::string& str);
    std::string ToUpper(const std::string& str);

    std::string Replace(const std::string& str, const std::string& from, const std::string& to);

    std::vector<std::string> Split(const std::string& str, char delimiter);

    std::string LTrim(const std::string& str);
    std::string RTrim(const std::string& str);
    std::string Trim(const std::string& str);

}

#endif