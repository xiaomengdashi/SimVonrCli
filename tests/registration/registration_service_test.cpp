#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "registration/registration_service.hpp"

TEST(RegistrationServiceTest, MoveToRegisteredOn200) {
    sim::registration::RegistrationService service;
    service.SetSendRegisterHandler([]() {});

    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Idle);

    service.StartRegister();
    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registering);

    service.OnResponse(200, sim::sip::SipMethod::Register, {});

    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registered);
}

TEST(RegistrationServiceTest, Handle423WithRetryExpires) {
    sim::registration::RegistrationService service;
    service.SetSendRegisterHandler([]() {});

    service.StartRegister();
    service.OnIntervalTooBrief(600);

    EXPECT_EQ(service.PendingExpires(), 600);
}

TEST(RegistrationServiceTest, SendImmediatelyAndAvoidDuplicateInflightRequests) {
    sim::registration::RegistrationService service(std::chrono::milliseconds(1000));
    int send_count = 0;
    service.SetSendRegisterHandler([&send_count]() {
        ++send_count;
    });

    EXPECT_TRUE(service.StartRegister());
    EXPECT_FALSE(service.StartRegister());
    EXPECT_EQ(send_count, 1);
}

TEST(RegistrationServiceTest, MarkFailureAfterSecondChallengeResponse) {
    sim::registration::RegistrationService service;
    service.SetSendRegisterHandler([]() {});

    EXPECT_TRUE(service.StartRegister());
    service.OnResponse(401, sim::sip::SipMethod::Register, {});
    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registering);

    service.OnResponse(401, sim::sip::SipMethod::Register, {});
    EXPECT_EQ(service.State(), sim::registration::RegistrationState::RegisterFailed);
}

TEST(RegistrationServiceTest, RetryOneSecondAfterTransportError) {
    sim::registration::RegistrationService service(std::chrono::milliseconds(150));
    std::atomic<int> send_count{0};
    service.SetSendRegisterHandler([&send_count]() {
        ++send_count;
    });

    EXPECT_TRUE(service.StartRegister());
    service.OnTransportError();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
    while (std::chrono::steady_clock::now() < deadline && send_count.load() < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_GE(send_count.load(), 2);
    EXPECT_EQ(service.State(), sim::registration::RegistrationState::Registering);
}
