#include "registration/registration_service.hpp"

#include <utility>

namespace sim {
namespace registration {

RegistrationService::RegistrationService(std::chrono::milliseconds retryInterval)
    : retryInterval_(retryInterval),
      retryThread_(&RegistrationService::RetryLoop, this) {
}

RegistrationService::~RegistrationService() {
    stop_.store(true);
    cv_.notify_all();
    if (retryThread_.joinable()) {
        retryThread_.join();
    }
}

void RegistrationService::SetSendRegisterHandler(std::function<void()> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    sendRegisterHandler_ = std::move(handler);
}

bool RegistrationService::StartRegister() {
    std::unique_lock<std::mutex> lock(mutex_);
    return TriggerRegisterAttemptLocked(lock);
}

void RegistrationService::StartUnregister() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = RegistrationState::Unregistering;
    inFlight_ = false;
    challengeSeen_ = false;
}

void RegistrationService::OnResponse(int statusCode,
                                     sip::SipMethod method,
                                     const std::map<std::string, std::string>&) {
    if (method != sip::SipMethod::Register) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (statusCode == 200) {
        state_ = RegistrationState::Registered;
        inFlight_ = false;
        challengeSeen_ = false;
        cv_.notify_all();
        return;
    }

    if (statusCode == 401 || statusCode == 407) {
        if (!challengeSeen_) {
            state_ = RegistrationState::Registering;
            challengeSeen_ = true;
            return;
        }

        state_ = RegistrationState::RegisterFailed;
        inFlight_ = false;
        challengeSeen_ = false;
        cv_.notify_all();
        return;
    }

    if (statusCode >= 300) {
        state_ = RegistrationState::RegisterFailed;
        inFlight_ = false;
        challengeSeen_ = false;
        cv_.notify_all();
    }
}

void RegistrationService::OnTransportError() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!inFlight_) {
        return;
    }

    state_ = RegistrationState::RegisterFailed;
    inFlight_ = false;
    challengeSeen_ = false;
    cv_.notify_all();
}

void RegistrationService::OnIntervalTooBrief(int minExpires) {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingExpires_ = minExpires;
}

RegistrationState RegistrationService::State() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

int RegistrationService::PendingExpires() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingExpires_;
}

bool RegistrationService::TriggerRegisterAttemptLocked(std::unique_lock<std::mutex>& lock) {
    if (inFlight_) {
        return false;
    }

    auto handler = sendRegisterHandler_;
    if (!handler) {
        state_ = RegistrationState::RegisterFailed;
        return false;
    }

    state_ = RegistrationState::Registering;
    inFlight_ = true;
    challengeSeen_ = false;
    lastAttemptAt_ = std::chrono::steady_clock::now();

    lock.unlock();
    try {
        handler();
    }
    catch (...) {
        lock.lock();
        state_ = RegistrationState::RegisterFailed;
        inFlight_ = false;
        challengeSeen_ = false;
        cv_.notify_all();
        return false;
    }
    lock.lock();

    return true;
}

void RegistrationService::RetryLoop() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (!stop_.load()) {
        cv_.wait_for(lock, std::chrono::milliseconds(50));
        if (stop_.load()) {
            break;
        }

        if (state_ != RegistrationState::RegisterFailed || inFlight_) {
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        if ((now - lastAttemptAt_) < retryInterval_) {
            continue;
        }

        TriggerRegisterAttemptLocked(lock);
    }
}

} // namespace registration
} // namespace sim
