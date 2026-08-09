# dsntp-ntp — NTP 网关（占位骨架）

| 项目 | 内容 |
|------|------|
| 里程碑 | **M5**（DRS-002 §14） |
| 需求 | FR-NTP、IF-NTP |
| 状态 | **目录占位，业务未实现** |
| 对外 | UDP **123** NTP/SNTP |

## 职责

- 消费 Agent 本机合成时间（UDS / `get_synced_time`）
- 经 chrony + time-shim 对外提供标准 NTP/SNTP
- 生产建议主备两台；客户端配置双 server

## 红线

1. **不参与** UDP 47500 共识协议，**不改写** \(T_c\)
2. 禁止从外网/外源 NTP 取时
3. 客户端禁止指向外网 NTP；防火墙仅放行→网关 UDP 123

## 目录

```text
dsntp-ntp/
├── README.md
├── chrony/          # chrony 配置示例占位
└── time-shim/       # 读 UDS → REFCLK/SOCK 占位
```

## 数据流

```text
dsntp-agent ──UDS synced_ns──► time-shim ──► chrony ──UDP:123──► 交换机/摄像机
```

## 下一步

1. 实现 time-shim 读取 `/var/run/time-agent.sock`（或配置路径）
2. 编写 chrony `refclock SOCK` / REFCLK 示例
3. 主备部署与客户端双 server 验收（AC-13/14）
