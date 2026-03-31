#pragma once

#include "core/sip_types.hpp"

#include <map>
#include <string>

namespace sim {
namespace registration {

enum class RegistrationState {
    Idle,
    Registering,
    Registered,
    Refreshing,
    Unregistering,
    RegisterFailed,
};

class RegistrationService {
public:
    void StartRegister();
    void StartUnregister();
    void OnResponse(int statusCode, sip::SipMethod method, const std::map<std::string, std::string>& headers);
    void OnIntervalTooBrief(int minExpires);

    RegistrationState State() const;
    int PendingExpires() const;

private:
    RegistrationState state_{RegistrationState::Idle};
    int pendingExpires_{300};
};

} // namespace registration
} // namespace sim
