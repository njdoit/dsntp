# C 语言工程骨架与开发上下文

| 项目 | 内容 |
|------|------|
| 文档编号 | **DSNTP-CTX-C-001** |
| 文档名称 | C 语言工程骨架与开发上下文 |
| 版本 | **V1.0** |
| 日期 | 2026-08-09 |
| 需求基线 | **DSNTP-DRS-002 V2.0**《开发需求说明书（合并版）》 |
| 总骨架 | **DSNTP-CTX-001 V2.1**《工程骨架与开发上下文》 |
| 主工程 | `基于AI的时间同步系统设计与实现/开发工程/dsntp-agent`（C11） |
| 参考实现 | `D:\dsntp\dtc-c`（C11，可对照移植） |
| 协议定稿 | DRS §7 / §12；Magic=`TSYN`，FAULT=`0x06`，Signature=`64B` |
| 用途 | 面向 **C 语言 Agent** 的模块 API、编译联调、填空顺序与红线；指导 M1～M3（及 Reporter/shim 的 C 侧）续开发 |

### 修订记录

| 版本 | 日期 | 说明 |
|------|------|------|
| **V1.0** | **2026-08-09** | **首版：在 CTX-001 系统骨架之上，固化 `dsntp-agent` C11 目录、头文件契约、线程与构建约定** |

### 文档关系

```text
DRS-002（合并版）
    │
    ├─► CTX-001 V2.1     系统级骨架（Agent / ctl / ntp）
    │
    └─► 本文档 CTX-C-001  C11 Agent 工程骨架（本文件）
              │
              ▼
         开发工程/dsntp-agent   ← 主开发树
         dtc-c                  ← 参考移植
```

| 路径 | 说明 |
|------|------|
| `项目汇报材料/工程骨架与开发上下文-C语言.md` | **本文档权威稿** |
| `基于AI的时间同步系统设计与实现/过程文档/工程骨架与开发上下文-C语言.md` | 同步副本 |
| `项目汇报材料/工程骨架与开发上下文.md` | 系统级 CTX-001 |
| `开发工程/dsntp-agent/` | C11 主工程 |

管控 Server（`dsntp-ctl`）可用 Go/HTTPS；**精同步热路径必须以 C Agent 实现**（DRS ENV-OS-005）。NTP `time-shim` 可用 C，消费本机 UDS。

---

## 1. 技术选型与平台

| 项 | 定稿 |
|----|------|
| 语言标准 | **C11**（`CMAKE_C_STANDARD 11`） |
| 构建 | CMake ≥ 3.16（推荐）或根目录 `Makefile` |
| 目标 OS | Linux 为主（PREEMPT_RT 为 M6）；开发可用 MSYS2/MinGW |
| 时钟源 | `clock_gettime(CLOCK_MONOTONIC)` → ns（`dsntp_ns_t` / `uint64_t`） |
| 网络 | POSIX UDP socket；默认端口 **47500**；建议 `SO_PRIORITY=7` |
| 本机 API | Unix Domain Socket（默认 `/var/run/time-agent.sock`） |
| 密码学 | OpenSSL ≥ 1.1 `libcrypto`，ECDSA-P256；Signature **64B** `r‖s` |
| 链接（Linux） | `m`、`pthread`；Win 骨架链 `ws2_32`（生产以 Linux 验收为准） |
| 命名空间 | 前缀 `dsntp_` / 宏 `DSNTP_`；头文件目录 `include/dsntp/` |

### 1.1 依赖安装（摘要）

```bash
# Debian/Ubuntu
sudo apt install -y build-essential cmake libssl-dev

# MSYS2 UCRT64（仅开发）
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-openssl
```

生产节点：**纯共识**关闭外源 NTPd/Chrony；**gateway** 仅允许 chrony/shim 消费本机合成时间（OC-06）。

---

## 2. 仓库与目录树（C 主工程）

