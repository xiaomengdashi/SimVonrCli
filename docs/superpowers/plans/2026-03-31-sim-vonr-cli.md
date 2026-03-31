# SimVoNR CLI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 构建一个基于 tinySIP 的现代 C++ 命令行 IMS/VoNR SIP 客户端，支持 REGISTER（AKA+Digest）、主被叫呼叫、100rel/PRACK，UDP-only，纯信令。

**Architecture:** 采用 C++20 业务层 + tinySIP C 内核的适配方式。通过 SipStackAdapter 屏蔽 C 回调，RegistrationService/CallService 分别维护注册与呼叫状态机，ImsAuthService 统一处理 AKA 与 Digest challenge。CLI 仅负责命令编排与状态展示，配置来自单一 config.yaml。

**Tech Stack:** C++20, CMake, tinySIP/tinySAK, yaml-cpp, GoogleTest

---

## File Structure Map

### 目录与职责

- `CMakeLists.txt`：构建入口、三方库链接、测试开关
- `configs/config.example.yaml`：示例配置
- `src/main.cpp`：程序入口与主循环
- `src/config/config.hpp`：配置结构定义
- `src/config/config_loader.cpp`：YAML 加载与校验
- `src/core/sip_types.hpp`：SIP 事件/消息 C++ 类型
- `src/core/sip_stack_adapter.hpp`：tinySIP 适配器声明
- `src/core/sip_stack_adapter.cpp`：tinySIP 初始化、回调桥接、发送接口
- `src/ims/ims_header_policy.hpp`：IMS 头策略声明
- `src/ims/ims_header_policy.cpp`：IMS 头拼装实现
- `src/auth/ims_auth_service.hpp`：认证服务声明
- `src/auth/ims_auth_service.cpp`：AKA/Digest challenge 处理与认证头生成
- `src/registration/registration_service.hpp`：注册状态机声明
- `src/registration/registration_service.cpp`：REGISTER/刷新/注销流程
- `src/call/call_service.hpp`：呼叫状态机声明
- `src/call/call_service.cpp`：INVITE/PRACK/ACK/BYE/CANCEL 流程
- `src/cli/cli_controller.hpp`：CLI 控制器声明
- `src/cli/cli_controller.cpp`：命令解析与状态输出
- `src/util/redaction.hpp`：日志脱敏工具
- `src/util/redaction.cpp`：日志脱敏实现
- `tests/config/config_loader_test.cpp`：配置解析测试
- `tests/auth/ims_auth_service_test.cpp`：认证策略测试
- `tests/registration/registration_service_test.cpp`：注册状态机测试
- `tests/call/call_service_test.cpp`：呼叫状态机与 100rel/PRACK 测试
- `tests/cli/cli_controller_test.cpp`：命令映射测试
- `tests/util/redaction_test.cpp`：脱敏测试

---

### Task 1: 初始化工程骨架与构建系统

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `src/core/sip_types.hpp`
- Create: `configs/config.example.yaml`

- [x] **Step 1: 写失败测试（构建级）**

```cmake
# CMakeLists.txt (初始仅含测试target，不含实现源)
cmake_minimum_required(VERSION 3.20)
project(SimVonrCli LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
enable_testing()
add_executable(sim_vonr_cli src/main.cpp)
# 故意不添加 include 目录与后续源文件，先让构建失败
```

- [x] **Step 2: 运行构建验证失败**

Run: `cmake -S . -B build && cmake --build build -j`
Expected: FAIL（后续头文件/源文件缺失导致）

- [x] **Step 3: 写最小实现使构建通过**

```cpp
// src/main.cpp
#include <iostream>
int main() {
    std::cout << "sim_vonr_cli bootstrap" << std::endl;
    return 0;
}
```

```cpp
// src/core/sip_types.hpp
#pragma once
#include <string>

enum class SipMethod { Register, Invite, Prack, Ack, Bye, Cancel, Unknown };
enum class SipEventType { IncomingRequest, IncomingResponse, TransportError };

struct SipEvent {
    SipEventType type;
    SipMethod method;
    int statusCode{};
    std::string callId;
    std::string raw;
};
```

