# DSNTP 开发工程（系统级骨架）

基于 **DSNTP-DRS-002 V2.0** 的**系统级**官方工程骨架根目录。  
覆盖 L1 管控 / L2 精同步 / L3 时间出口 / L4 宽同步，以及共享契约与部署拓扑。

| 文档 | 路径 |
|------|------|
| 需求基线 | `cproj/物理隔离网络分布式时间共识同步系统-开发需求说明书-合并版.md`（DRS-002） |
| 系统上下文 | `工程骨架与开发上下文.md`（CTX-001 **V2.2**） |
| C Agent 专用 | `工程骨架与开发上下文-C语言.md`（CTX-C-001） |
| 本目录架构说明 | [`docs/系统架构.md`](docs/系统架构.md) |

## 仓库布局

```text
开发工程/
├── Makefile                 # 系统级一键构建入口
├── api/                     # IF-CTL 契约（proto + OpenAPI）
├── deploy/                  # 拓扑 / systemd / compose
├── docs/                    # 系统架构 · 接口矩阵 · 部署拓扑
├── dsntp-agent/             # L2 精同步 Agent（C11，M1～M3）
├── dsntp-ctl/               # L1 Time Control Server（Go，M4）
├── dsntp-ntp/               # L3 chrony + time-shim（M5）
└── dsntp-wide/              # L4 异构 NTP 模板与采集占位（M5～M6）
```

| 子目录 | 层 | 里程碑 | 状态 |
|--------|----|--------|------|
| [`api/`](api/) | 契约 | 共用 | `agent.proto` + `openapi-v1.yaml` |
| [`dsntp-agent/`](dsntp-agent/) | L2 | M1～M3 | 可编译 C11 骨架 |
| [`dsntp-ctl/`](dsntp-ctl/) | L1 | M4 | Go HTTP 可运行骨架；gRPC 待接 proto |
| [`dsntp-ntp/`](dsntp-ntp/) | L3 | M5 | chrony 示例 + time-shim 源码骨架 |
| [`dsntp-wide/`](dsntp-wide/) | L4 | M5～M6 | 设备 NTP 模板 |
| [`deploy/`](deploy/) | 部署 | M6 | `poc-n5.yaml` + systemd |
| [`docs/`](docs/) | 文档 | — | 系统级说明 |

参考实现（只读）：`D:\dsntp\dtc-c\`。契约副本：`D:\dsntp\api\agent.proto`。

## 系统关系

```text
dsntp-ctl (L1) ◄── Reporter / HTTPS JSON ── dsntp-agent (L2)
                      │                         │
                      │                    UDP:47500 TSYN
                      │                         ▼ peers
                      │                    UDS synced_ns
                      │                         ▼
                 HTTP:8080              dsntp-ntp (L3) → UDP:123
                 gRPC:50051                   │
                                              ▼
                                       dsntp-wide (L4)
                                       交换机 / 摄像机 …
```

## 红线

1. 管控与 NTP **不得**改写 P2P 共识 \(T_c\)
2. FAULT_NOTIFY = `0x06`；Signature = `64B`；Magic = `0x5453594E`（"TSYN"）
3. 样本 &lt; 2f+1 → **不更新** \(T_c\)
4. Server 宕机 → Agent **继续**共识

## 快速入口

```bash
cd "基于AI的时间同步系统设计与实现/开发工程"
make tree
make agent          # CMake 构建 Agent
make ctl            # 需 Go 1.22+
# Linux: make ntp-shim
```

联调拓扑见 [`deploy/topology/poc-n5.yaml`](deploy/topology/poc-n5.yaml)。
