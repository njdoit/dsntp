/* payload.c — 各 Type Payload 编解码占位；M1 优先 TIME_*/RESULT/ACK */
#include "dsntp/protocol.h"
/* 骨架阶段：业务 payload 由 agent 直接 memcpy 结构体（注意大端转换待补）。 */
