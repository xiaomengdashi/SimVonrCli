#include <filesystem>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "config/config.hpp"

namespace {

std::filesystem::path project_root() {
    return std::filesystem::path(TEST_PROJECT_ROOT);
}

} // namespace

TEST(ConfigLoaderTest, LoadsYamlConfigFromFile) {
    const auto cfg_path = project_root() / "configs" / "config.example.yaml";

    const sim::config::Config cfg = sim::config::load_config(cfg_path.string());

    EXPECT_EQ(cfg.local.bind_ip, "0.0.0.0");
    EXPECT_EQ(cfg.local.bind_port, 5060);
    EXPECT_EQ(cfg.pcscf.host, "proxy.example.com");
    EXPECT_EQ(cfg.pcscf.port, 5060);
    EXPECT_EQ(cfg.pcscf.transport, "udp");
    EXPECT_EQ(cfg.identity.impi, "alice@example.com");
    EXPECT_EQ(cfg.identity.impu, "sip:alice@example.com");
    EXPECT_EQ(cfg.identity.realm, "example.com");
    EXPECT_EQ(cfg.sip.user_agent, "sim-vonr-cli/0.1.0");
    EXPECT_EQ(cfg.sip.expires, 3600);
    EXPECT_FALSE(cfg.sip.enable_100rel);
    EXPECT_FALSE(cfg.ims_headers.enable);
}

TEST(ConfigLoaderTest, RejectsNonUdpTransport) {
    const auto cfg_path = project_root() / "tests" / "fixtures" / "config_tcp.yaml";

    EXPECT_THROW(
        static_cast<void>(sim::config::load_config(cfg_path.string())),
        std::runtime_error);
}