```text
开发工程/dsntp-agent/
├── CMakeLists.txt / Makefile
├── README.md
├── include/dsntp/           # 对外 C API 契约（稳定面）
│   ├── types.h              # 错误码、角色、node_id / round / ns
│   ├── protocol.h           # TSYN Header/Payload/Packet + 编解码
│   ├── clock.h              # 单调钟 + 合成钟
│   ├── fsm.h                # 状态 / 事件；PeerFault 不改本机态
│   ├── measure.h            # T0～T3、RTT、滑动窗口回归
│   ├── consensus.h          # 中位数、法定人数
│   ├── recover.h            # PeerFault 投票 + Recovering
│   ├── crypto.h             # sign/verify（Header‖Payload）
│   ├── net.h                # UDP send/recv
│   ├── config.h             # DR-003 key=value
│   ├── api_local.h          # IF-APP UDS
│   └── agent.h              # create / run / destroy
├── src/
│   ├── protocol/{codec.c,payload.c}
│   ├── clock/clock.c
│   ├── fsm/fsm.c
│   ├── measure/measure.c
│   ├── consensus/consensus.c
│   ├── recover/recover.c
│   ├── crypto/crypto.c      # M2：接 OpenSSL（当前占位）
│   ├── net/udp.c
│   ├── config/config.c
│   ├── api/local_sock.c
│   ├── agent/agent.c        # ★ 主闭环填空
│   └── main.c
├── tests/test_smoke.c
├── deploy/{agent.example.conf,dsntp-agent.service}
├── scripts/gen_keys.sh
└── docs/{模块映射.md,状态机报文对照.md}
```

对照移植源：`D:\dsntp\dtc-c\`（头文件前缀 `dtc_`，勿与 `dsntp_` 混链同一进程）。

---

## 3. 模块 ↔ 需求 ↔ 源文件

| 模块 | 头文件 | 源文件 | DRS | 骨架状态 | M 阶段 |
|------|--------|--------|-----|----------|--------|
| types | `types.h` | — | §2 / DR | 已定 | — |
| protocol | `protocol.h` | `codec.c` `payload.c` | §7 IF-TSYN、OC | Header/Packet 可用；payload 字段待齐 | M1 |
| clock | `clock.h` | `clock.c` | FR-CLK、FR-SYN | 单调 + 合成公式 | M1 |
| fsm | `fsm.h` | `fsm.c` | §6 FR-FSM | 合法转移；PeerFault 不改态 | M1 |
| measure | `measure.h` | `measure.c` | FR-MEAS、FR-SYN-001 | RTT/θ̂ + 回归 | M1 |
| consensus | `consensus.h` | `consensus.c` | FR-CNS | 中位数；&lt;2f+1 不更新 | M1 |
| recover | `recover.h` | `recover.c` | FR-FLT | API 有；未接线 agent | M2 |
| crypto | `crypto.h` | `crypto.c` | FR-SEC | **占位** → OpenSSL | M2 |
| net | `net.h` | `udp.c` | UDP 47500 | bind/send/recv | M1 |
| config | `config.h` | `config.c` | DR-003 | key=value | M1 |
| api | `api_local.h` | `local_sock.c` | IF-APP | UDS 骨架 | M1 |
| agent | `agent.h` | `agent.c` | §5.10 / §6.4 | 主循环 **TODO** | M1～M3 |
| reporter | （待建） | （待建） | FR-CTL | 独立线程；禁止热路径阻塞 | M4 |
| ext_sync | （待扩） | （待扩） | Ext_Sync / EXT_TIME_IN | 本地帧，不占 0x01～0x08 | M3 |

---

## 4. C API 契约速查（以头文件为准）

### 4.1 公共类型（`types.h`）

```c
typedef enum dsntp_err {
    DSNTP_OK = 0,
    DSNTP_ERR_INVAL, DSNTP_ERR_IO, DSNTP_ERR_TIMEOUT,
    DSNTP_ERR_CRYPTO, DSNTP_ERR_PROTO, DSNTP_ERR_NOMEM,
    DSNTP_ERR_STATE, DSNTP_ERR_QUORUM
} dsntp_err_t;

