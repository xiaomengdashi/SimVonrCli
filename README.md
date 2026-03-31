# SimVonrCli

`SimVonrCli` 是一个基于 **Doubango tinySIP** 的轻量命令行 SIP/IMS 客户端示例，面向 VoNR 信令流程验证。

当前重点能力：
- UDP 传输
- REGISTER / INVITE 鉴权（默认 AKA，支持 Digest 单次回落）
- 基本呼叫流程：`call` / `answer` / `hangup`
- 来电自动接听（默认开启，可运行时切换）

## 1. 编译

### 依赖
- CMake >= 3.20
- C++20 编译器（g++/clang++）
- yaml-cpp
- GTest（用于测试）
- doubango/tinySIP 及相关静态库（项目默认从 `third_party/doubango/build/install` 链接）

### 构建命令
在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build -j
```

### 运行测试（可选）

```bash
ctest --test-dir build --output-on-failure
```

## 2. 使用

### 准备配置
默认配置文件：`configs/config.example.yaml`

请按你的 IMS 环境修改至少以下字段：
- `pcscf.host`
- `pcscf.port`
- `identity.impi`
- `identity.impu`
- `identity.realm`
- `auth.aka.opc`
- `auth.aka.ki`
- `auth.aka.amf`
- `auth.aka.sqn`（可按网侧策略配置，示例值仅占位）
- `auth.digest.password`（Digest 回落或强制 digest 模式时使用）

可选项：
- `sip.auto_answer: true|false`（默认 `true`）

### 最小可运行配置示例

> 以下为最小示例，请替换为你的真实 IMS 参数。

```yaml
local:
  bind_ip: "0.0.0.0"
  bind_port: 5060

pcscf:
  host: "<your-pcscf-host-or-ip>"
  port: 5060
  transport: "udp"

identity:
  impi: "<your-impi>"
  impu: "sip:<your-impu-user>@<your-realm>"
  realm: "<your-realm>"

auth:
  mode: "aka"
  digest:
    password: "<your-password>"
  aka:
    opc: "11111111111111111111111111111111"
    ki: "11111111111111111111111111111111"
    amf: "8000"
    sqn: "000000000000"

sip:
  user_agent: "sim-vonr-cli/0.1.0"
  expires: 3600
  enable_100rel: false
  auto_answer: true

ims_headers:
  enable: false
```

### 启动

```bash
./build/sim_vonr_cli
```

或指定配置文件：

```bash
./build/sim_vonr_cli /path/to/your/config.yaml
```

### CLI 命令
- `register`：发送 REGISTER
- `call <sip:uri>`：发起呼叫（INVITE）
- `answer`：应答当前来电/会话（200 OK）
- `hangup`：挂断当前会话（BYE）
- `autoanswer on`：开启自动接听
- `autoanswer off`：关闭自动接听
- `quit`：退出程序

## 3. 说明

- 该项目用于 SIP/IMS 信令流程验证与开发调试。
- 默认鉴权模式为 `auth.mode: "aka"`（VoNR 场景），当 AKA 不可用时会执行单次 Digest 回落。
- 若需强制使用 Digest，可将 `auth.mode` 改为 `"digest"`。
- 若使用示例配置中的占位地址（如 `proxy.example.com`），程序会因 DNS/网络不可达而无法建立 SIP 传输。请替换为真实 IMS 参数。