# dsntp-ctl/api

权威契约不在本目录内嵌副本，统一引用：

| 文件 | 说明 |
|------|------|
| `开发工程/api/agent.proto` | IF-CTL gRPC（ReportStream / Heartbeat / ConfigAck） |
| `开发工程/api/openapi-v1.yaml` | IF-CTL HTTP |
| `D:\dsntp\api\agent.proto` | 仓库根副本 |

本阶段 HTTP 骨架已实现于 `internal/httpapi`；gRPC 生成与接入为 M4 后续工作。
