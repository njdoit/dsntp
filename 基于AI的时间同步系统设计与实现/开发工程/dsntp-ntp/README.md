# dsntp-ntp — NTP 网关（系统级 M5 骨架）

| 项目 | 内容 |
|------|------|
| 里程碑 | **M5** |
| 需求 | FR-NTP、IF-NTP |
| 对外 | UDP **123** |
| 状态 | chrony 示例 + time-shim **C 源码骨架**（需 Linux/WSL 编译） |

## 数据流

```text
dsntp-agent ──UDS──► time-shim ──SOCK──► chrony ──UDP:123──► dsntp-wide 客户端
```

## 目录

```text
dsntp-ntp/
├── chrony/chrony.conf.example
├── time-shim/{main.c,Makefile}
└── deploy/   # 预留节点级部署片段
```

## 红线

1. 不参与 TSYN，不改写 \(T_c\)
2. chrony **禁止**外源 NTP
3. 生产主备双网关；客户端双 server

## 构建（Linux）

```bash
make -C time-shim
# 安装 chrony 后使用 chrony/chrony.conf.example
```
