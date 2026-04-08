#include <iostream>
#include <string>

#include "cli/cli_controller.hpp"
#include "config/config.hpp"
#include "core/sip_stack_adapter.hpp"
#include "registration/registration_service.hpp"

int main(int argc, char** argv) {
    const std::string config_path = (argc > 1) ? argv[1] : "configs/config.example.yaml";

    try {
        const auto cfg = sim::config::load_config(config_path);

        sim::sip::SipStackAdapter sip_adapter;
        sim::registration::RegistrationService registration;
        bool auto_answer = cfg.sip.auto_answer;
        registration.SetSendRegisterHandler([&sip_adapter]() {
            sip_adapter.SendRaw("REGISTER");
        });
        sip_adapter.SetIdentity(
            cfg.identity.realm,
            cfg.identity.impi,
            cfg.identity.impu,
            cfg.auth.mode,
            cfg.auth.digest.password,
            cfg.auth.aka.opc,
            cfg.auth.aka.ki,
            cfg.auth.aka.amf,
            cfg.auth.aka.sqn);
        sip_adapter.SetEventHandler([&sip_adapter, &registration, &auto_answer](const sim::sip::SipEvent& event) {
            std::cout << "[sip-event] type=" << static_cast<int>(event.type)
                      << " method=" << sim::sip::to_string(event.method)
                      << " status=" << event.statusCode
                      << " callId=" << event.callId
                      << " phrase=" << event.raw
                      << std::endl;

            if (event.type == sim::sip::SipEventType::IncomingResponse &&
                event.method == sim::sip::SipMethod::Register) {
                registration.OnResponse(event.statusCode, event.method, {});
            }
            else if (event.type == sim::sip::SipEventType::TransportError &&
                     registration.State() == sim::registration::RegistrationState::Registering) {
                registration.OnTransportError();
            }

            if (auto_answer &&
                event.type == sim::sip::SipEventType::IncomingRequest &&
                event.method == sim::sip::SipMethod::Invite) {
                try {
                    sip_adapter.AnswerCall();
                    std::cout << "auto-answer: 200 OK sent" << std::endl;
                }
                catch (const std::exception& ex) {
                    std::cerr << "auto-answer failed: " << ex.what() << std::endl;
                }
            }
        });
        sip_adapter.InitializeUdp(
            cfg.local.bind_ip,
            cfg.local.bind_port,
            cfg.pcscf.host,
            cfg.pcscf.port);
        if (registration.StartRegister()) {
            std::cout << "REGISTER sent" << std::endl;
        }

        sim::cli::CliController controller;
        std::string line;

        std::cout << "sim_vonr_cli ready" << std::endl;
        while (std::getline(std::cin, line)) {
            const auto cmd = controller.Parse(line);
            if (cmd.type == sim::cli::CliCommandType::Quit) {
                break;
            }

            if (cmd.type == sim::cli::CliCommandType::Register) {
                if (registration.StartRegister()) {
                    std::cout << "REGISTER sent" << std::endl;
                }
                continue;
            }

            if (cmd.type == sim::cli::CliCommandType::Call) {
                sip_adapter.StartCall(cmd.arg);
                std::cout << "INVITE sent to " << cmd.arg << std::endl;
                continue;
            }

            if (cmd.type == sim::cli::CliCommandType::Answer) {
                sip_adapter.AnswerCall();
                std::cout << "200 OK sent" << std::endl;
                continue;
            }

            if (cmd.type == sim::cli::CliCommandType::Hangup) {
                sip_adapter.HangupCall();
                std::cout << "BYE sent" << std::endl;
                continue;
            }

            if (cmd.type == sim::cli::CliCommandType::AutoAnswer) {
                auto_answer = (cmd.arg == "on");
                std::cout << "autoanswer " << (auto_answer ? "on" : "off") << std::endl;
                continue;
            }

            std::cout << "cmd=" << static_cast<int>(cmd.type) << " arg=" << cmd.arg << std::endl;
        }

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "fatal: " << ex.what() << std::endl;
        return 1;
    }
}
