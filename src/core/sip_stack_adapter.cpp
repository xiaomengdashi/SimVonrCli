#include "core/sip_stack_adapter.hpp"

#include <tinysak/tsk_debug.h>
#include <tinysak/tsk_memory.h>
#include <tinysip/tsip.h>
#include <tinysak/tsk_object.h>
#include <tinysip/tinysip/api/tsip_api_common.h>
#include <tinysip/tinysip/api/tsip_api_invite.h>
#include <tinysip/tinysip/api/tsip_api_register.h>
#include <tinysip/tinysip/headers/tsip_header.h>
#include <tinysip/tinysip/headers/tsip_header_Proxy_Authenticate.h>
#include <tinysip/tinysip/headers/tsip_header_WWW_Authenticate.h>
#include <tinysip/tinysip/tsip_action.h>
#include <tinysip/tinysip/tsip_event.h>
#include <tinysip/tinysip/tsip_message.h>
#include <tinymedia/tmedia_common.h>

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace sim {
namespace sip {

namespace {

std::string ToLower(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

uint16_t ParseAmf(const std::string& value) {
    if (value.empty()) {
        return 0x8000;
    }
    const auto parsed = static_cast<unsigned long>(std::stoul(value, nullptr, 16));
    return static_cast<uint16_t>(parsed & 0xFFFF);
}

std::string DecodeHexBytes(const std::string& value) {
    std::string hex_value = value;
    if (hex_value.rfind("0x", 0) == 0 || hex_value.rfind("0X", 0) == 0) {
        hex_value.erase(0, 2);
    }
    if (hex_value.size() != 32) {
        throw std::runtime_error("AKA K must be 16 bytes encoded as 32 hex chars");
    }

    std::string decoded;
    decoded.reserve(16);
    for (std::size_t i = 0; i < hex_value.size(); i += 2) {
        const auto hi = static_cast<unsigned char>(hex_value[i]);
        const auto lo = static_cast<unsigned char>(hex_value[i + 1]);
        if (!std::isxdigit(hi) || !std::isxdigit(lo)) {
            throw std::runtime_error("AKA K must be valid hex");
        }
        const auto byte = static_cast<char>(std::stoul(hex_value.substr(i, 2), nullptr, 16));
        decoded.push_back(byte);
    }
    return decoded;
}

bool FillChallengeFromHeader(const tsip_header_t* header, Challenge& out) {
    if (!header) {
        return false;
    }

    if (header->type == tsip_htype_WWW_Authenticate) {
        const auto* h = reinterpret_cast<const tsip_header_WWW_Authenticate_t*>(header);
        out.scheme = h->scheme ? h->scheme : "Digest";
        out.algorithm = h->algorithm ? h->algorithm : "";
        out.realm = h->realm ? h->realm : "";
        out.nonce = h->nonce ? h->nonce : "";
        return true;
    }

    if (header->type == tsip_htype_Proxy_Authenticate) {
        const auto* h = reinterpret_cast<const tsip_header_Proxy_Authenticate_t*>(header);
        out.scheme = h->scheme ? h->scheme : "Digest";
        out.algorithm = h->algorithm ? h->algorithm : "";
        out.realm = h->realm ? h->realm : "";
        out.nonce = h->nonce ? h->nonce : "";
        return true;
    }

    return false;
}

bool TryExtractChallenge(const tsip_message_t* message, Challenge& out) {
    if (!message) {
        return false;
    }

    const auto* www = tsip_message_get_header(message, tsip_htype_WWW_Authenticate);
    if (FillChallengeFromHeader(www, out)) {
        return true;
    }

    const auto* proxy = tsip_message_get_header(message, tsip_htype_Proxy_Authenticate);
    return FillChallengeFromHeader(proxy, out);
}

SipMethod ToSipMethod(const tsip_message_t* message) {
    if (!message || !message->CSeq || !message->CSeq->method) {
        return SipMethod::Unknown;
    }

    const std::string method{message->CSeq->method};
    if (method == "REGISTER") {
        return SipMethod::Register;
    }
    if (method == "INVITE") {
        return SipMethod::Invite;
    }
    if (method == "PRACK") {
        return SipMethod::Prack;
    }
    if (method == "ACK") {
        return SipMethod::Ack;
    }
    if (method == "BYE") {
        return SipMethod::Bye;
    }
    if (method == "CANCEL") {
        return SipMethod::Cancel;
    }
    return SipMethod::Unknown;
}
} // namespace

SipStackAdapter::SipStackAdapter() {
    tsk_debug_set_level(DEBUG_LEVEL_INFO);
}

SipStackAdapter::~SipStackAdapter() {
    if (callSession_) {
        auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
        TSK_OBJECT_SAFE_FREE(session);
        callSession_ = nullptr;
    }

    if (stack_) {
        auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
        tsip_stack_stop(stack);
        TSK_OBJECT_SAFE_FREE(stack);
        stack_ = nullptr;
    }
}

void SipStackAdapter::SetIdentity(const std::string& realm,
                                  const std::string& impi,
                                  const std::string& impu,
                                  const std::string& authMode,
                                  const std::string& digestPassword,
                                  const std::string& aka_opc,
                                  const std::string& aka_ki,
                                  const std::string& akaAmf,
                                  const std::string& akaSqn) {
    realm_ = realm;
    impi_ = impi;
    impu_ = impu;
    authMode_ = ToLower(authMode.empty() ? "aka" : authMode);
    digestPassword_ = digestPassword;
    akaOpc_ = aka_opc;
    akaKi_ = (authMode_ == "aka") ? DecodeHexBytes(aka_ki) : aka_ki;
    akaAmf_ = akaAmf;
    akaSqn_ = akaSqn;

    if (authMode_ != "aka" && authMode_ != "digest") {
        throw std::runtime_error("auth.mode must be aka or digest");
    }
}

void SipStackAdapter::InitializeUdp(const std::string& bindIp,
                                    int bindPort,
                                    const std::string& pcscfHost,
                                    int pcscfPort) {
    bindIp_ = bindIp;
    bindPort_ = bindPort;
    pcscfHost_ = pcscfHost;
    pcscfPort_ = pcscfPort;

    if (realm_.empty() || impi_.empty() || impu_.empty()) {
        throw std::runtime_error("SIP identity is not configured");
    }

    if (callSession_) {
        auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
        TSK_OBJECT_SAFE_FREE(session);
        callSession_ = nullptr;
    }

    if (stack_) {
        auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
        tsip_stack_stop(stack);
        TSK_OBJECT_SAFE_FREE(stack);
        stack_ = nullptr;
    }

    activeAuthPath_ = (authMode_ == "aka") ? AuthPath::AKA : AuthPath::Digest;
    registerFallbackUsed_ = false;
    registerChallengeRetried_ = false;

    stack_ = tsip_stack_create(
        &SipStackAdapter::OnTsipEvent,
        realm_.c_str(),
        impi_.c_str(),
        impu_.c_str(),
        TSIP_STACK_SET_LOCAL_IP(bindIp_.c_str()),
        TSIP_STACK_SET_LOCAL_PORT(static_cast<unsigned>(bindPort_)),
        TSIP_STACK_SET_PROXY_CSCF(pcscfHost_.c_str(), static_cast<unsigned>(pcscfPort_), "udp", "ipv4"),
        TSIP_STACK_SET_USERDATA(this),
        TSIP_STACK_SET_NULL());

    if (!stack_) {
        throw std::runtime_error("tsip_stack_create failed");
    }

    auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
    ApplyAuthToRunningStack();
    if (tsip_stack_start(stack) != 0) {
        TSK_OBJECT_SAFE_FREE(stack);
        stack_ = nullptr;
        throw std::runtime_error("tsip_stack_start failed");
    }
}

void SipStackAdapter::ApplyAuthToRunningStack() {
    if (!stack_) {
        return;
    }

    auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
    if (activeAuthPath_ == AuthPath::AKA) {
        tsip_stack_set(
            stack,
            TSIP_STACK_SET_PASSWORD(akaKi_.c_str()),
            TSIP_STACK_SET_IMS_AKA_OPERATOR_ID(akaOpc_.c_str()),
            TSIP_STACK_SET_IMS_AKA_AMF(ParseAmf(akaAmf_)),
            TSIP_STACK_SET_NULL());
    }
    else {
        tsip_stack_set(
            stack,
            TSIP_STACK_SET_PASSWORD(digestPassword_.c_str()),
            TSIP_STACK_SET_NULL());
    }
}

bool SipStackAdapter::HasAkaMaterial() const {
    return !akaOpc_.empty() && !akaKi_.empty();
}

void SipStackAdapter::SendRegister(bool resetAuthState) {
    if (!stack_) {
        throw std::runtime_error("SIP stack is not initialized");
    }

    if (resetAuthState) {
        registerFallbackUsed_ = false;
        registerChallengeRetried_ = false;
    }

    auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
    tsip_ssession_handle_t* session = tsip_ssession_create(stack, TSIP_SSESSION_SET_NULL());
    if (!session) {
        throw std::runtime_error("tsip_ssession_create failed");
    }

    const int ret = tsip_api_register_send_register(
        session,
        TSIP_ACTION_SET_NULL());

    TSK_OBJECT_SAFE_FREE(session);

    if (ret != 0) {
        throw std::runtime_error("tsip_api_register_send_register failed");
    }
}

void SipStackAdapter::SetEventHandler(Handler h) {
    handler_ = std::move(h);
}

void SipStackAdapter::SendRaw(const std::string& data) {
    if (!stack_) {
        throw std::runtime_error("SIP stack is not initialized");
    }

    if (data == "REGISTER") {
        SendRegister(true);
    }
}

void SipStackAdapter::StartCall(const std::string& targetUri) {
    if (!stack_) {
        throw std::runtime_error("SIP stack is not initialized");
    }
    if (targetUri.empty()) {
        throw std::runtime_error("target URI is empty");
    }
    if (callSession_) {
        throw std::runtime_error("call already in progress");
    }

    inviteFallbackUsed_ = false;
    inviteChallengeRetried_ = false;
    activeCallTargetUri_ = targetUri;

    auto* stack = static_cast<tsip_stack_handle_t*>(stack_);
    tsip_ssession_handle_t* session = tsip_ssession_create(
        stack,
        TSIP_SSESSION_SET_TO_STR(targetUri.c_str()),
        TSIP_SSESSION_SET_NULL());
    if (!session) {
        throw std::runtime_error("tsip_ssession_create failed");
    }

    const int ret = tsip_api_invite_send_invite(
        session,
        tmedia_none,
        TSIP_ACTION_SET_NULL());
    if (ret != 0) {
        TSK_OBJECT_SAFE_FREE(session);
        throw std::runtime_error("tsip_api_invite_send_invite failed");
    }

    callSession_ = session;
}

void SipStackAdapter::AnswerCall() {
    if (!callSession_) {
        throw std::runtime_error("no call session to answer");
    }

    auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
    if (tsip_api_common_accept(session, TSIP_ACTION_SET_NULL()) != 0) {
        throw std::runtime_error("tsip_api_common_accept failed");
    }
}

void SipStackAdapter::HangupCall() {
    if (!callSession_) {
        throw std::runtime_error("no call session to hang up");
    }

    auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
    if (tsip_api_common_hangup(session, TSIP_ACTION_SET_NULL()) != 0) {
        throw std::runtime_error("tsip_api_common_hangup failed");
    }

    TSK_OBJECT_SAFE_FREE(session);
    callSession_ = nullptr;
    inviteChallengeRetried_ = false;
    inviteFallbackUsed_ = false;
    activeCallTargetUri_.clear();
}

void SipStackAdapter::InjectTestEvent(const SipEvent& event) {
    if (handler_) {
        handler_(event);
    }
}

int SipStackAdapter::OnTsipEvent(const ::tsip_event_s* event) {
    if (!event) {
        return -1;
    }

    const auto* self = static_cast<const SipStackAdapter*>(event->userdata);
    if (!self) {
        return 0;
    }

    const_cast<SipStackAdapter*>(self)->HandleTsipEvent(event);
    return 0;
}

void SipStackAdapter::HandleTsipEvent(const ::tsip_event_s* event) {
    if (!event) {
        return;
    }

    const auto* message = static_cast<const tsip_message_t*>(event->sipmessage);
    const auto method = ToSipMethod(message);

    if (message && !TSIP_MESSAGE_IS_RESPONSE(message) && method == SipMethod::Invite && event->ss && !callSession_) {
        callSession_ = tsk_object_ref(event->ss);
    }

    if (event->code == tsip_event_code_dialog_terminated && callSession_) {
        auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
        TSK_OBJECT_SAFE_FREE(session);
        callSession_ = nullptr;
        inviteChallengeRetried_ = false;
        inviteFallbackUsed_ = false;
        activeCallTargetUri_.clear();
    }

    if (message && TSIP_MESSAGE_IS_RESPONSE(message) && method == SipMethod::Register) {
        const int status = TSIP_RESPONSE_CODE(message);
        if (status == 200) {
            registerChallengeRetried_ = false;
        }
        else if ((status == 401 || status == 407) && !registerChallengeRetried_) {
            Challenge challenge;
            if (TryExtractChallenge(message, challenge)) {
                try {
                    const auto authResult = authService_.BuildAuthorization(
                        challenge,
                        AuthContext{
                            .hasAkaMaterial = HasAkaMaterial(),
                            .fallbackUsed = registerFallbackUsed_,
                            .digestPassword = digestPassword_,
                        });

                    if (authResult.path == AuthPath::Digest && activeAuthPath_ != AuthPath::Digest) {
                        registerFallbackUsed_ = true;
                    }

                    if (activeAuthPath_ != authResult.path) {
                        activeAuthPath_ = authResult.path;
                        ApplyAuthToRunningStack();
                    }

                    registerChallengeRetried_ = true;
                    SendRegister(false);
                }
                catch (...) {
                }
            }
        }
    }

    if (message && TSIP_MESSAGE_IS_RESPONSE(message) && method == SipMethod::Invite && callSession_) {
        const int status = TSIP_RESPONSE_CODE(message);
        if (status >= 200 && status < 300) {
            inviteChallengeRetried_ = false;
        }
        else if ((status == 401 || status == 407) && !inviteChallengeRetried_) {
            Challenge challenge;
            if (TryExtractChallenge(message, challenge)) {
                try {
                    const auto authResult = authService_.BuildAuthorization(
                        challenge,
                        AuthContext{
                            .hasAkaMaterial = HasAkaMaterial(),
                            .fallbackUsed = inviteFallbackUsed_,
                            .digestPassword = digestPassword_,
                        });

                    if (authResult.path == AuthPath::Digest && activeAuthPath_ != AuthPath::Digest) {
                        inviteFallbackUsed_ = true;
                    }

                    if (activeAuthPath_ != authResult.path) {
                        activeAuthPath_ = authResult.path;
                        ApplyAuthToRunningStack();
                    }

                    auto* session = static_cast<tsip_ssession_handle_t*>(callSession_);
                    if (tsip_api_invite_send_invite(session, tmedia_none, TSIP_ACTION_SET_NULL()) == 0) {
                        inviteChallengeRetried_ = true;
                    }
                }
                catch (...) {
                }
            }
        }
    }

    if (!handler_) {
        return;
    }

    SipEvent out{};

    if (message) {
        out.method = method;
        if (message->Call_ID && message->Call_ID->value) {
            out.callId = message->Call_ID->value;
        }

        if (TSIP_MESSAGE_IS_RESPONSE(message)) {
            out.type = SipEventType::IncomingResponse;
            out.statusCode = TSIP_RESPONSE_CODE(message);
        }
        else {
            out.type = SipEventType::IncomingRequest;
            out.statusCode = 0;
        }
    }
    else {
        out.type = SipEventType::TransportError;
        out.statusCode = event->code;
    }

    out.raw = event->phrase ? event->phrase : "";
    handler_(out);
}

} // namespace sip
} // namespace sim