typedef uint16_t dsntp_node_id_t;
typedef uint32_t dsntp_round_t;
typedef uint64_t dsntp_ns_t;   /* 纳秒，来自 CLOCK_MONOTONIC */
```

角色：`DSNTP_ROLE_CONSENSUS_ONLY` / `DSNTP_ROLE_GATEWAY`。

### 4.2 协议常量（`protocol.h`）

| 宏 | 值 | 说明 |
|----|-----|------|
| `DSNTP_MAGIC` | `0x5453594E` | "TSYN" |
| `DSNTP_VERSION` | `0x02` | |
| `DSNTP_HEADER_SIZE` | 16 | |
| `DSNTP_SIGNATURE_SIZE` | **64** | OC-01 |
| `DSNTP_TYPE_FAULT_NOTIFY` | **0x06** | OC-02；禁止用 0x03 |
| `DSNTP_DEFAULT_PORT` | 47500 | |

线格式：`Header(16) ‖ Payload ‖ Signature(64)`；字节序**大端**；`#pragma pack(push,1)` 结构体与编解码一致。

AlphaPpm（IF-TSYN-015）：

```c
double  dsntp_alpha_from_wire(int64_t alpha_ppm); /* (ppm/1e6)*1e-6 */
int64_t dsntp_alpha_to_wire(double alpha);
```

强制验签类型：`START_SYNC`、`CONSENSUS_RESULT`、`FAULT_NOTIFY`、`RECOVER_ANNOUNCE`（`dsntp_crypto_type_must_verify`）。

### 4.3 状态机（`fsm.h`）

状态：`INIT → SYNC_WAIT → COLLECTING → CONSENSUS → ESTIMATING → RUNNING`（主闭环）；`FAULT` / `RECOVERING` / `STOPPED` / `EXT_SYNC`。

事件要点：

- `DSNTP_EV_PEER_FAULT_SUSPECT` / `CONFIRM`：**不得**改变本机 `fsm.state`（FR-FSM-002）
- 仅 `DSNTP_EV_LOCAL_FAULT` → `FAULT`

### 4.4 合成时钟（`clock.h`）

```c
dsntp_ns_t dsntp_clock_monotonic_ns(void);
dsntp_ns_t dsntp_clock_synced_ns(const dsntp_clock_t *clk);
/* T_syn = C_now + beta_hat + alpha_hat * (C_now - last_sync_c) */
```

默认**不**改写 `CLOCK_REALTIME`（网关由 shim/chrony 消费 UDS）。

### 4.5 测量 / 共识 / 恢复

| API | 语义 |
|-----|------|
| `dsntp_measure_compute` | RTT≥0 且 ≤2×Dmax，否则无效 |
| `dsntp_window_regress` | 窗口满 L 后最小二乘 α̂/β̂ |
| `dsntp_consensus_median` | 有效样本中位数 → \(T_c\) |
| `dsntp_consensus_quorum_ok` | 样本数 ≥2f+1，否则**不更新** \(T_c\) |
| `dsntp_consensus_ack_quorum` | ACK 数 ≥2f+1 → ConsensusDone |
| `dsntp_peer_fault_*` | 连续 miss → Confirm；票数 ≥f+1 剔除 |
| `dsntp_recover_*` | 静默 ≥2 周期 → ANNOUNCE → ACK≥2f+1 |

### 4.6 Agent 组装（`agent.h` / `agent.c`）

```c
dsntp_agent_t *dsntp_agent_create(const dsntp_config_t *cfg);
dsntp_err_t    dsntp_agent_run(dsntp_agent_t *ag);   /* 阻塞主循环 */
void           dsntp_agent_request_stop(dsntp_agent_t *ag);
void           dsntp_agent_destroy(dsntp_agent_t *ag);
```

内部已聚合：`cfg`、`fsm`、`clock`、`consensus`、`window`、`net*`、`crypto*`、`api*`、`round`。

**当前填空位置（`src/agent/agent.c`）**：

| 状态 | TODO |
|------|------|
| `COLLECTING` | 广播 `TIME_REQ`，收 `TIME_RESP`，算样本；超时 → `PEER_FAULT_SUSPECT` |
| `CONSENSUS` | `median` + 发 `CONSENSUS_RESULT`；收集 ACK≥2f+1 |
| `ESTIMATING` | `window_regress` → `dsntp_clock_update_estimates` |
| `RECOVERING` | 静默 → `RECOVER_ANNOUNCE` → ACK |
| 全程收包路径 | decode →（验签）→ 按 Type 分发 |

