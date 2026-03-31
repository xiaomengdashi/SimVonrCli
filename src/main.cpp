#include <iostream>

#include "core/sip_types.hpp"

int main() {
    const sim::sip::SipRequest request{
        .method = sim::sip::SipMethod::Invite,
        .uri = "sip:bob@example.com",
        .from = "sip:alice@example.com",
        .to = "sip:bob@example.com",
        .call_id = "bootstrap-call-id"
    };

    std::cout << "sim-vonr-cli bootstrap running" << '\n';
    std::cout << "sample method: " << sim::sip::to_string(request.method) << '\n';

    return 0;
}
