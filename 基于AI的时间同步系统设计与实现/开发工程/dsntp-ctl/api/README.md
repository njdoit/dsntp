# dsntp-ctl API 占位

消息契约以仓库 `api/agent.proto`（IF-CTL）为准。

规划方法（DRS-002 §9.2）：

| 接口 | 方向 | 说明 |
|------|------|------|
| ReportStream | Agent ↔ Server | TimeReport / Event；下推配置与指令 |
| Heartbeat | Agent → Server | 轻量心跳 |
| ConfigAck | Agent → Server | 配置应用回执 |
| GET /api/v1/nodes | 运维 → Server | 节点列表 |
| POST /api/v1/config/publish | 运维 → Server | 发布配置 |
| POST /api/v1/nodes/{id}/ext-sync | 运维 → Server | 触发 Ext_Sync |

本目录暂不放置生成代码；实现时在此加入 stub 或生成物说明。
