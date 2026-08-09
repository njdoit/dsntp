# dsntp-ctl — Time Control Server（占位骨架）

| 项目 | 内容 |
|------|------|
| 里程碑 | **M4**（DRS-002 §14） |
| 需求 | FR-CTL、IF-CTL、NFR-OPS |
| 状态 | **目录占位，业务未实现** |
| 端口（规划） | gRPC **50051**；HTTP 控制台 **8080** |

## 职责

- 汇聚 Agent Reporter 上报（Heartbeat / Metrics / Event）
- 配置版本下发与 ConfigAck
- 告警、审计、最小控制台
- 可选触发 Ext_Sync 指令

## 红线

1. **不得**参与 P2P 时间共识，**不得**改写 \(T_c\)
2. Server 宕机时 Agent 须继续共识（本地缓存配置）
3. 管控 I/O 不得阻塞 Agent UDP 热路径

## 目录

```text
dsntp-ctl/
├── README.md
├── cmd/server/          # 进程入口（待实现）
├── internal/
│   ├── ingest/          # 上报汇聚
│   ├── config/          # 配置版本管理
│   └── alert/           # 告警规则
└── api/                 # 契约说明（见下方）
```

## API 契约

优先引用仓库根目录（若存在）：

- `D:\dsntp\api\agent.proto`

本目录 `api/README.md` 说明字段期望；实现语言建议 Go，亦可用 HTTPS JSON 等价形态（DRS IF-CTL）。

## TimeReport 核心字段（摘要）

`node_id, seq, reported_at_ns, config, params, monotonic_ns, synced_ns, consensus_tc, round, fsm_state, max_peer_offset_ms, rtt_avg_ms, peers[]`

## 下一步

1. 选定实现语言与 `agent.proto` 对齐
2. 实现 ingest + 内存/SQLite 存储
3. 配置发布与 ConfigAck
4. 最小 HTTP 节点列表与告警页