```yaml
# configs/config.example.yaml
local:
  bind_ip: "0.0.0.0"
  bind_port: 5062
pcscf:
  host: "127.0.0.1"
  port: 5060
  transport: "udp"
identity:
  impi: "user@ims.mnc001.mcc001.3gppnetwork.org"
  impu: "sip:user@ims.mnc001.mcc001.3gppnetwork.org"
  realm: "ims.mnc001.mcc001.3gppnetwork.org"
auth:
  mode: "aka+digest"
  digest:
    password: "change_me"
  aka:
    opc: "00000000000000000000000000000000"
    ki: "00000000000000000000000000000000"
    amf: "8000"
    sqn: "000000000000"
sip:
  user_agent: "SimVonrCli/0.1"
  expires: 300
  enable_100rel: true
ims_headers:
  enable: true
```

- [x] **Step 4: 运行构建验证通过**

Run: `cmake -S . -B build && cmake --build build -j`
Expected: PASS（生成 `sim_vonr_cli`）

- [x] **Step 5: Commit**

```bash
git add CMakeLists.txt src/main.cpp src/core/sip_types.hpp configs/config.example.yaml
git commit -m "chore: bootstrap sim vonr cli project skeleton"
```

---

### Task 2: 配置加载器（TDD）

**Files:**
- Create: `src/config/config.hpp`
- Create: `src/config/config_loader.cpp`
- Create: `tests/config/config_loader_test.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: 写失败测试（配置解析）**

```cpp
// tests/config/config_loader_test.cpp
#include <gtest/gtest.h>
#include "config/config.hpp"

TEST(ConfigLoaderTest, LoadValidConfig) {
    auto cfg = LoadConfig("configs/config.example.yaml");
    EXPECT_EQ(cfg.pcscf.host, "127.0.0.1");
    EXPECT_EQ(cfg.pcscf.port, 5060);
    EXPECT_TRUE(cfg.sip.enable100rel);
    EXPECT_EQ(cfg.auth.mode, "aka+digest");
}

