# dsntp-ctl — Time Control Server（系统级 M4 骨架）

| 项目 | 内容 |
|------|------|
| 里程碑 | **M4** |
| 需求 | FR-CTL、IF-CTL、NFR-OPS |
| 语言 | Go 1.22+ |
| 端口 | gRPC **50051**（占位）；HTTP **8080**（已接线） |
| 状态 | **HTTP 可运行骨架**；gRPC/`ReportStream` 待接 `api/agent.proto` |

## 红线

1. **不得**参与 P2P 共识，**不得**改写 \(T_c\)
2. Server 宕机时 Agent 继续共识
3. 管控 I/O 不得进入 Agent UDP 热路径

## 目录

```text
dsntp-ctl/
├── go.mod
├── cmd/server/main.go
├── internal/
│   ├── ingest/     # TimeReport 内存汇聚
│   ├── config/     # 配置版本发布 / Ack
│   ├── alert/      # NFR-OPS-002 阈值骨架
│   └── httpapi/    # IF-CTL-004～008 + /ingest/report
└── api/            # 说明：权威契约在 ../../api/
```

## 运行

```bash
cd dsntp-ctl
go run ./cmd/server -http :8080
# curl http://127.0.0.1:8080/api/v1/health
# curl -X POST http://127.0.0.1:8080/api/v1/ingest/report -d '{"node_id":1,"seq":1,"fsm_state":"Running"}'
```

## 契约

- gRPC：`开发工程/api/agent.proto`（及 `D:\dsntp\api\agent.proto` 副本）
- HTTP：`开发工程/api/openapi-v1.yaml`
