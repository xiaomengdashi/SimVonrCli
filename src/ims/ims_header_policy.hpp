#pragma once

#include <string>
#include <vector>

namespace sim {
namespace ims {

class ImsHeaderPolicy {
public:
    static std::vector<std::string> BuildForRegister(
        const std::string& impu,
        bool enable100rel);

    static std::vector<std::string> BuildForInvite(
        const std::string& impu,
        bool enable100rel);
};

} // namespace ims
} // namespace sim
