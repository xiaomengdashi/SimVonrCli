#pragma once

#include "core/sip_types.hpp"

#include <functional>
#include <string>

namespace sim {
namespace sip {

class SipStackAdapter {
public:
    using Handler = std::function<void(const SipEvent&)>;

    void InitializeUdp(const std::string& bindIp, int bindPort, const std::string& pcscfHost, int pcscfPort);
    void SetEventHandler(Handler h);
    void SendRaw(const std::string& data);

    void InjectTestEvent(const SipEvent& event);

private:
    Handler handler_;
};

} // namespace sip
} // namespace sim
