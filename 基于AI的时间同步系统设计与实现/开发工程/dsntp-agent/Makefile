CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Iinclude
LDFLAGS ?=
ifeq ($(OS),Windows_NT)
  LDFLAGS += -lws2_32
else
  LDFLAGS += -lm -lpthread
endif

SRCS = \
  src/protocol/codec.c src/protocol/payload.c \
  src/clock/clock.c src/fsm/fsm.c src/measure/measure.c \
  src/consensus/consensus.c src/recover/recover.c \
  src/crypto/crypto.c src/net/udp.c src/config/config.c \
  src/api/local_sock.c src/agent/agent.c src/main.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: dsntp-agent

dsntp-agent: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

test: test_smoke
	./test_smoke

test_smoke: tests/test_smoke.c src/protocol/codec.c src/fsm/fsm.c src/consensus/consensus.c src/measure/measure.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) dsntp-agent test_smoke
