#pragma once

#include "auth/ims_auth_service.hpp"
#include "core/sip_types.hpp"

#include <functional>
#include <string>

struct tsip_event_s;

namespace sim {
namespace sip {

class SipStackAdapter {
public:
    using Handler = std::function<void(const SipEvent&)>;

    SipStackAdapter();
    ~SipStackAdapter();

    SipStackAdapter(const SipStackAdapter&) = delete;
    SipStackAdapter& operator=(const SipStackAdapter&) = delete;

    void InitializeUdp(const std::string& bindIp, int bindPort, const std::string& pcscfHost, int pcscfPort);
    void SetIdentity(const std::string& realm,
                     const std::string& impi,
                     const std::string& impu,
                     const std::string& authMode,
                     const std::string& digestPassword,
                     const std::string& aka_opc,
                     const std::string& aka_ki,
                     const std::string& akaAmf,
                     const std::string& akaSqn);
    void SetEventHandler(Handler h);
    void SendRaw(const std::string& data);
    void StartCall(const std::string& targetUri);
    void AnswerCall();
    void HangupCall();

    void InjectTestEvent(const SipEvent& event);

private:
    static int OnTsipEvent(const ::tsip_event_s* event);
    void HandleTsipEvent(const ::tsip_event_s* event);
    void SendRegister(bool resetAuthState);
    void ApplyAuthToRunningStack();
    bool HasAkaMaterial() const;

    Handler handler_;
    void* stack_{nullptr};
    void* callSession_{nullptr};

    std::string bindIp_;
    int bindPort_{};
    std::string pcscfHost_;
    int pcscfPort_{};

    std::string realm_;
    std::string impi_;
    std::string impu_;
    std::string authMode_{"aka"};
    std::string digestPassword_;
    std::string akaOpc_;
    std::string akaKi_;
    std::string akaAmf_;
    std::string akaSqn_;

    AuthPath activeAuthPath_{AuthPath::AKA};
    bool registerFallbackUsed_{false};
    bool registerChallengeRetried_{false};
    bool inviteFallbackUsed_{false};
    bool inviteChallengeRetried_{false};
    std::string activeCallTargetUri_;
    ImsAuthService authService_;
};

} // namespace sip
} // namespace sim
