#include "util/redaction.hpp"

#include <regex>

namespace sim {
namespace util {

std::string RedactSipLine(const std::string& line) {
    std::string output = line;
    output = std::regex_replace(output, std::regex(R"(response=[^,\s]+)"), "response=<redacted>");
    output = std::regex_replace(output, std::regex(R"(ck=[^,\s]+)"), "ck=<redacted>");
    output = std::regex_replace(output, std::regex(R"(ik=[^,\s]+)"), "ik=<redacted>");
    output = std::regex_replace(output, std::regex(R"(ki=[^,\s]+)"), "ki=<redacted>");
    output = std::regex_replace(output, std::regex(R"(opc=[^,\s]+)"), "opc=<redacted>");
    return output;
}

} // namespace util
} // namespace sim
