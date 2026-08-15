# Portable Makefile for GCC/Clang (Linux / MSYS2). Prefer CMake when available.
#   make
#   make test

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS_CORE =
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Linux)
  LDFLAGS_CORE += -lm -lpthread
endif

CORE_SRCS = \
  lib/src/protocol/codec.c \
  lib/src/protocol/payload.c \
  lib/src/clock/clock.c \
  lib/src/fsm/fsm.c \
  lib/src/measure/measure.c \
  lib/src/consensus/consensus.c \
  lib/src/recover/recover.c \
  lib/src/crypto/crypto.c \
  lib/src/net/udp.c \
  lib/src/config/config.c

CORE_OBJS = $(CORE_SRCS:%.c=build/%.o)

.PHONY: all test clean agent shim ctl

all: build/dsntp-agent build/dsntp-time-shim build/dsntp-ctl build/test_smoke

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/libdsntp_core.a: $(CORE_OBJS)
	ar rcs $@ $^

build/dsntp-agent: apps/agent/main.c apps/agent/agent.c apps/agent/local_sock.c apps/agent/reporter.c build/libdsntp_core.a
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_CORE)

build/dsntp-time-shim: apps/time-shim/main.c build/libdsntp_core.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_CORE)

build/dsntp-ctl: apps/ctl/main.c
	$(CC) $(CFLAGS) $< -o $@

build/test_smoke: tests/test_smoke.c build/libdsntp_core.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_CORE)

test: build/test_smoke
	./build/test_smoke

clean:
	rm -rf build
