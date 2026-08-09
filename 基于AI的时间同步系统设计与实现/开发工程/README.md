# DSNTP 开发工程

基于 **DSNTP-DRS-002 V2.0**《开发需求说明书（合并版）》的官方工程骨架根目录。

配套上下文文档：`项目汇报材料/工程骨架与开发上下文.md`（DSNTP-CTX-001 V2.0）。

## 仓库布局

| 子目录 | 里程碑 | 说明 |
|--------|--------|------|
| [`dsntp-agent/`](dsntp-agent/) | M1～M3 | 精同步共识 Agent（C11），可编译骨架 |
| [`dsntp-ctl/`](dsntp-ctl/) | M4 | 管控 Time Control Server（占位） |
| [`dsntp-ntp/`](dsntp-ntp/) | M5 | NTP 网关 chrony + time-shim（占位） |

参考实现（只读对照）：`D:\dsntp\dtc-c\`。

## 系统关系

```text
dsntp-ctl (管控面) ◄── Reporter ── dsntp-agent ◄──UDP:47500 TSYN──► peers
                                      │
                                      ▼ UDS synced_ns
                                 dsntp-ntp → UDP:123 → 异构设备
```

红线：管控与 NTP **不得**改写 P2P 共识 \(T_c\)；FAULT_NOTIFY=`0x06`；Signature=`64B`。

## 需求文档

- `D:\dsntp\项目汇报材料\物理隔离网络分布式时间共识同步系统-开发需求说明书.md`
- `D:\dsntp\项目汇报材料\工程骨架与开发上下文.md`
