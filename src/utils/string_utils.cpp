// string_utils.cpp
#include "../../inc/utils/string_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace StringUtils {

    bool StartsWith(const std::string& str, const std::string& prefix) {
        return str.rfind(prefix, 0) == 0;
    }

    bool EndsWith(const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }

    std::string ToLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string ToUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    std::string Replace(const std::string& str, const std::string& from, const std::string& to) {
        std::string result = str;
        size_t start_pos = 0;
        while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
            result.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return result;
    }

    std::vector<std::string> Split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string LTrim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        return (start == std::string::npos) ? "" : str.substr(start);
    }

    std::string RTrim(const std::string& str) {
        size_t end = str.find_last_not_of(" \t\n\r");
        return (end == std::string::npos) ? "" : str.substr(0, end + 1);
    }

    std::string Trim(const std::string& str) {
        return LTrim(RTrim(str));
    }

}
