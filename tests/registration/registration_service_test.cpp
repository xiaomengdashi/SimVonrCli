#include <map>
#include <string>

#include <gtest/gtest.h>

#include "registration/registration_service.hpp"

TEST(RegistrationServiceTest, MoveToRegisteredOn200) {
    sim::registration::RegistrationService service;

    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Idle);

    service.StartRegister();
    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registering);

    service.OnResponse(200, sim::sip::SipMethod::Register, {});

    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registered);
}

TEST(RegistrationServiceTest, Handle423WithRetryExpires) {
    sim::registration::RegistrationService service;

    service.StartRegister();
    service.OnIntervalTooBrief(600);

    EXPECT_EQ(service.PendingExpires(), 600);
}
