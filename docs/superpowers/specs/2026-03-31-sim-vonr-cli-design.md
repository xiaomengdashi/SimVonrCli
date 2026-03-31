# SimVoNR CLI 设计说明

## 1. 目标与范围

构建一个基于 tinySIP 的小型 SIP 命令行客户端（现代 C++），用于 IMS/VoNR 场景，支持：

- REGISTER（含刷新与注销）
- 主叫/被叫呼叫流程
- 100rel/PRACK
- AKA + Digest 认证（AKA 优先，Digest 回落）

本版明确不包含：

- RTP 音频收发（仅信令）
- TCP/TLS（仅 UDP）
- 多通话并发（仅单活动通话）

## 2. 总体架构

采用 **C++20 外壳 + tinySIP/tinySAK C 核心**。

### 2.1 模块划分

1. **SipStackAdapter**
   - 封装 tinySIP 初始化、UDP transport、消息收发、事件桥接
   - 对外提供 C++ 回调事件

2. **RegistrationService**
   - 处理 REGISTER、401/407 challenge、刷新、注销
   - 管理注册状态与事务重试规则

3. **ImsAuthService**
   - 统一处理 AKA 与 Digest 认证
   - challenge 识别与 Authorization 头生成

4. **CallService**
   - 管理主叫/被叫会话状态机
   - 处理 INVITE/PRACK/ACK/BYE/CANCEL

5. **ImsHeaderPolicy**
   - 统一拼装 IMS 常见头（按配置启用）
   - 例如 `P-Preferred-Identity`、`P-Access-Network-Info`、`Supported: 100rel`

6. **CliController**
   - 命令解析与交互输出
   - 调用各 service 并输出状态变化

### 2.2 运行模型

- 单进程、事件驱动
- 主线程处理 CLI 命令
- SIP 回调线程/事件分发统一进入状态机更新
- 日志默认脱敏

## 3. 配置设计

配置文件：`config.yaml`

- `local.bind_ip`, `local.bind_port`
- `pcscf.host`, `pcscf.port`, `transport: udp`
- `identity.impi`, `identity.impu`, `identity.realm`
- `auth.mode: aka+digest`
- `auth.digest.password`
- `auth.aka.opc`, `auth.aka.ki`, `auth.aka.amf`, `auth.aka.sqn`（或外部提供接口）
- `sip.user_agent`, `sip.expires`, `sip.enable_100rel: true`
- `ims_headers.enable: true`

本版配置启动时读取一次，运行期不热更新。

## 4. 注册流程设计

### 4.1 状态机

`Idle -> Registering -> Registered -> Refreshing -> Unregistering -> Idle`

失败分支：`RegisterFailed`

### 4.2 流程

1. 发送无鉴权 REGISTER
2. 收到 401/407 后解析 challenge
3. 认证选择：
   - 可识别 AKA challenge：先走 AKA
   - 否则走 Digest
4. 携带认证头重发 REGISTER
5. 收到 200 OK，进入 Registered 并启动刷新计时（`expires * 0.8`）

### 4.3 刷新与注销

- 刷新：按计时重发 REGISTER
- 注销：`Expires: 0`

### 4.4 失败策略

- 自动认证重试上限：2 次
- 超限后进入 RegisterFailed，等待用户手动 `register`
- 若返回 423，按 `Min-Expires` 自动调整并重试

## 5. 认证策略（AKA + Digest）

- 默认优先 AKA
- AKA 计算材料缺失或失败时，允许回落 Digest
- 对同一 challenge 上下文最多执行一次 `AKA -> Digest` 回落，防止死循环
- 日志中不输出密钥、摘要响应等敏感字段

## 6. 呼叫流程设计

### 6.1 主叫（UAC）状态机

`Idle -> OutgoingInvite -> EarlyDialog -> Confirmed -> Terminating -> Idle`

流程：

1. 发送 INVITE（固定 SDP + `Supported: 100rel`）
2. 收到 180/183：
   - 若含 `Require: 100rel` 或 `RSeq`，发送 PRACK
3. 收到 200 INVITE，发送 ACK，进入 Confirmed
4. 任一方 BYE 后释放到 Idle
5. 失败响应（>=300）结束会话并回 Idle

### 6.2 被叫（UAS）状态机

`Idle -> IncomingInvite -> Ringing/Early -> Confirmed -> Terminating -> Idle`

流程：

1. 收到 INVITE，CLI 显示来电
2. 用户 `answer` 后发送 180/183（按可靠临时响应要求处理）与最终 200
3. 收到 ACK 后进入 Confirmed
4. BYE 后回 Idle

忙线策略：已有活动通话时新来电返回 `486 Busy Here`。

### 6.3 100rel / PRACK

- 本版固定支持 100rel
- 收到可靠临时响应必须发 PRACK
- 校验 `RAck` 与 `RSeq/CSeq` 匹配，失败则终止该 early dialog

## 7. CLI 命令

- `register`
- `unregister`
- `call <sip:uri>`
- `answer`
- `hangup`
- `status`
- `quit`（若已注册先尝试注销）

## 8. 错误处理与日志

- 事务超时：打印错误并回退到可重试状态
- 协议错误（例如 PRACK 不匹配）：终止当前呼叫并标记原因
- 认证失败：达到重试上限后停止自动重试
- 日志脱敏：不打印 key/response/ck/ik 等

## 9. 测试与验收标准

以下条件全部满足即完成：

1. IMS 环境下可成功 REGISTER 并自动刷新
2. AKA 与 Digest 两条认证路径均可通过（依 challenge）
3. 主叫流程完整：`INVITE -> (180/183 + PRACK) -> 200 -> ACK -> BYE`
4. 被叫流程完整：接听并正常挂断
5. 100rel/PRACK 流程与 RAck 匹配正确
6. CLI 可独立操作，日志可读且无敏感信息泄露

## 10. 实施边界

本阶段只实现最小可用 IMS 信令客户端，保持单通话、UDP-only、无媒体。后续如需扩展 TCP/TLS、RTP、多路会话，将在新设计与计划中单独定义。