---

## 5. 线程与实时模型（C 实现约定）

对齐 NFR-RES-004 / FR-ARCH-004 / FR-CTL-001：

| 线程（建议） | 职责 | 约束 |
|--------------|------|------|
| 主控 / FSM | 状态推进、轮次、调用 measure/consensus | 热路径；可 `SCHED_FIFO`（M6） |
| 网络收 | UDP recv → 入队 | 不调 gRPC/HTTPS |
| 网络发 | 出队 → sendto | 可与收合并，但须低延迟 |
| 定时器 | TimerTick / CollectTimeout(22ms) | 高精度优先 |
| Reporter（M4） | 无锁快照读 α̂/β̂/`synced_ns`/fsm → 上报 | **禁止**在 UDP 回调里阻塞 I/O |
| UDS poll | `dsntp_api_local_poll` | 可挂主循环或轻量线程 |

共享状态：共识热路径写、Reporter/UDS 读 → 用原子/`seqlock` 或短临界区；**不得**在持锁时做签名（长耗时）以外的管控网络。

M6：`mlockall`、CPU 亲和、`isolcpus` 见 DRS §11；systemd 示例见 `deploy/dsntp-agent.service`。

---

## 6. 编码约定

1. **大端线格式**：多字节字段用显式编解码（参考 `codec.c`），勿直接 `memcpy` 本机序结构上线。
2. ** packed 结构**仅描述布局；发送前仍走 `dsntp_*_encode`。
3. **错误码**：函数返回 `dsntp_err_t`；指针创建失败返回 `NULL`。
4. **日志**：状态迁移、轮次、验签失败、PeerFault/LocalFault 必须可审计（后续可接 Reporter Event）。
5. **无全局可变单例**（除只读常量）；上下文挂在 `dsntp_agent_t`。
6. **头文件**可被 C++ `extern "C"` 包含；实现文件保持 `.c`。
7. **禁止**在数据面路径调用管控客户端。

---

## 7. 构建、测试与运行

```bash
cd "基于AI的时间同步系统设计与实现/开发工程/dsntp-agent"
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/dsntp-agent -c deploy/agent.example.conf
```

或 `make && make test`。

密钥（M2）：

```bash
./scripts/gen_keys.sh 5 keys   # 每节点私钥 + 共享公钥目录
```

单测焦点（随 M1/M2 扩展）：

| 测试 | 覆盖 |
|------|------|
| smoke | Type=0x06、FSM PeerFault、median 基本 |
| codec | Header/各 Payload 大端往返 |
| measure | RTT 过滤、回归 |
| alpha | `dsntp_alpha_from_wire` / `to_wire`（AC-16） |
| crypto | 签验、篡改拒绝（AC-07） |

参考实现完整测试树：`dtc-c/tests/`、`dtc-c/docs/编译与测试指南.md`。

---

## 8. 配置项（`dsntp_config_t` / DR-003）

| 字段 | 含义 | 默认意向 |
|------|------|----------|
| `node_id` | 本机 ID | 必填 |
| `role` | consensus_only / gateway | consensus_only |
| `n` / `f` | 须 \(n\geq 3f+1\) | 5 / 1 |
| `period_ms` | T | 1000 |
| `dmax_ms` | Dmax | 10 → CollectTimeout≈22ms |
| `window_L` | L | 10 |
| `port` | UDP | 47500 |
| `peers[]` | `host:port` 或约定格式 | |
| `privkey_path` / `pubkey_dir` | PEM | |
| `require_crypto` / `verify_all` | 强制/扩展验签 | M2 打开 |
| `sock_path` | UDS | `/var/run/time-agent.sock` |

热更新策略见 DRS §5.7 / CTX-001 §3.5：邻居列表滚动重启；T/L/Dmax 下轮生效。

---

## 9. 状态机—报文—C 符号对照

