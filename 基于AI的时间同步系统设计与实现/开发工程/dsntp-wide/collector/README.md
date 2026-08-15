# 宽同步偏差采集器（占位）
#
# 规划：周期性读取交换机 SNMP / 摄像机 ONVIF 时间，计算相对网关偏差，
# 经 HTTPS/gRPC 写入 dsntp-ctl 事件流（IF-CTL-008）。
#
# 本目录当前仅占位，不实现协议客户端。

print("dsntp-wide collector placeholder — implement SNMP/ONVIF in M6")
