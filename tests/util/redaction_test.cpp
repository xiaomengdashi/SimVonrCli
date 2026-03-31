#include <gtest/gtest.h>

#include "util/redaction.hpp"

TEST(RedactionTest, RedactSensitiveFields) {
    const std::string raw = "Authorization: Digest response=abcd, ck=1234, ik=5678";

    const auto output = sim::util::RedactSipLine(raw);

    EXPECT_NE(output.find("response=<redacted>"), std::string::npos);
    EXPECT_NE(output.find("ck=<redacted>"), std::string::npos);
    EXPECT_NE(output.find("ik=<redacted>"), std::string::npos);
}
