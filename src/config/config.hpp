#pragma once

#include <string>

namespace sim {
namespace config {

struct LocalConfig {
    std::string bind_ip;
    int bind_port{0};
};

struct PcscfConfig {
    std::string host;
    int port{0};
    std::string transport;
};

struct IdentityConfig {
    std::string impi;
    std::string impu;
    std::string realm;
};

struct DigestConfig {
    std::string password;
};

struct AkaConfig {
    std::string opc;
    std::string ki;
    std::string amf;
    std::string sqn;
};

struct AuthConfig {
    std::string mode;
    DigestConfig digest;
    AkaConfig aka;
};

struct SipConfig {
    std::string user_agent;
    int expires{0};
    bool enable_100rel{false};
    bool auto_answer{true};
};

struct ImsHeadersConfig {
    bool enable{false};
};

struct Config {
    LocalConfig local;
    PcscfConfig pcscf;
    IdentityConfig identity;
    AuthConfig auth;
    SipConfig sip;
    ImsHeadersConfig ims_headers;
};

Config load_config(const std::string& path);

} // namespace config
} // namespace sim