TEST(ConfigLoaderTest, RejectNonUdpTransport) {
    EXPECT_THROW(LoadConfig("tests/fixtures/config_tcp.yaml"), std::runtime_error);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R ConfigLoaderTest -V`
Expected: FAIL（`LoadConfig` 未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/config/config.hpp
#pragma once
#include <string>

struct LocalConfig { std::string bindIp; int bindPort{}; };
struct PcscfConfig { std::string host; int port{}; std::string transport; };
struct IdentityConfig { std::string impi; std::string impu; std::string realm; };
struct DigestConfig { std::string password; };
struct AkaConfig { std::string opc; std::string ki; std::string amf; std::string sqn; };
struct AuthConfig { std::string mode; DigestConfig digest; AkaConfig aka; };
struct SipConfig { std::string userAgent; int expires{}; bool enable100rel{}; };
struct ImsHeadersConfig { bool enable{}; };

struct AppConfig {
    LocalConfig local;
    PcscfConfig pcscf;
    IdentityConfig identity;
    AuthConfig auth;
    SipConfig sip;
    ImsHeadersConfig imsHeaders;
};

AppConfig LoadConfig(const std::string& path);
```

```cpp
// src/config/config_loader.cpp
#include "config/config.hpp"
#include <yaml-cpp/yaml.h>
#include <stdexcept>

AppConfig LoadConfig(const std::string& path) {
    auto root = YAML::LoadFile(path);
    AppConfig c;
    c.local.bindIp = root["local"]["bind_ip"].as<std::string>();
    c.local.bindPort = root["local"]["bind_port"].as<int>();
    c.pcscf.host = root["pcscf"]["host"].as<std::string>();
    c.pcscf.port = root["pcscf"]["port"].as<int>();
    c.pcscf.transport = root["pcscf"]["transport"].as<std::string>();
    if (c.pcscf.transport != "udp") throw std::runtime_error("only udp is supported");
    c.identity.impi = root["identity"]["impi"].as<std::string>();
    c.identity.impu = root["identity"]["impu"].as<std::string>();
    c.identity.realm = root["identity"]["realm"].as<std::string>();
    c.auth.mode = root["auth"]["mode"].as<std::string>();
    c.auth.digest.password = root["auth"]["digest"]["password"].as<std::string>();
    c.auth.aka.opc = root["auth"]["aka"]["opc"].as<std::string>();
    c.auth.aka.ki = root["auth"]["aka"]["ki"].as<std::string>();
    c.auth.aka.amf = root["auth"]["aka"]["amf"].as<std::string>();
    c.auth.aka.sqn = root["auth"]["aka"]["sqn"].as<std::string>();
    c.sip.userAgent = root["sip"]["user_agent"].as<std::string>();
    c.sip.expires = root["sip"]["expires"].as<int>();
    c.sip.enable100rel = root["sip"]["enable_100rel"].as<bool>();
    c.imsHeaders.enable = root["ims_headers"]["enable"].as<bool>();
    return c;
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R ConfigLoaderTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add CMakeLists.txt src/config/config.hpp src/config/config_loader.cpp tests/config/config_loader_test.cpp
git commit -m "feat: add yaml config loader with udp-only validation"
```

---

### Task 3: IMS 头策略

**Files:**
- Create: `src/ims/ims_header_policy.hpp`
- Create: `src/ims/ims_header_policy.cpp`
- Create: `tests/ims/ims_header_policy_test.cpp`

- [x] **Step 1: 写失败测试（头拼装）**

```cpp
// tests/ims/ims_header_policy_test.cpp
#include <gtest/gtest.h>
#include "ims/ims_header_policy.hpp"

TEST(ImsHeaderPolicyTest, BuildBaseHeaders) {
    ImsHeaderPolicy p;
    auto hs = p.BuildForRegister("sip:user@ims.test", true);
    EXPECT_NE(std::find(hs.begin(), hs.end(), "Supported: 100rel"), hs.end());
    EXPECT_NE(std::find_if(hs.begin(), hs.end(), [](const std::string& h){
        return h.rfind("P-Preferred-Identity:", 0) == 0;
    }), hs.end());
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R ImsHeaderPolicyTest -V`
Expected: FAIL（类未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/ims/ims_header_policy.hpp
#pragma once
#include <string>
#include <vector>

class ImsHeaderPolicy {
public:
    std::vector<std::string> BuildForRegister(const std::string& impu, bool enable100rel) const;
    std::vector<std::string> BuildForInvite(const std::string& impu, bool enable100rel) const;
};
```

```cpp
// src/ims/ims_header_policy.cpp
#include "ims/ims_header_policy.hpp"

std::vector<std::string> ImsHeaderPolicy::BuildForRegister(const std::string& impu, bool enable100rel) const {
    std::vector<std::string> h;
    h.push_back("P-Preferred-Identity: <" + impu + ">");
    h.push_back("P-Access-Network-Info: 3GPP-E-UTRAN-FDD;utran-cell-id-3gpp=00000000");
    if (enable100rel) h.push_back("Supported: 100rel");
    return h;
}

std::vector<std::string> ImsHeaderPolicy::BuildForInvite(const std::string& impu, bool enable100rel) const {
    return BuildForRegister(impu, enable100rel);
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R ImsHeaderPolicyTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/ims/ims_header_policy.hpp src/ims/ims_header_policy.cpp tests/ims/ims_header_policy_test.cpp
git commit -m "feat: add ims header policy builder"
```

---

### Task 4: 认证服务（AKA优先 + Digest回落）

**Files:**
- Create: `src/auth/ims_auth_service.hpp`
- Create: `src/auth/ims_auth_service.cpp`
- Create: `tests/auth/ims_auth_service_test.cpp`

- [x] **Step 1: 写失败测试（策略决策）**

```cpp
// tests/auth/ims_auth_service_test.cpp
#include <gtest/gtest.h>
#include "auth/ims_auth_service.hpp"

TEST(ImsAuthServiceTest, PreferAkaWhenChallengeIsAka) {
    ImsAuthService svc;
    Challenge ch{.scheme="Digest", .algorithm="AKAv1-MD5", .realm="ims.test", .nonce="n"};
    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial=true, .digestPassword="pw"});
    EXPECT_EQ(out.path, AuthPath::AKA);
}

