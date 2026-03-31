#include <gtest/gtest.h>

#include "core/sip_stack_adapter.hpp"

TEST(SipStackAdapterTest, DispatchIncomingResponseToCallback) {
    sim::sip::SipStackAdapter adapter;
    bool called = false;

    adapter.SetEventHandler([&](const sim::sip::SipEvent& event) {
        called = (event.type == sim::sip::SipEventType::IncomingResponse &&
                  event.method == sim::sip::SipMethod::Register &&
                  event.statusCode == 200);
    });

    adapter.InjectTestEvent({
        .type = sim::sip::SipEventType::IncomingResponse,
        .method = sim::sip::SipMethod::Register,
        .statusCode = 200,
        .callId = "cid",
        .raw = "raw",
    });

    EXPECT_TRUE(called);
}
