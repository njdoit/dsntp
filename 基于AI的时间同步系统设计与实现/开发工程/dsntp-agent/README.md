# dsntp-agent — 物理隔离网络分布式时间共识同步（开发骨架）

| 项目 | 内容 |
|------|------|
| 需求基线 | **DSNTP-DRS-002 V2.0**《开发需求说明书（合并版）》 |
| 上下文 | DSNTP-CTX-C-001 **V1.0**《C 语言工程骨架与开发上下文》（[`…-C语言.md`](../../../项目汇报材料/工程骨架与开发上下文-C语言.md)）；系统级见 CTX-001 |
| 协议定稿 | DRS §7 / §12：Type `0x01`～`0x08`，Signature **64B**，FAULT=`0x06`，Magic=`0x5453594E`（"TSYN"） |
| 语言 | C11 |
| 状态 | 可编译骨架：模块 API + FSM 主闭环已搭；网络联调与 OpenSSL 待按 M1/M2 填充 |

> 参考实现：`D:\dsntp\dtc-c`（功能更完整，可对照移植）。

## 目录结构

```text
dsntp-agent/
├── include/dsntp/     公共头文件（API 契约）
├── src/
│   ├── protocol/      IF-TSYN 编解码
│   ├── clock/         FR-CLK / FR-SYN 合成钟
│   ├── fsm/           FR-FSM（PeerFault ≠ LocalFault）
│   ├── measure/       FR-MEAS + 滑动窗口回归
│   ├── consensus/     FR-CNS 中位数 / 法定人数
│   ├── recover/       FR-FLT 故障投票与恢复
│   ├── crypto/        FR-SEC ECDSA 占位
│   ├── net/           UDP 47500
│   ├── config/        DR-003
│   ├── api/           IF-APP Unix Socket
│   ├── agent/         七步主循环组装
│   └── main.c
├── tests/             冒烟单测（含 Type=0x06 / PeerFault）
├── deploy/            配置与 systemd
├── scripts/           密钥生成
├── docs/              模块映射、状态机—报文对照
├── CMakeLists.txt
└── Makefile
```

## 模块 ↔ 需求映射

| 模块 | DRS 需求 | 骨架进度 |
|------|----------|----------|
| protocol | §7 IF-TSYN、OC-01～03 | Header/Packet 编解码可用 |
| clock | FR-CLK、FR-SYN-002 | 单调钟 + 合成公式 |
| fsm | §6 FR-FSM | 合法转移表；PeerFault 不改本机态 |
| measure | FR-MEAS、FR-SYN-001 | RTT/θ̂ + 回归 |
| consensus | FR-CNS | 中位数；不足 2f+1 不更新 |
| recover | FR-FLT | 投票/静默/ACK 计数 API |
| crypto | FR-SEC | 占位（返回 CRYPTO） |
| net | UDP | bind/send/recv |
| config | DR-003 | key=value |
| api | IF-APP | UDS 骨架 |
| agent | §5.7 / §6.4 | 状态机主循环 TODO 填空 |

详见 [docs/模块映射.md](docs/模块映射.md)、[docs/状态机报文对照.md](docs/状态机报文对照.md)。

## 快速构建

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/dsntp-agent -c deploy/agent.example.conf
```

或 `make && make test`。

## 需求与上下文文档

- DRS：`D:\dsntp\项目汇报材料\物理隔离网络分布式时间共识同步系统-开发需求说明书-合并版.md`
- CTX-C（本工程首选）：`D:\dsntp\项目汇报材料\工程骨架与开发上下文-C语言.md`
- CTX（系统级）：`D:\dsntp\项目汇报材料\工程骨架与开发上下文.md`

## 开发分期（DRS §14 / CTX §6）

| 里程碑 | 任务 | 填空位置 |
|--------|------|----------|
| **M1** | TIME_REQ/RESP、中位数、合成钟可读 | `agent.c` Collecting/Consensus；`payload.c` |
| **M2** | OpenSSL 签名、FAULT/RECOVER、AC-18 | `crypto.c`；`agent.c` + `recover` |
| **M3** | 多层共识轻量路径、周期自适应、Ext_Sync | `consensus` / `agent` |
| **M4～M5** | 管控 / NTP | 见同级 `dsntp-ctl`、`dsntp-ntp` |
| **M6** | 72h / 70% 背景流量验收 | 测试与部署 |

## 红线（DRS §12）

1. `FAULT_NOTIFY` Type = **0x06**（禁止 0x03）
2. Signature = **64B**（r‖s）
3. 邻居超时 → PeerFault*，**检测方不进 Fault**
4. 有效样本 &lt; 2f+1 → **不更新** \(T_c\)
5. AlphaPpm 按 IF-TSYN-015 换算
6. Magic = `0x5453594E`（"TSYN"）
