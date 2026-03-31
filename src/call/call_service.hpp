#pragma once

#include <string>

namespace sim {
namespace call {

enum class CallState {
    Idle,
    OutgoingInvite,
    IncomingInvite,
    EarlyDialog,
    Confirmed,
    Terminating,
};

class CallService {
public:
    void StartOutgoing(const std::string& targetUri);
    void OnIncomingInvite();
    void OnProvisional(int statusCode, bool reliable, int rseq, int cseq);
    void OnInvite200();
    void OnBye();

    int OnIncomingInviteWhenBusy() const;
    bool ShouldSendPrack() const;
    bool ShouldSendAck() const;
    CallState State() const;

private:
    CallState state_{CallState::Idle};
    bool shouldSendPrack_{false};
    bool shouldSendAck_{false};
};

} // namespace call
} // namespace sim