| 场景 | 报文 Type | C 符号 | 主要源码 |
|------|-----------|--------|----------|
| 启动 | 0x01 | `DSNTP_TYPE_START_SYNC` | `agent` / `fsm` |
| 采样 | 0x02/0x03 | `TIME_REQ` / `TIME_RESP` | `measure` + `agent` TODO |
| 共识 | 0x04/0x07 | `CONSENSUS_RESULT` / `ACK` | `consensus` + `agent` TODO |
| 对端故障 | **0x06** | `FAULT_NOTIFY` | `recover` |
| 恢复 | 0x05/0x08 | `RECOVER_ANNOUNCE` / `ACK` | `recover` |
| Ext_Sync | 本地 EXT_TIME_IN | （待实现） | 不占 P2P Type |
| 读时间 | UDS | `dsntp_clock_synced_ns` | `api/local_sock.c` |

完整强制表：DRS §6.3；工程内：`docs/状态机报文对照.md`。

---

## 10. M1～M6（C 侧任务拆解）

| 阶段 | C 工程任务 | 出口 |
|------|------------|------|
| **M1** | 补齐 payload 大端；`COLLECTING`/`CONSENSUS`/`ESTIMATING` 真实收发与合成钟可读 | 3～5 节点 Running；AC-01/02/15/16 |
| **M2** | OpenSSL 接入 `crypto.c`；FAULT/RECOVER 接入主循环；AC-18 齐套 | AC-03/04/07/17/18 |
| **M3** | 多层 3/2 轮路径、周期自适应、EXT_TIME_IN | 算法规格覆盖 |
| **M4** | 新增 `reporter` 模块（pthread）；HTTPS/gRPC 客户端非热路径；对 `dsntp-ctl` | AC-10/11/12 |
| **M5** | `time-shim`（C）读 UDS → chrony SOCK/REFCLK；角色 gateway | AC-13/14 |
| **M6** | RT 调度、72h、70% 流量、RSS/CPU | AC-05/06/08 |

建议编码顺序：

1. `protocol/payload.c` 各 Type 编解码  
2. `agent.c` Collecting 收发 + CollectTimeout  
3. `agent.c` Consensus RESULT/ACK  
4. `crypto.c` OpenSSL  
5. `recover` 事件接线  
6. Ext_Sync / 自适应 / Reporter  

从 `dtc-c` 移植时：逐文件对照语义，改前缀与类型名，保持线格式与 OC 红线不变。

---

## 11. 红线清单（C 实现必守）

1. `FAULT_NOTIFY` = **0x06**，Signature = **64B**  
2. 邻居超时 → PeerFault\*，**检测方不进** `DSNTP_ST_FAULT`  
3. 有效样本 &lt; 2f+1 → **不写** `consensus.tc`  
4. AlphaPpm 必须经 `dsntp_alpha_from_wire`，禁止把线格式整数直接乘 \(\Delta t\)  
5. 签名输入 = `Header ‖ Payload`；关键 Type 强制验签  
6. Reporter / 管控 I/O **不得**阻塞 UDP 热路径  
7. Server / chrony **不得**改写 P2P \(T_c\)  
8. 勿将 `dtc_*` 与 `dsntp_*` 两套状态机链进同一进程  

---

## 12. 与系统级 CTX / 管控 / NTP 的边界

| 组件 | 语言 | 与本文档关系 |
|------|------|----------------|
| `dsntp-agent` | **C11** | 本文档范围 |
| `dsntp-ctl` | Go 或其它 | 见 CTX-001 §5.1；C 仅实现 Reporter 客户端 |
| `dsntp-ntp` / time-shim | C 或脚本+chrony | M5；读 Agent UDS |
| `api/agent.proto` | protobuf | 管控契约；C 侧可用生成桩或 HTTPS JSON |

系统分层图、OC 全文、AC 总表见 CTX-001 / DRS-002，本文不重复展开。

---

## 13. 文档维护

- 需求变更 → 只改 **DRS-002**，再更新 CTX-001 与本 CTX-C。  
- C API / 目录变更 → 更新本文件 §2～§4 与 `dsntp-agent/docs/模块映射.md`。  
- 权威稿：`项目汇报材料/工程骨架与开发上下文-C语言.md`；同步 `过程文档/` 副本。  
- 续开发入口：DRS 相关章 + **本 CTX-C §4 / §10 / §11** + `agent.c` TODO。

---

**文档结束（DSNTP-CTX-C-001 V1.0）**