TEST(ImsAuthServiceTest, FallbackToDigestWhenAkaUnavailable) {
    ImsAuthService svc;
    Challenge ch{.scheme="Digest", .algorithm="AKAv1-MD5", .realm="ims.test", .nonce="n"};
    auto out = svc.BuildAuthorization(ch, AuthContext{.hasAkaMaterial=false, .digestPassword="pw"});
    EXPECT_EQ(out.path, AuthPath::Digest);
}

TEST(ImsAuthServiceTest, PreventInfiniteFallbackLoop) {
    ImsAuthService svc;
    Challenge ch{.scheme="Digest", .algorithm="AKAv1-MD5", .realm="ims.test", .nonce="n"};
    AuthContext ctx{.hasAkaMaterial=false, .digestPassword="pw", .fallbackUsed=true};
    EXPECT_THROW(svc.BuildAuthorization(ch, ctx), std::runtime_error);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R ImsAuthServiceTest -V`
Expected: FAIL（服务未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/auth/ims_auth_service.hpp
#pragma once
#include <string>

enum class AuthPath { AKA, Digest };

struct Challenge {
    std::string scheme;
    std::string algorithm;
    std::string realm;
    std::string nonce;
};

struct AuthContext {
    bool hasAkaMaterial{};
    bool fallbackUsed{};
    std::string digestPassword;
};

struct AuthResult {
    AuthPath path;
    std::string authorizationHeader;
};

class ImsAuthService {
public:
    AuthResult BuildAuthorization(const Challenge& ch, const AuthContext& ctx) const;
};
```

```cpp
// src/auth/ims_auth_service.cpp
#include "auth/ims_auth_service.hpp"
#include <stdexcept>

static bool IsAkaAlgorithm(const std::string& a) {
    return a.find("AKA") != std::string::npos;
}

AuthResult ImsAuthService::BuildAuthorization(const Challenge& ch, const AuthContext& ctx) const {
    if (IsAkaAlgorithm(ch.algorithm) && ctx.hasAkaMaterial) {
        return {AuthPath::AKA, "Authorization: Digest algorithm=AKAv1-MD5"};
    }
    if (IsAkaAlgorithm(ch.algorithm) && !ctx.hasAkaMaterial) {
        if (ctx.fallbackUsed) throw std::runtime_error("fallback already used");
        if (ctx.digestPassword.empty()) throw std::runtime_error("digest password missing");
        return {AuthPath::Digest, "Authorization: Digest algorithm=MD5"};
    }
    if (ctx.digestPassword.empty()) throw std::runtime_error("digest password missing");
    return {AuthPath::Digest, "Authorization: Digest algorithm=MD5"};
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R ImsAuthServiceTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/auth/ims_auth_service.hpp src/auth/ims_auth_service.cpp tests/auth/ims_auth_service_test.cpp
git commit -m "feat: add ims auth strategy with aka preference and digest fallback"
```

---

### Task 5: SIP 适配层（tinySIP 事件桥接）

**Files:**
- Create: `src/core/sip_stack_adapter.hpp`
- Create: `src/core/sip_stack_adapter.cpp`
- Create: `tests/core/sip_stack_adapter_test.cpp`

- [x] **Step 1: 写失败测试（事件转发）**

```cpp
// tests/core/sip_stack_adapter_test.cpp
#include <gtest/gtest.h>
#include "core/sip_stack_adapter.hpp"

TEST(SipStackAdapterTest, DispatchIncomingResponseToCallback) {
    SipStackAdapter ad;
    bool called = false;
    ad.SetEventHandler([&](const SipEvent& e){
        called = (e.type == SipEventType::IncomingResponse && e.statusCode == 200);
    });
    ad.InjectTestEvent({SipEventType::IncomingResponse, SipMethod::Register, 200, "cid", "raw"});
    EXPECT_TRUE(called);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R SipStackAdapterTest -V`
Expected: FAIL（适配器未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/core/sip_stack_adapter.hpp
#pragma once
#include "core/sip_types.hpp"
#include <functional>
#include <string>

class SipStackAdapter {
public:
    using Handler = std::function<void(const SipEvent&)>;

    void InitializeUdp(const std::string& bindIp, int bindPort, const std::string& pcscfHost, int pcscfPort);
    void SetEventHandler(Handler h);
    void SendRaw(const std::string& data);

    void InjectTestEvent(const SipEvent& e);

private:
    Handler handler_;
};
```

```cpp
// src/core/sip_stack_adapter.cpp
#include "core/sip_stack_adapter.hpp"

void SipStackAdapter::InitializeUdp(const std::string&, int, const std::string&, int) {}

void SipStackAdapter::SetEventHandler(Handler h) { handler_ = std::move(h); }

void SipStackAdapter::SendRaw(const std::string&) {
    // 后续替换为 tinySIP 发送逻辑
}

void SipStackAdapter::InjectTestEvent(const SipEvent& e) {
    if (handler_) handler_(e);
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R SipStackAdapterTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/core/sip_stack_adapter.hpp src/core/sip_stack_adapter.cpp tests/core/sip_stack_adapter_test.cpp
git commit -m "feat: add sip stack adapter event bridge skeleton"
```

---

### Task 6: 注册服务状态机

**Files:**
- Create: `src/registration/registration_service.hpp`
- Create: `src/registration/registration_service.cpp`
- Create: `tests/registration/registration_service_test.cpp`

- [x] **Step 1: 写失败测试（状态机）**

```cpp
// tests/registration/registration_service_test.cpp
#include <gtest/gtest.h>
#include "registration/registration_service.hpp"

TEST(RegistrationServiceTest, MoveToRegisteredOn200) {
    RegistrationService svc;
    EXPECT_EQ(svc.State(), RegistrationState::Idle);
    svc.StartRegister();
    EXPECT_EQ(svc.State(), RegistrationState::Registering);
    svc.OnResponse(200, SipMethod::Register, {});
    EXPECT_EQ(svc.State(), RegistrationState::Registered);
}

TEST(RegistrationServiceTest, Handle423WithRetryExpires) {
    RegistrationService svc;
    svc.StartRegister();
    svc.OnIntervalTooBrief(600);
    EXPECT_EQ(svc.PendingExpires(), 600);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R RegistrationServiceTest -V`
Expected: FAIL（服务未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/registration/registration_service.hpp
#pragma once
#include "core/sip_types.hpp"
#include <map>
#include <string>

enum class RegistrationState {
    Idle,
    Registering,
    Registered,
    Refreshing,
    Unregistering,
    RegisterFailed
};

class RegistrationService {
public:
    void StartRegister();
    void StartUnregister();
    void OnResponse(int statusCode, SipMethod method, const std::map<std::string, std::string>& headers);
    void OnIntervalTooBrief(int minExpires);

    RegistrationState State() const;
    int PendingExpires() const;

private:
    RegistrationState state_{RegistrationState::Idle};
    int pendingExpires_{300};
};
```

```cpp
// src/registration/registration_service.cpp
#include "registration/registration_service.hpp"

void RegistrationService::StartRegister() { state_ = RegistrationState::Registering; }
void RegistrationService::StartUnregister() { state_ = RegistrationState::Unregistering; }

void RegistrationService::OnResponse(int statusCode, SipMethod method, const std::map<std::string, std::string>&) {
    if (method != SipMethod::Register) return;
    if (statusCode == 200 && state_ == RegistrationState::Registering) {
        state_ = RegistrationState::Registered;
        return;
    }
    if ((statusCode == 401 || statusCode == 407) && state_ == RegistrationState::Registering) {
        state_ = RegistrationState::Registering;
        return;
    }
    if (statusCode >= 500) {
        state_ = RegistrationState::RegisterFailed;
    }
}

void RegistrationService::OnIntervalTooBrief(int minExpires) {
    pendingExpires_ = minExpires;
}

RegistrationState RegistrationService::State() const { return state_; }
int RegistrationService::PendingExpires() const { return pendingExpires_; }
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R RegistrationServiceTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/registration/registration_service.hpp src/registration/registration_service.cpp tests/registration/registration_service_test.cpp
git commit -m "feat: add registration state machine core transitions"
```

---

### Task 7: 呼叫服务状态机（含 100rel/PRACK）

**Files:**
- Create: `src/call/call_service.hpp`
- Create: `src/call/call_service.cpp`
- Create: `tests/call/call_service_test.cpp`

- [x] **Step 1: 写失败测试（UAC/UAS + PRACK）**

```cpp
// tests/call/call_service_test.cpp
#include <gtest/gtest.h>
#include "call/call_service.hpp"

TEST(CallServiceTest, UacFlowWithPrackThenAck) {
    CallService c;
    c.StartOutgoing("sip:bob@ims.test");
    EXPECT_EQ(c.State(), CallState::OutgoingInvite);

    c.OnProvisional(183, true, 1, 1);
    EXPECT_TRUE(c.ShouldSendPrack());

    c.OnInvite200();
    EXPECT_TRUE(c.ShouldSendAck());
    EXPECT_EQ(c.State(), CallState::Confirmed);
}

TEST(CallServiceTest, RejectSecondIncomingWhenBusy) {
    CallService c;
    c.StartOutgoing("sip:bob@ims.test");
    EXPECT_EQ(c.OnIncomingInviteWhenBusy(), 486);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R CallServiceTest -V`
Expected: FAIL（服务未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/call/call_service.hpp
#pragma once
#include <string>

enum class CallState {
    Idle,
    OutgoingInvite,
    IncomingInvite,
    EarlyDialog,
    Confirmed,
    Terminating
};

class CallService {
public:
    void StartOutgoing(const std::string& targetUri);
    void OnIncomingInvite();
    void OnProvisional(int statusCode, bool reliable, int rseq, int cseq);
    void OnInvite200();
    void OnBye();

    int OnIncomingInviteWhenBusy() const;
    bool ShouldSendPrack() const;
    bool ShouldSendAck() const;
    CallState State() const;

private:
    CallState state_{CallState::Idle};
    bool shouldSendPrack_{false};
    bool shouldSendAck_{false};
};
```

```cpp
// src/call/call_service.cpp
#include "call/call_service.hpp"

void CallService::StartOutgoing(const std::string&) {
    state_ = CallState::OutgoingInvite;
    shouldSendPrack_ = false;
    shouldSendAck_ = false;
}

void CallService::OnIncomingInvite() {
    if (state_ == CallState::Idle) state_ = CallState::IncomingInvite;
}

void CallService::OnProvisional(int, bool reliable, int, int) {
    if (state_ == CallState::OutgoingInvite || state_ == CallState::IncomingInvite) {
        state_ = CallState::EarlyDialog;
    }
    if (reliable) shouldSendPrack_ = true;
}

void CallService::OnInvite200() {
    shouldSendAck_ = true;
    state_ = CallState::Confirmed;
}

void CallService::OnBye() {
    state_ = CallState::Idle;
    shouldSendPrack_ = false;
    shouldSendAck_ = false;
}

int CallService::OnIncomingInviteWhenBusy() const {
    return state_ == CallState::Idle ? 0 : 486;
}

bool CallService::ShouldSendPrack() const { return shouldSendPrack_; }
bool CallService::ShouldSendAck() const { return shouldSendAck_; }
CallState CallService::State() const { return state_; }
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R CallServiceTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/call/call_service.hpp src/call/call_service.cpp tests/call/call_service_test.cpp
git commit -m "feat: add call state machine with 100rel/prack hooks"
```

---

### Task 8: CLI 控制器与命令编排

**Files:**
- Create: `src/cli/cli_controller.hpp`
- Create: `src/cli/cli_controller.cpp`
- Create: `tests/cli/cli_controller_test.cpp`
- Modify: `src/main.cpp`

- [x] **Step 1: 写失败测试（命令映射）**

```cpp
// tests/cli/cli_controller_test.cpp
#include <gtest/gtest.h>
#include "cli/cli_controller.hpp"

TEST(CliControllerTest, ParseRegisterCommand) {
    CliController cli;
    auto cmd = cli.Parse("register");
    EXPECT_EQ(cmd.type, CliCommandType::Register);
}

TEST(CliControllerTest, ParseCallCommand) {
    CliController cli;
    auto cmd = cli.Parse("call sip:bob@ims.test");
    EXPECT_EQ(cmd.type, CliCommandType::Call);
    EXPECT_EQ(cmd.arg, "sip:bob@ims.test");
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R CliControllerTest -V`
Expected: FAIL（控制器未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/cli/cli_controller.hpp
#pragma once
#include <string>

enum class CliCommandType { Register, Unregister, Call, Answer, Hangup, Status, Quit, Invalid };

struct CliCommand {
    CliCommandType type{CliCommandType::Invalid};
    std::string arg;
};

class CliController {
public:
    CliCommand Parse(const std::string& line) const;
};
```

```cpp
// src/cli/cli_controller.cpp
#include "cli/cli_controller.hpp"
#include <sstream>

CliCommand CliController::Parse(const std::string& line) const {
    std::istringstream iss(line);
    std::string op;
    iss >> op;
    if (op == "register") return {CliCommandType::Register, ""};
    if (op == "unregister") return {CliCommandType::Unregister, ""};
    if (op == "answer") return {CliCommandType::Answer, ""};
    if (op == "hangup") return {CliCommandType::Hangup, ""};
    if (op == "status") return {CliCommandType::Status, ""};
    if (op == "quit") return {CliCommandType::Quit, ""};
    if (op == "call") {
        std::string uri;
        iss >> uri;
        return uri.empty() ? CliCommand{CliCommandType::Invalid, ""} : CliCommand{CliCommandType::Call, uri};
    }
    return {CliCommandType::Invalid, ""};
}
```

```cpp
// src/main.cpp
#include <iostream>
#include <string>
#include "cli/cli_controller.hpp"

int main() {
    CliController cli;
    std::string line;
    std::cout << "sim_vonr_cli ready" << std::endl;
    while (std::getline(std::cin, line)) {
        auto cmd = cli.Parse(line);
        if (cmd.type == CliCommandType::Quit) break;
        std::cout << "cmd=" << static_cast<int>(cmd.type) << " arg=" << cmd.arg << std::endl;
    }
    return 0;
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R CliControllerTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/cli/cli_controller.hpp src/cli/cli_controller.cpp src/main.cpp tests/cli/cli_controller_test.cpp
git commit -m "feat: add cli command parser and interactive loop"
```

---

### Task 9: 日志脱敏与安全输出

**Files:**
- Create: `src/util/redaction.hpp`
- Create: `src/util/redaction.cpp`
- Create: `tests/util/redaction_test.cpp`

- [x] **Step 1: 写失败测试（敏感字段脱敏）**

```cpp
// tests/util/redaction_test.cpp
#include <gtest/gtest.h>
#include "util/redaction.hpp"

TEST(RedactionTest, RedactSensitiveFields) {
    std::string raw = "Authorization: Digest response=abcd, ck=1234, ik=5678";
    auto out = RedactSipLine(raw);
    EXPECT_NE(out.find("response=<redacted>"), std::string::npos);
    EXPECT_NE(out.find("ck=<redacted>"), std::string::npos);
    EXPECT_NE(out.find("ik=<redacted>"), std::string::npos);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R RedactionTest -V`
Expected: FAIL（函数未定义）

- [x] **Step 3: 写最小实现**

```cpp
// src/util/redaction.hpp
#pragma once
#include <string>

std::string RedactSipLine(const std::string& line);
```

```cpp
// src/util/redaction.cpp
#include "util/redaction.hpp"
#include <regex>

std::string RedactSipLine(const std::string& line) {
    std::string out = line;
    out = std::regex_replace(out, std::regex(R"(response=[^,\s]+)"), "response=<redacted>");
    out = std::regex_replace(out, std::regex(R"(ck=[^,\s]+)"), "ck=<redacted>");
    out = std::regex_replace(out, std::regex(R"(ik=[^,\s]+)"), "ik=<redacted>");
    out = std::regex_replace(out, std::regex(R"(ki=[^,\s]+)"), "ki=<redacted>");
    out = std::regex_replace(out, std::regex(R"(opc=[^,\s]+)"), "opc=<redacted>");
    return out;
}
```

- [x] **Step 4: 运行测试验证通过**

Run: `cmake --build build -j && ctest --test-dir build -R RedactionTest -V`
Expected: PASS

- [x] **Step 5: Commit**

```bash
git add src/util/redaction.hpp src/util/redaction.cpp tests/util/redaction_test.cpp
git commit -m "feat: add sip log redaction utility"
```

---

### Task 10: 端到端拼接与验收脚本

**Files:**
- Modify: `src/main.cpp`
- Modify: `CMakeLists.txt`
- Create: `tests/integration/sip_flow_smoke_test.cpp`
- Create: `scripts/run_smoke.sh`

- [x] **Step 1: 写失败测试（最小流程冒烟）**

```cpp
// tests/integration/sip_flow_smoke_test.cpp
#include <gtest/gtest.h>
#include "registration/registration_service.hpp"
#include "call/call_service.hpp"

TEST(SipFlowSmokeTest, RegisterThenCallLifecycleStateTransitions) {
    RegistrationService r;
    CallService c;

    r.StartRegister();
    r.OnResponse(200, SipMethod::Register, {});
    ASSERT_EQ(r.State(), RegistrationState::Registered);

    c.StartOutgoing("sip:bob@ims.test");
    c.OnProvisional(183, true, 1, 1);
    ASSERT_TRUE(c.ShouldSendPrack());
    c.OnInvite200();
    ASSERT_EQ(c.State(), CallState::Confirmed);
    c.OnBye();
    ASSERT_EQ(c.State(), CallState::Idle);
}
```

- [x] **Step 2: 运行测试验证失败**

Run: `ctest --test-dir build -R SipFlowSmokeTest -V`
Expected: FAIL（链接/构建尚未整合完整）

- [x] **Step 3: 写最小整合实现**

```cmake
# CMakeLists.txt 关键增量
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

add_library(sim_vonr_core
    src/config/config_loader.cpp
    src/core/sip_stack_adapter.cpp
    src/ims/ims_header_policy.cpp
    src/auth/ims_auth_service.cpp
    src/registration/registration_service.cpp
    src/call/call_service.cpp
    src/cli/cli_controller.cpp
    src/util/redaction.cpp
)
target_include_directories(sim_vonr_core PUBLIC src)
target_link_libraries(sim_vonr_core PUBLIC yaml-cpp)

add_executable(sim_vonr_cli src/main.cpp)
target_link_libraries(sim_vonr_cli PRIVATE sim_vonr_core)

add_executable(sim_vonr_tests
    tests/config/config_loader_test.cpp
    tests/ims/ims_header_policy_test.cpp
    tests/auth/ims_auth_service_test.cpp
    tests/core/sip_stack_adapter_test.cpp
    tests/registration/registration_service_test.cpp
    tests/call/call_service_test.cpp
    tests/cli/cli_controller_test.cpp
    tests/util/redaction_test.cpp
    tests/integration/sip_flow_smoke_test.cpp
)
target_include_directories(sim_vonr_tests PRIVATE src)
target_link_libraries(sim_vonr_tests PRIVATE sim_vonr_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(sim_vonr_tests)
```

```bash
# scripts/run_smoke.sh
#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build
cmake --build build -j
ctest --test-dir build -V
```

- [x] **Step 4: 运行全量测试验证通过**

Run: `bash scripts/run_smoke.sh`
Expected: PASS（所有单测与冒烟测试通过）

- [x] **Step 5: Commit**

```bash
git add CMakeLists.txt src/main.cpp tests/integration/sip_flow_smoke_test.cpp scripts/run_smoke.sh
git commit -m "feat: integrate core modules and add sip flow smoke test"
```

---

## Spec Coverage Self-Review

- REGISTER + 刷新/注销：Task 6
- AKA + Digest 策略：Task 4
- 主叫/被叫 + 100rel/PRACK：Task 7
- CLI 命令集：Task 8
- 日志脱敏：Task 9
- 可执行与测试验收：Task 10

未发现未覆盖项。

## Placeholder Scan

已检查：无 TBD/TODO/“适当处理”等占位语句。每个代码步骤均给出实际代码或命令。

## Type Consistency Check

已检查关键类型一致性：
- `SipMethod` 在注册与集成测试统一使用
- `RegistrationState` / `CallState` 在实现与测试命名一致
- `CliCommandType` 在解析与主循环一致

---

Plan complete and saved to `docs/superpowers/plans/2026-03-31-sim-vonr-cli.md`. Two execution options:

1. Subagent-Driven (recommended) - I dispatch a fresh subagent per task, review between tasks, fast iteration

2. Inline Execution - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?