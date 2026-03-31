#include "core/sip_stack_adapter.hpp"

#include <utility>

namespace sim {
namespace sip {

void SipStackAdapter::InitializeUdp(const std::string&, int, const std::string&, int) {}

void SipStackAdapter::SetEventHandler(Handler h) {
    handler_ = std::move(h);
}

void SipStackAdapter::SendRaw(const std::string&) {}

void SipStackAdapter::InjectTestEvent(const SipEvent& event) {
    if (handler_) {
        handler_(event);
    }
}

} // namespace sip
} // namespace sim
