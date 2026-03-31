#include "call/call_service.hpp"

namespace sim {
namespace call {

void CallService::StartOutgoing(const std::string&) {
    state_ = CallState::OutgoingInvite;
    shouldSendPrack_ = false;
    shouldSendAck_ = false;
}

void CallService::OnIncomingInvite() {
    if (state_ == CallState::Idle) {
        state_ = CallState::IncomingInvite;
    }
}

void CallService::OnProvisional(int, bool reliable, int, int) {
    if (state_ == CallState::OutgoingInvite || state_ == CallState::IncomingInvite) {
        state_ = CallState::EarlyDialog;
    }
    if (reliable) {
        shouldSendPrack_ = true;
    }
}

void CallService::OnInvite200() {
    shouldSendAck_ = true;
    state_ = CallState::Confirmed;
}

void CallService::OnBye() {
    state_ = CallState::Idle;
    shouldSendPrack_ = false;
    shouldSendAck_ = false;
}

int CallService::OnIncomingInviteWhenBusy() const {
    return state_ == CallState::Idle ? 0 : 486;
}

bool CallService::ShouldSendPrack() const {
    return shouldSendPrack_;
}

bool CallService::ShouldSendAck() const {
    return shouldSendAck_;
}

CallState CallService::State() const {
    return state_;
}

} // namespace call
} // namespace sim
