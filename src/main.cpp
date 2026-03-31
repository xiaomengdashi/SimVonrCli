#include <iostream>
#include <string>

#include "cli/cli_controller.hpp"

int main() {
    sim::cli::CliController controller;
    std::string line;

    std::cout << "sim_vonr_cli ready" << std::endl;
    while (std::getline(std::cin, line)) {
        const auto cmd = controller.Parse(line);
        if (cmd.type == sim::cli::CliCommandType::Quit) {
            break;
        }
        std::cout << "cmd=" << static_cast<int>(cmd.type) << " arg=" << cmd.arg << std::endl;
    }

    return 0;
}
