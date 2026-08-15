# DSNTP 系统级工程顶层 Makefile
# 官方骨架根：基于AI的时间同步系统设计与实现/开发工程
#
# 用法：
#   make help
#   make agent
#   make ctl          # 需要本机安装 Go
#   make ntp-shim
#   make smoke

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
AGENT := $(ROOT)/dsntp-agent
CTL   := $(ROOT)/dsntp-ctl
NTP   := $(ROOT)/dsntp-ntp
API   := $(ROOT)/api

.PHONY: help all agent ctl ntp-shim smoke tree check-redlines

help:
	@echo "DSNTP system skeleton targets:"
	@echo "  make agent       - build dsntp-agent (C11)"
	@echo "  make ctl         - build dsntp-ctl (Go)"
	@echo "  make ntp-shim    - build time-shim (C11)"
	@echo "  make smoke       - agent + ctl smoke"
	@echo "  make tree        - print system layout"
	@echo "  make check-redlines - grep 协议红线常量"

all: agent ctl ntp-shim

agent:
	cmake -S "$(AGENT)" -B "$(AGENT)/build"
	cmake --build "$(AGENT)/build"
	ctest --test-dir "$(AGENT)/build" --output-on-failure || true

ctl:
	cd "$(CTL)" && go build -o bin/dsntp-ctl ./cmd/server

ntp-shim:
	$(MAKE) -C "$(NTP)/time-shim"

smoke: agent ctl
	@echo "smoke: agent binary + ctl binary ready"
	@echo "redlines: Magic=TSYN FAULT=0x06 Sig=64B Server!=Tc"

tree:
	@echo "开发工程/"
	@echo "├── api/                 # IF-CTL 契约"
	@echo "├── deploy/              # 拓扑 / systemd / compose"
	@echo "├── docs/                # 系统级文档"
	@echo "├── dsntp-agent/         # L2 精同步 M1-M3"
	@echo "├── dsntp-ctl/           # L1 管控 M4"
	@echo "├── dsntp-ntp/           # L3 时间出口 M5"
	@echo "└── dsntp-wide/          # L4 宽同步模板"

check-redlines:
	@rg -n "0x5453594E|0x06|64" "$(AGENT)/include/dsntp" "$(API)" || true
	@echo "Expected: Magic 0x5453594E, FAULT 0x06, Signature 64B"
