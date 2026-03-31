#include <gtest/gtest.h>

#include "cli/cli_controller.hpp"

TEST(CliControllerTest, ParseRegisterCommand) {
    sim::cli::CliController controller;

    auto cmd = controller.Parse("register");

    EXPECT_EQ(cmd.type, sim::cli::CliCommandType::Register);
}

TEST(CliControllerTest, ParseCallCommand) {
    sim::cli::CliController controller;

    auto cmd = controller.Parse("call sip:bob@ims.test");

    EXPECT_EQ(cmd.type, sim::cli::CliCommandType::Call);
    EXPECT_EQ(cmd.arg, "sip:bob@ims.test");
}
