#include "registration/registration_service.hpp"

namespace sim {
namespace registration {

void RegistrationService::StartRegister() {
    state_ = RegistrationState::Registering;
}

void RegistrationService::StartUnregister() {
    state_ = RegistrationState::Unregistering;
}

void RegistrationService::OnResponse(int statusCode,
                                     sip::SipMethod method,
                                     const std::map<std::string, std::string>&) {
    if (method != sip::SipMethod::Register) {
        return;
    }

    if (statusCode == 200 && state_ == RegistrationState::Registering) {
        state_ = RegistrationState::Registered;
        return;
    }

    if ((statusCode == 401 || statusCode == 407) && state_ == RegistrationState::Registering) {
        state_ = RegistrationState::Registering;
        return;
    }

    if (statusCode >= 500) {
        state_ = RegistrationState::RegisterFailed;
    }
}

void RegistrationService::OnIntervalTooBrief(int minExpires) {
    pendingExpires_ = minExpires;
}

RegistrationState RegistrationService::State() const {
    return state_;
}

int RegistrationService::PendingExpires() const {
    return pendingExpires_;
}

} // namespace registration
} // namespace sim
