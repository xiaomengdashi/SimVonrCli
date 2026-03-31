#pragma once

#include <string>

namespace sim {
namespace sip {

enum class SipMethod {
    Invite,
    Ack,
    Bye,
    Register,
    Options,
    Unknown
};

struct SipRequest {
    SipMethod method{SipMethod::Unknown};
    std::string uri;
    std::string from;
    std::string to;
    std::string call_id;
};

inline std::string to_string(SipMethod method) {
    switch (method) {
    case SipMethod::Invite:
        return "INVITE";
    case SipMethod::Ack:
        return "ACK";
    case SipMethod::Bye:
        return "BYE";
    case SipMethod::Register:
        return "REGISTER";
    case SipMethod::Options:
        return "OPTIONS";
    default:
        return "UNKNOWN";
    }
}

} // namespace sip
} // namespace sim
