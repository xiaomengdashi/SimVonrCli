#pragma once

#include <string>

namespace sim {
namespace sip {

enum class SipMethod {
    Invite,
    Prack,
    Ack,
    Bye,
    Cancel,
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

enum class SipEventType {
    IncomingRequest,
    IncomingResponse,
    TransportError,
};

struct SipEvent {
    SipEventType type{SipEventType::TransportError};
    SipMethod method{SipMethod::Unknown};
    int statusCode{};
    std::string callId;
    std::string raw;
};

inline std::string to_string(SipMethod method) {
    switch (method) {
    case SipMethod::Invite:
        return "INVITE";
    case SipMethod::Prack:
        return "PRACK";
    case SipMethod::Ack:
        return "ACK";
    case SipMethod::Bye:
        return "BYE";
    case SipMethod::Cancel:
        return "CANCEL";
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
