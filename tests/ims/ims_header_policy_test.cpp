#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "ims/ims_header_policy.hpp"

namespace {

bool Contains(const std::vector<std::string>& headers, const std::string& header) {
    return std::find(headers.begin(), headers.end(), header) != headers.end();
}

} // namespace

TEST(ImsHeaderPolicyTest, BuildForRegisterIncludesRequiredHeadersWithout100rel) {
    const auto headers = sim::ims::ImsHeaderPolicy::BuildForRegister("sip:alice@example.com", false);

    EXPECT_TRUE(Contains(headers, "P-Preferred-Identity: <sip:alice@example.com>"));
    EXPECT_TRUE(Contains(headers, "P-Access-Network-Info: 3GPP-E-UTRAN-FDD;utran-cell-id-3gpp=00000000"));
    EXPECT_FALSE(Contains(headers, "Supported: 100rel"));
}

TEST(ImsHeaderPolicyTest, BuildForRegisterIncludes100relWhenEnabled) {
    const auto headers = sim::ims::ImsHeaderPolicy::BuildForRegister("sip:alice@example.com", true);

    EXPECT_TRUE(Contains(headers, "Supported: 100rel"));
}

TEST(ImsHeaderPolicyTest, BuildForInviteIncludesRequiredHeadersWhen100relEnabled) {
    const auto invite_headers = sim::ims::ImsHeaderPolicy::BuildForInvite("sip:alice@example.com", true);

    EXPECT_TRUE(Contains(invite_headers, "P-Preferred-Identity: <sip:alice@example.com>"));
    EXPECT_TRUE(Contains(invite_headers, "P-Access-Network-Info: 3GPP-E-UTRAN-FDD;utran-cell-id-3gpp=00000000"));
    EXPECT_TRUE(Contains(invite_headers, "Supported: 100rel"));
}
