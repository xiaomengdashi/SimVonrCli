#include <gtest/gtest.h>

#include "core/sip_types.hpp"
#include "registration/registration_service.hpp"
#include "call/call_service.hpp"

TEST(SipFlowSmokeTest, RegisterThenCallLifecycleStateTransitions) {
    sim::registration::RegistrationService registration;
    sim::call::CallService call;

    registration.StartRegister();
    registration.OnResponse(200, sim::sip::SipMethod::Register, {});
    ASSERT_EQ(registration.State(), sim::registration::RegistrationState::Registered);

    call.StartOutgoing("sip:bob@ims.test");
    call.OnProvisional(183, true, 1, 1);
    ASSERT_TRUE(call.ShouldSendPrack());

    call.OnInvite200();
    ASSERT_EQ(call.State(), sim::call::CallState::Confirmed);

    call.OnBye();
    ASSERT_EQ(call.State(), sim::call::CallState::Idle);
}
