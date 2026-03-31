#pragma once

#include <string>

namespace sim {
namespace cli {

enum class CliCommandType {
    Register,
    Unregister,
    Call,
    Answer,
    Hangup,
    Status,
    Quit,
    Invalid,
};

struct CliCommand {
    CliCommandType type{CliCommandType::Invalid};
    std::string arg;
};

class CliController {
public:
    CliCommand Parse(const std::string& line) const;
};

} // namespace cli
} // namespace sim
