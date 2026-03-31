#include "config/config.hpp"

#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace sim {
namespace config {

Config load_config(const std::string& path) {
    const YAML::Node root = YAML::LoadFile(path);

    Config cfg;

    cfg.local.bind_ip = root["local"]["bind_ip"].as<std::string>();
    cfg.local.bind_port = root["local"]["bind_port"].as<int>();

    cfg.pcscf.host = root["pcscf"]["host"].as<std::string>();
    cfg.pcscf.port = root["pcscf"]["port"].as<int>();
    cfg.pcscf.transport = root["pcscf"]["transport"].as<std::string>("udp");

    if (cfg.pcscf.transport != "udp") {
        throw std::runtime_error("pcscf.transport must be udp");
    }

    cfg.identity.impi = root["identity"]["impi"].as<std::string>();
    cfg.identity.impu = root["identity"]["impu"].as<std::string>();
    cfg.identity.realm = root["identity"]["realm"].as<std::string>();

    cfg.auth.mode = root["auth"]["mode"].as<std::string>("aka");
    cfg.auth.digest.password = root["auth"]["digest"]["password"].as<std::string>();
    cfg.auth.aka.opc = root["auth"]["aka"]["opc"].as<std::string>();
    cfg.auth.aka.ki = root["auth"]["aka"]["ki"].as<std::string>();
    cfg.auth.aka.amf = root["auth"]["aka"]["amf"].as<std::string>();
    cfg.auth.aka.sqn = root["auth"]["aka"]["sqn"].as<std::string>();

    cfg.sip.user_agent = root["sip"]["user_agent"].as<std::string>();
    cfg.sip.expires = root["sip"]["expires"].as<int>();
    cfg.sip.enable_100rel = root["sip"]["enable_100rel"].as<bool>();
    cfg.sip.auto_answer = root["sip"]["auto_answer"].as<bool>(true);

    cfg.ims_headers.enable = root["ims_headers"]["enable"].as<bool>();

    return cfg;
}

} // namespace config
} // namespace sim
