#include "cli/cli_controller.hpp"

#include <sstream>

namespace sim {
namespace cli {

CliCommand CliController::Parse(const std::string& line) const {
    std::istringstream iss(line);
    std::string op;
    iss >> op;

    if (op == "register") {
        return {CliCommandType::Register, ""};
    }
    if (op == "unregister") {
        return {CliCommandType::Unregister, ""};
    }
    if (op == "answer") {
        return {CliCommandType::Answer, ""};
    }
    if (op == "hangup") {
        return {CliCommandType::Hangup, ""};
    }
    if (op == "autoanswer") {
        std::string mode;
        iss >> mode;
        if (mode == "on" || mode == "off") {
            return {CliCommandType::AutoAnswer, mode};
        }
        return {CliCommandType::Invalid, ""};
    }
    if (op == "status") {
        return {CliCommandType::Status, ""};
    }
    if (op == "quit") {
        return {CliCommandType::Quit, ""};
    }
    if (op == "call") {
        std::string uri;
        iss >> uri;
        if (uri.empty()) {
            return {CliCommandType::Invalid, ""};
        }
        return {CliCommandType::Call, uri};
    }

    return {CliCommandType::Invalid, ""};
}

} // namespace cli
} // namespace sim
