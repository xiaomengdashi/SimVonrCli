#include <gtest/gtest.h>

#include "call/call_service.hpp"

TEST(CallServiceTest, UacFlowWithPrackThenAck) {
    sim::call::CallService service;

    service.StartOutgoing("sip:bob@ims.test");
    EXPECT_EQ(service.State(), sim::call::CallState::OutgoingInvite);

    service.OnProvisional(183, true, 1, 1);
    EXPECT_TRUE(service.ShouldSendPrack());

    service.OnInvite200();
    EXPECT_TRUE(service.ShouldSendAck());
    EXPECT_EQ(service.State(), sim::call::CallState::Confirmed);
}

TEST(CallServiceTest, RejectSecondIncomingWhenBusy) {
    sim::call::CallService service;

    service.StartOutgoing("sip:bob@ims.test");

    EXPECT_EQ(service.OnIncomingInviteWhenBusy(), 486);
}
