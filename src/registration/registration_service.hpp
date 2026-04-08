#pragma once

#include "core/sip_types.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

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
    explicit RegistrationService(std::chrono::milliseconds retryInterval = std::chrono::seconds(1));
    ~RegistrationService();

    RegistrationService(const RegistrationService&) = delete;
    RegistrationService& operator=(const RegistrationService&) = delete;

    void SetSendRegisterHandler(std::function<void()> handler);
    bool StartRegister();
    void StartUnregister();
    void OnResponse(int statusCode, sip::SipMethod method, const std::map<std::string, std::string>& headers);
    void OnTransportError();
    void OnIntervalTooBrief(int minExpires);

    RegistrationState State() const;
    int PendingExpires() const;

private:
    bool TriggerRegisterAttemptLocked(std::unique_lock<std::mutex>& lock);
    void RetryLoop();

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::function<void()> sendRegisterHandler_;
    RegistrationState state_{RegistrationState::Idle};
    int pendingExpires_{300};
    bool inFlight_{false};
    bool challengeSeen_{false};
    std::chrono::steady_clock::time_point lastAttemptAt_{};
    std::chrono::milliseconds retryInterval_;
    std::atomic<bool> stop_{false};
    std::thread retryThread_;
};

} // namespace registration
} // namespace sim
