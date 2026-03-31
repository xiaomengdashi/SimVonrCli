#include "ims/ims_header_policy.hpp"

namespace sim {
namespace ims {

std::vector<std::string> ImsHeaderPolicy::BuildForRegister(
    const std::string& impu,
    bool enable100rel) {
    std::vector<std::string> headers;
    headers.emplace_back("P-Preferred-Identity: <" + impu + ">");
    headers.emplace_back("P-Access-Network-Info: 3GPP-E-UTRAN-FDD;utran-cell-id-3gpp=00000000");

    if (enable100rel) {
        headers.emplace_back("Supported: 100rel");
    }

    return headers;
}

std::vector<std::string> ImsHeaderPolicy::BuildForInvite(
    const std::string& impu,
    bool enable100rel) {
    return BuildForRegister(impu, enable100rel);
}

} // namespace ims
} // namespace sim
