# time-shim 占位

读取 Agent 合成时间并驱动 chrony。

规划接口：

1. 连接 Agent UDS（默认 `/var/run/time-agent.sock`）
2. 周期性获取 `synced_ns`
3. 写入 chrony SOCK/REFCLK

```text
// TODO: poll UDS → write chrony sock
```

语言：C 或 Python 均可；须满足 NFR 资源与实时性要求。
